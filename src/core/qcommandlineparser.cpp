/****************************************************************************
**
** Copyright (C) 2013 Laszlo Papp <lpapp@kde.org>
** Copyright (C) 2013 David Faure <faure@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcommandlineparser.h"

#include <QCoreApplication>
#include <QHash>
#include <QVector>
#include <stdio.h>
#include <stdlib.h>

typedef QHash<QString, int> NameHash_t;

class QCommandLineParserPrivate
{
public:
    inline QCommandLineParserPrivate()
        : singleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions),
          builtinVersionOption(false),
          builtinHelpOption(false),
          needsParsing(true)
    { }

    bool parse(const QStringList &args);
    void checkParsed(const char *method);
    QStringList aliases(const QString &name) const;
    QString helpText() const;
    bool registerFoundOption(const QString &optionName);
    bool parseOptionValue(const QString &optionName, const QString &argument,
                          QStringList::const_iterator *argumentIterator,
                          QStringList::const_iterator argsEnd);

    //! Error text set when parse() returns false
    QString errorText;

    //! The command line options used for parsing
    QList<QCommandLineOption> commandLineOptionList;

    //! Hash mapping option names to their offsets in commandLineOptionList and optionArgumentList.
    NameHash_t nameHash;

    //! Option values found (only for options with a value)
    QHash<int, QStringList> optionValuesHash;

    //! Names of options found on the command line.
    QStringList optionNames;

    //! Arguments which did not belong to any option.
    QStringList positionalArgumentList;

    //! Names of options which were unknown.
    QStringList unknownOptionNames;

    //! Application description
    QString description;

    //! Documentation for positional arguments
    struct PositionalArgumentDefinition
    {
        QString name;
        QString description;
        QString syntax;
    };
    QVector<PositionalArgumentDefinition> positionalArgumentDefinitions;

    //! The parsing mode for "-abc"
    QCommandLineParser::SingleDashWordOptionMode singleDashWordOptionMode;

    //! Whether addVersionOption was called
    bool builtinVersionOption;

    //! Whether addHelpOption was called
    bool builtinHelpOption;

    //! True if parse() needs to be called
    bool needsParsing;
};

QStringList QCommandLineParserPrivate::aliases(const QString &optionName) const
{
    const NameHash_t::const_iterator it = nameHash.find(optionName);
    if (it == nameHash.end()) {
        qWarning("QCommandLineParser: option not defined: \"%s\"", qPrintable(optionName));
        return QStringList();
    }
    return commandLineOptionList.at(*it).names();
}

/*!
    \since 5.2
    \class QCommandLineParser
    \inmodule QtCore
    \ingroup tools

    \brief The QCommandLineParser class provides a means for handling the
    command line options.

    QCoreApplication provides the command-line arguments as a simple list of strings.
    QCommandLineParser provides the ability to define a set of options, parse the
    command-line arguments, and store which options have actually been used, as
    well as option values.

    Any argument that isn't an option (i.e. doesn't start with a \c{-}) is stored
    as a "positional argument".

    The parser handles short names, long names, more than one name for the same
    option, and option values.

    Options on the command line are recognized as starting with a single or
    double \c{-} character(s).
    The option \c{-} (single dash alone) is a special case, often meaning standard
    input, and not treated as an option. The parser will treat everything after the
    option \c{--} (double dash) as positional arguments.

    Short options are single letters. The option \c{v} would be specified by
    passing \c{-v} on the command line. In the default parsing mode, short options
    can be written in a compact form, for instance \c{-abc} is equivalent to \c{-a -b -c}.
    The parsing mode for can be set to ParseAsLongOptions, in which case \c{-abc}
    will be parsed as the long option \c{abc}.

    Long options are more than one letter long and cannot be compacted together.
    The long option \c{verbose} would be passed as \c{--verbose} or \c{-verbose}.

    Passing values to options can be done using the assignment operator: \c{-v=value}
    \c{--verbose=value}, or a space: \c{-v value} \c{--verbose value}, i.e. the next
    argument is used as value (even if it starts with a \c{-}).

    The parser does not support optional values - if an option is set to
    require a value, one must be present. If such an option is placed last
    and has no value, the option will be treated as if it had not been
    specified.

    The parser does not automatically support negating or disabling long options
    by using the format \c{--disable-option} or \c{--no-option}. However, it is
    possible to handle this case explicitly by making an option with \c{no-option}
    as one of its names, and handling the option explicitly.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser_main.cpp 0

    Known limitation: the parsing of Qt options inside QCoreApplication and subclasses
    happens before QCommandLineParser exists, so it can't take it into account. This
    means any option value that looks like a builtin Qt option, will be treated by
    QCoreApplication as a builtin Qt option. Example: \c{--profile -reverse} will
    lead to QGuiApplication seeing the -reverse option set, and removing it from
    QCoreApplication::arguments() before QCommandLineParser defines the \c{profile}
    option and parses the command line.

    \sa QCommandLineOption, QCoreApplication
*/

/*!
    Constructs a command line parser object.
*/
QCommandLineParser::QCommandLineParser()
    : d(new QCommandLineParserPrivate)
{
}

/*!
    Destroys the command line parser object.
*/
QCommandLineParser::~QCommandLineParser()
{
    delete d;
}

/*!
    \enum QCommandLineParser::SingleDashWordOptionMode

    This enum describes the way the parser interprets command-line
    options that use a single dash followed by multiple letters, as as \c{-abc}.

    \value ParseAsCompactedShortOptions \c{-abc} is interpreted as \c{-a -b -c},
    i.e. as three short options that have been compacted on the command-line,
    if none of the options take a value. If \c{a} takes a value, then it
    is interpreted as \c{-a bc}, i.e. the short option \c{a} followed by the value \c{bc}.
    This is typically used in tools that behave like compilers, in order
    to handle options such as \c{-DDEFINE=VALUE} or \c{-I/include/path}.
    This is the default parsing mode. New applications are recommended to
    use this mode.

    \value ParseAsLongOptions \c{-abc} is interpreted as \c{--abc},
    i.e. as the long option named \c{abc}. This is how Qt's own tools
    (uic, rcc...) have always been parsing arguments. This mode should be
    used for preserving compatibility in applications that were parsing
    arguments in such a way.

    \sa setSingleDashWordOptionMode()
*/

/*!
    Sets the parsing mode to \a singleDashWordOptionMode.
    This must be called before process() or parse().
*/
void QCommandLineParser::setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode singleDashWordOptionMode)
{
    d->singleDashWordOptionMode = singleDashWordOptionMode;
}

/*!
    Adds the option \a option to look for while parsing.

    Returns \c true if adding the option was successful; otherwise returns \c false.

    Adding the option fails if there is no name attached to the option, or
    the option has a name that clashes with an option name added before.
 */
bool QCommandLineParser::addOption(const QCommandLineOption &option)
{
    QStringList optionNames = option.names();

    if (!optionNames.isEmpty()) {
        Q_FOREACH (const QString &name, optionNames) {
            if (d->nameHash.contains(name))
                return false;
        }

        d->commandLineOptionList.append(option);

        const int offset = d->commandLineOptionList.size() - 1;
        Q_FOREACH (const QString &name, optionNames)
            d->nameHash.insert(name, offset);

        return true;
    }

    return false;
}

/*!
    Adds the \c{-v} / \c{--version} option, which displays the version string of the application.

    This option is handled automatically by QCommandLineParser.

    You can set the actual version string by using QCoreApplication::setApplicationVersion().

    Returns the option instance, which can be used to call isSet().
*/
QCommandLineOption QCommandLineParser::addVersionOption()
{
    QCommandLineOption opt(QStringList() << "v" << "version", tr("Displays version information."));
    addOption(opt);
    d->builtinVersionOption = true;
    return opt;
}

/*!
    Adds the help option (\c{-h}, \c{--help} and \c{-?} on Windows)
    This option is handled automatically by QCommandLineParser.

    Remember to use setApplicationDescription to set the application description,
    which will be displayed when this option is used.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser_main.cpp 0

    Returns the option instance, which can be used to call isSet().
*/
QCommandLineOption QCommandLineParser::addHelpOption()
{
    QCommandLineOption opt(QStringList()
#ifdef Q_OS_WIN
                << "?"
#endif
                << "h"
                << "help", tr("Displays this help."));
    addOption(opt);
    d->builtinHelpOption = true;
    return opt;
}

/*!
    Sets the application \a description shown by helpText().
*/
void QCommandLineParser::setApplicationDescription(const QString &description)
{
    d->description = description;
}

/*!
    Returns the application description set in setApplicationDescription().
*/
QString QCommandLineParser::applicationDescription() const
{
    return d->description;
}

/*!
    Defines an additional argument to the application, for the benefit of the help text.

    The argument \a name and \a description will appear under the \c{Arguments:} section
    of the help. If \a syntax is specified, it will be appended to the Usage line, otherwise
    the \a name will be appended.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser.cpp 2

    \sa addHelpOption(), helpText()
*/
void QCommandLineParser::addPositionalArgument(const QString &name, const QString &description, const QString &syntax)
{
    QCommandLineParserPrivate::PositionalArgumentDefinition arg;
    arg.name = name;
    arg.description = description;
    arg.syntax = syntax.isEmpty() ? name : syntax;
    d->positionalArgumentDefinitions.append(arg);
}

/*!
    Clears the definitions of additional arguments from the help text.

    This is only needed for the special case of tools which support multiple commands
    with different options. Once the actual command has been identified, the options
    for this command can be defined, and the help text for the command can be adjusted
    accordingly.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser.cpp 3
*/
void QCommandLineParser::clearPositionalArguments()
{
    d->positionalArgumentDefinitions.clear();
}

/*!
    Parses the command line \a arguments.

    Most programs don't need to call this, a simple call to process() is enough.

    parse() is more low-level, and only does the parsing. The application will have to
    take care of the error handling, using errorText() if parse() returns \c false.
    This can be useful for instance to show a graphical error message in graphical programs.

    Calling parse() instead of process() can also be useful in order to ignore unknown
    options temporarily, because more option definitions will be provided later on
    (depending on one of the arguments), before calling process().

    Don't forget that \a arguments must start with the name of the executable (ignored, though).

    Returns \c false in case of a parse error (unknown option or missing value); returns \c true otherwise.

    \sa process()
*/
bool QCommandLineParser::parse(const QStringList &arguments)
{
    return d->parse(arguments);
}

/*!
    Returns a translated error text for the user.
    This should only be called when parse() returns \c false.
*/
QString QCommandLineParser::errorText() const
{
    if (!d->errorText.isEmpty())
        return d->errorText;
    if (d->unknownOptionNames.count() == 1)
        return tr("Unknown option '%1'.").arg(d->unknownOptionNames.first());
    if (d->unknownOptionNames.count() > 1)
        return tr("Unknown options: %1.").arg(d->unknownOptionNames.join(", "));
    return QString();
}

/*!
    Processes the command line \a arguments.

    In addition to parsing the options (like parse()), this function also handles the builtin
    options and handles errors.

    The builtin options are \c{--version} if addVersionOption was called and \c{--help} if addHelpOption was called.

    When invoking one of these options, or when an error happens (for instance an unknown option was
    passed), the current process will then stop, using the exit() function.

    \sa QCoreApplication::arguments(), parse()
 */
void QCommandLineParser::process(const QStringList &arguments)
{
    if (!d->parse(arguments)) {
        fprintf(stderr, "%s\n", qPrintable(errorText()));
        ::exit(EXIT_FAILURE);
    }

    if (d->builtinVersionOption && isSet("version")) {
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()), qPrintable(QCoreApplication::applicationVersion()));
        ::exit(EXIT_SUCCESS);
    }

    if (d->builtinHelpOption && isSet("help"))
        showHelp(EXIT_SUCCESS);
}

/*!
    \overload

    The command line is obtained from the QCoreApplication instance \a app.
 */
void QCommandLineParser::process(const QCoreApplication &app)
{
    // QCoreApplication::arguments() is static, but the app instance must exist so we require it as parameter
    Q_UNUSED(app);
    process(QCoreApplication::arguments());
}

void QCommandLineParserPrivate::checkParsed(const char *method)
{
    if (needsParsing)
        qWarning("QCommandLineParser: call process() or parse() before %s", method);
}

/*!
    \internal
    Looks up the option \a optionName (found on the command line) and register it as found.
    Returns \c true on success.
 */
bool QCommandLineParserPrivate::registerFoundOption(const QString &optionName)
{
    if (nameHash.contains(optionName)) {
        optionNames.append(optionName);
        return true;
    } else {
        unknownOptionNames.append(optionName);
        return false;
    }
}

/*!
    \internal
    \brief Parse the value for a given option, if it was defined to expect one.

    The value is taken from the next argument, or after the equal sign in \a argument.

    \param optionName the short option name
    \param argument the argument from the command line currently parsed. Only used for -k=value parsing.
    \param argumentIterator iterator to the currently parsed argument. Incremented if the next argument contains the value.
    \param argsEnd args.end(), to check if ++argumentIterator goes out of bounds
    Returns \c true on success.
 */
bool QCommandLineParserPrivate::parseOptionValue(const QString &optionName, const QString &argument,
                                                 QStringList::const_iterator *argumentIterator, QStringList::const_iterator argsEnd)
{
    const QLatin1Char assignChar('=');
    const NameHash_t::const_iterator nameHashIt = nameHash.constFind(optionName);
    if (nameHashIt != nameHash.constEnd()) {
        const int assignPos = argument.indexOf(assignChar);
        const NameHash_t::mapped_type optionOffset = *nameHashIt;
        const bool withValue = !commandLineOptionList.at(optionOffset).valueName().isEmpty();
        if (withValue) {
            if (assignPos == -1) {
                ++(*argumentIterator);
                if (*argumentIterator == argsEnd) {
                    errorText = QCommandLineParser::tr("Missing value after '%1'.").arg(argument);
                    return false;
                }
                optionValuesHash[optionOffset].append(*(*argumentIterator));
            } else {
                optionValuesHash[optionOffset].append(argument.mid(assignPos + 1));
            }
        } else {
            if (assignPos != -1) {
                errorText = QCommandLineParser::tr("Unexpected value after '%1'.").arg(argument.left(assignPos));
                return false;
            }
        }
    }
    return true;
}

/*!
    \internal

    Parse the list of arguments \a args, and fills in
    optionNames, optionValuesHash, unknownOptionNames, positionalArguments, and errorText.

    Any results from a previous parse operation are removed.

    The parser will not look for further options once it encounters the option
    \c{--}; this does not include when \c{--} follows an option that requires a value.
 */
bool QCommandLineParserPrivate::parse(const QStringList &args)
{
    needsParsing = false;
    bool error = false;

    const QString     doubleDashString("--");
    const QLatin1Char dashChar('-');
    const QLatin1Char assignChar('=');

    bool doubleDashFound = false;
    errorText.clear();
    positionalArgumentList.clear();
    optionNames.clear();
    unknownOptionNames.clear();
    optionValuesHash.clear();

    if (args.isEmpty()) {
        qWarning("QCommandLineParser: argument list cannot be empty, it should contain at least the executable name");
        return false;
    }

    QStringList::const_iterator argumentIterator = args.begin();
    ++argumentIterator; // skip executable name

    for (; argumentIterator != args.end() ; ++argumentIterator) {
        QString argument = *argumentIterator;

        if (doubleDashFound) {
            positionalArgumentList.append(argument);
        } else if (argument.startsWith(doubleDashString)) {
            if (argument.length() > 2) {
                QString optionName = argument.mid(2).section(assignChar, 0, 0);
                if (registerFoundOption(optionName)) {
                    if (!parseOptionValue(optionName, argument, &argumentIterator, args.end()))
                        error = true;
                } else {
                    error = true;
                }
            } else {
                doubleDashFound = true;
            }
        } else if (argument.startsWith(dashChar)) {
            if (argument.size() == 1) { // single dash ("stdin")
                positionalArgumentList.append(argument);
                continue;
            }
            switch (singleDashWordOptionMode) {
            case QCommandLineParser::ParseAsCompactedShortOptions:
            {
                QString optionName;
                bool valueFound = false;
                for (int pos = 1 ; pos < argument.size(); ++pos) {
                    optionName = argument.mid(pos, 1);
                    if (!registerFoundOption(optionName)) {
                        error = true;
                    } else {
                        const NameHash_t::const_iterator nameHashIt = nameHash.constFind(optionName);
                        Q_ASSERT(nameHashIt != nameHash.constEnd()); // checked by registerFoundOption
                        const NameHash_t::mapped_type optionOffset = *nameHashIt;
                        const bool withValue = !commandLineOptionList.at(optionOffset).valueName().isEmpty();
                        if (withValue) {
                            if (pos + 1 < argument.size()) {
                                if (argument.at(pos + 1) == assignChar)
                                    ++pos;
                                optionValuesHash[optionOffset].append(argument.mid(pos + 1));
                                valueFound = true;
                            }
                            break;
                        }
                        if (pos + 1 < argument.size() && argument.at(pos + 1) == assignChar)
                            break;
                    }
                }
                if (!valueFound && !parseOptionValue(optionName, argument, &argumentIterator, args.end()))
                    error = true;
                break;
            }
            case QCommandLineParser::ParseAsLongOptions:
            {
                const QString optionName = argument.mid(1).section(assignChar, 0, 0);
                if (registerFoundOption(optionName)) {
                    if (!parseOptionValue(optionName, argument, &argumentIterator, args.end()))
                        error = true;
                } else {
                    error = true;
                }
                break;
            }
            }
        } else {
            positionalArgumentList.append(argument);
        }
        if (argumentIterator == args.end())
            break;
    }
    return !error;
}

/*!
    Checks whether the option \a name was passed to the application.

    Returns \c true if the option \a name was set, false otherwise.

    The name provided can be any long or short name of any option that was
    added with \c addOption(). All the options names are treated as being
    equivalent. If the name is not recognized or that option was not present,
    false is returned.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser.cpp 0
 */

bool QCommandLineParser::isSet(const QString &name) const
{
    d->checkParsed("isSet");
    if (d->optionNames.contains(name))
        return true;
    const QStringList aliases = d->aliases(name);
    Q_FOREACH (const QString &optionName, d->optionNames) {
        if (aliases.contains(optionName))
            return true;
    }
    return false;
}

/*!
    Returns the option value found for the given option name \a optionName, or
    an empty string if not found.

    The name provided can be any long or short name of any option that was
    added with \c addOption(). All the option names are treated as being
    equivalent. If the name is not recognized or that option was not present, an
    empty string is returned.

    For options found by the parser, the last value found for
    that option is returned. If the option wasn't specified on the command line,
    the default value is returned.

    An empty string is returned if the option does not take a value.

    \sa values(), QCommandLineOption::setDefaultValue(), QCommandLineOption::setDefaultValues()
 */

QString QCommandLineParser::value(const QString &optionName) const
{
    d->checkParsed("value");
    const QStringList valueList = values(optionName);

    if (!valueList.isEmpty())
        return valueList.last();

    return QString();
}

/*!
    Returns a list of option values found for the given option name \a
    optionName, or an empty list if not found.

    The name provided can be any long or short name of any option that was
    added with \c addOption(). All the options names are treated as being
    equivalent. If the name is not recognized or that option was not present, an
    empty list is returned.

    For options found by the parser, the list will contain an entry for
    each time the option was encountered by the parser. If the option wasn't
    specified on the command line, the default values are returned.

    An empty list is returned if the option does not take a value.

    \sa value(), QCommandLineOption::setDefaultValue(), QCommandLineOption::setDefaultValues()
 */

QStringList QCommandLineParser::values(const QString &optionName) const
{
    d->checkParsed("values");
    const NameHash_t::const_iterator it = d->nameHash.constFind(optionName);
    if (it != d->nameHash.constEnd()) {
        const int optionOffset = *it;
        QStringList values = d->optionValuesHash.value(optionOffset);
        if (values.isEmpty())
            values = d->commandLineOptionList.at(optionOffset).defaultValues();
        return values;
    }

    qWarning("QCommandLineParser: option not defined: \"%s\"", qPrintable(optionName));
    return QStringList();
}

/*!
    \overload
    Checks whether the \a option was passed to the application.

    Returns \c true if the \a option was set, false otherwise.

    This is the recommended way to check for options with no values.

    Example:
    \snippet code/src_corelib_tools_qcommandlineparser.cpp 1
*/
bool QCommandLineParser::isSet(const QCommandLineOption &option) const
{
    // option.names() might be empty if the constructor failed
    return !option.names().isEmpty() && isSet(option.names().first());
}

/*!
    \overload
    Returns the option value found for the given \a option, or
    an empty string if not found.

    For options found by the parser, the last value found for
    that option is returned. If the option wasn't specified on the command line,
    the default value is returned.

    An empty string is returned if the option does not take a value.

    \sa values(), QCommandLineOption::setDefaultValue(), QCommandLineOption::setDefaultValues()
*/
QString QCommandLineParser::value(const QCommandLineOption &option) const
{
    return value(option.names().first());
}

/*!
    \overload
    Returns a list of option values found for the given \a option,
    or an empty list if not found.

    For options found by the parser, the list will contain an entry for
    each time the option was encountered by the parser. If the option wasn't
    specified on the command line, the default values are returned.

    An empty list is returned if the option does not take a value.

    \sa value(), QCommandLineOption::setDefaultValue(), QCommandLineOption::setDefaultValues()
*/
QStringList QCommandLineParser::values(const QCommandLineOption &option) const
{
    return values(option.names().first());
}

/*!
    Returns a list of positional arguments.

    These are all of the arguments that were not recognized as part of an
    option.
 */

QStringList QCommandLineParser::positionalArguments() const
{
    d->checkParsed("positionalArguments");
    return d->positionalArgumentList;
}

/*!
    Returns a list of option names that were found.

    This returns a list of all the recognized option names found by the
    parser, in the order in which they were found. For any long options
    that were in the form {--option=value}, the value part will have been
    dropped.

    The names in this list do not include the preceding dash characters.
    Names may appear more than once in this list if they were encountered
    more than once by the parser.

    Any entry in the list can be used with \c value() or with
    \c values() to get any relevant option values.
 */

QStringList QCommandLineParser::optionNames() const
{
    d->checkParsed("optionNames");
    return d->optionNames;
}

/*!
    Returns a list of unknown option names.

    This list will include both long an short name options that were not
    recognized. For any long options that were in the form {--option=value},
    the value part will have been dropped and only the long name is added.

    The names in this list do not include the preceding dash characters.
    Names may appear more than once in this list if they were encountered
    more than once by the parser.

    \sa optionNames()
 */

QStringList QCommandLineParser::unknownOptionNames() const
{
    d->checkParsed("unknownOptionNames");
    return d->unknownOptionNames;
}

/*!
    Displays the help information, and exits the application.
    This is automatically triggered by the --help option, but can also
    be used to display the help when the user is not invoking the
    application correctly.
    The exit code is set to \a exitCode. It should be set to 0 if the
    user requested to see the help, and to any other value in case of
    an error.

    \sa helpText()
*/
void QCommandLineParser::showHelp(int exitCode)
{
    fprintf(stdout, "%s", qPrintable(d->helpText()));
    ::exit(exitCode);
}

/*!
    Returns a string containing the complete help information.

    \sa showHelp()
*/
QString QCommandLineParser::helpText() const
{
    return d->helpText();
}

static QString wrapText(const QString &names, int longestOptionNameString, const QString &description)
{
    const QLatin1Char nl('\n');
    QString text = QString("  ") + names.leftJustified(longestOptionNameString) + QLatin1Char(' ');
    const int indent = text.length();
    int lineStart = 0;
    int lastBreakable = -1;
    const int max = 79 - indent;
    int x = 0;
    const int len = description.length();

    for (int i = 0; i < len; ++i) {
        ++x;
        const QChar c = description.at(i);
        if (c.isSpace())
            lastBreakable = i;

        int breakAt = -1;
        int nextLineStart = -1;
        if (x > max && lastBreakable != -1) {
            // time to break and we know where
            breakAt = lastBreakable;
            nextLineStart = lastBreakable + 1;
        } else if ((x > max - 1 && lastBreakable == -1) || i == len - 1) {
            // time to break but found nowhere [-> break here], or end of last line
            breakAt = i + 1;
            nextLineStart = breakAt;
        } else if (c == nl) {
            // forced break
            breakAt = i;
            nextLineStart = i + 1;
        }

        if (breakAt != -1) {
            const int numChars = breakAt - lineStart;
            //qDebug() << "breakAt=" << description.at(breakAt) << "breakAtSpace=" << breakAtSpace << lineStart << "to" << breakAt << description.mid(lineStart, numChars);
            if (lineStart > 0)
                text += QString(indent, QLatin1Char(' '));
            text += description.mid(lineStart, numChars) + nl;
            x = 0;
            lastBreakable = -1;
            lineStart = nextLineStart;
            if (lineStart < len && description.at(lineStart).isSpace())
                ++lineStart; // don't start a line with a space
            i = lineStart;
        }
    }

    return text;
}

QString QCommandLineParserPrivate::helpText() const
{
    const QLatin1Char nl('\n');
    QString text;
    const QString exeName = QCoreApplication::instance()->arguments().first();
    QString usage = exeName;
    if (!commandLineOptionList.isEmpty()) {
        usage += QLatin1Char(' ');
        usage += QCommandLineParser::tr("[options]");
    }
    Q_FOREACH (const PositionalArgumentDefinition &arg, positionalArgumentDefinitions) {
        usage += QLatin1Char(' ');
        usage += arg.syntax;
    }
    text += QCommandLineParser::tr("Usage: %1").arg(usage) + nl;
    if (!description.isEmpty())
       text += description + nl;
    text += nl;
    if (!commandLineOptionList.isEmpty())
        text += QCommandLineParser::tr("Options:") + nl;
    QStringList optionNameList;
    int longestOptionNameString = 0;
    Q_FOREACH (const QCommandLineOption &option, commandLineOptionList) {
        QStringList optionNames;
        Q_FOREACH (const QString &optionName, option.names()) {
            if (optionName.length() == 1)
                optionNames.append(QLatin1Char('-') + optionName);
            else
                optionNames.append(QString("--") + optionName);
        }
        QString optionNamesString = optionNames.join(", ");
        if (!option.valueName().isEmpty())
            optionNamesString += QString(" <") + option.valueName() + QLatin1Char('>');
        optionNameList.append(optionNamesString);
        longestOptionNameString = qMax(longestOptionNameString, optionNamesString.length());
    }
    ++longestOptionNameString;
    for (int i = 0; i < commandLineOptionList.count(); ++i) {
        const QCommandLineOption &option = commandLineOptionList.at(i);
        text += wrapText(optionNameList.at(i), longestOptionNameString, option.description());
    }
    if (!positionalArgumentDefinitions.isEmpty()) {
        if (!commandLineOptionList.isEmpty())
            text += nl;
        text += QCommandLineParser::tr("Arguments:") + nl;
        Q_FOREACH (const PositionalArgumentDefinition &arg, positionalArgumentDefinitions) {
            text += wrapText(arg.name, longestOptionNameString, arg.description);
        }
    }
    return text;
}
