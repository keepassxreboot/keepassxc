/*! \file
 *
 * MockNetworkAccessManager
 * https://gitlab.com/julrich/MockNetworkAccessManager
 *
 * \version 0.10.1
 * \author Jochen Ulrich <jochen.ulrich@t-online.de>
 * \copyright © 2018-2022 Jochen Ulrich. Licensed under MIT license (https://opensource.org/licenses/MIT)
 * except for the HttpStatus namespace which is licensed under Creative Commons CC0
 * (http://creativecommons.org/publicdomain/zero/1.0/).
 */
/*
Copyright © 2018-2022 Jochen Ulrich

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MOCKNETWORKACCESSMANAGER_HPP
#define MOCKNETWORKACCESSMANAGER_HPP

#include <QtGlobal>

#ifdef Q_CC_MSVC
#pragma warning( push, 0 )
#endif

#include <QtCore>
#include <QtNetwork>
#include <QAtomicInt>

#ifdef Q_CC_MSVC
#pragma warning( pop )
#endif

#include <algorithm>
#include <utility>
#include <vector>
#include <climits>
#include <type_traits>
#include <memory>

#if QT_VERSION >= QT_VERSION_CHECK( 6,0,0 )
	#if defined( QT_CORE5COMPAT_LIB )
		#include <QtCore5Compat>
	#endif
#endif // Qt >= 6.0.0

#if ( QT_VERSION < QT_VERSION_CHECK( 6,0,0 ) ) || ( defined( QT_FEATURE_textcodec ) && QT_FEATURE_textcodec == 1 )
/*! Defined if the QTextCodec is available.
 *
 * This is if %Qt version is below 6.0.0 or if the QtCore5Compat library is linked.
 */
#define MOCKNETWORKACCESSMANAGER_QT_HAS_TEXTCODEC
#endif

//! \cond QT_POLYFILL
#ifndef Q_NAMESPACE
#define Q_NAMESPACE
#endif

#ifndef Q_ENUM_NS
#define Q_ENUM_NS(x)
#endif

#ifndef Q_DECL_DEPRECATED_X
#define Q_DECL_DEPRECATED_X(x)
#endif
//! \endcond

#if QT_VERSION < QT_VERSION_CHECK( 5,14,0 )
namespace Qt
{
typedef QString::SplitBehavior SplitBehaviorFlags;
} // namespace Qt
#endif // Qt < 5.14.0

/*! Provides functions and classes to mock network replies in %Qt applications.
 */
namespace MockNetworkAccess
{

	Q_NAMESPACE

/*! Returns the logging category used by the library.
 *
 * The name of the category is `MockNetworkAccessManager`.
 * All messages logged by the library use this logging category.
 *
 * \return The QLoggingCategory of the MockNetworkAccessManager library.
 */
inline Q_LOGGING_CATEGORY( log, "MockNetworkAccessManager" )

/*! Behavior switches defining different behaviors for the classes of the Manager.
 * \sa page_behaviorFlags
 */
enum BehaviorFlag
{
	/*! Defines that a class behaves as expected according to the documentation and standards
	 * (RFCs etc.). This also means it should behave like most %Qt bugs being fixed
	 * (see \ref page_knownQtBugs for a list of exceptions).
	 * This flag cannot be combined with other BehaviorFlags.
	 * \sa \ref page_knownQtBugs
	 */
	Behavior_Expected = 0,

	/*! Defines that the MockReplies emits an `uploadProgress(0, 0)` signal after the download.
	 * There is QTBUG-44782 in QNetworkReply which causes it to emit an `uploadProgress(0, 0)` signal after
	 * the download of the reply has finished.
	 * \sa https://bugreports.qt.io/browse/QTBUG-44782
	 */
	Behavior_FinalUpload00Signal       = 1<<1,
	/*! Defines that the Manager does not automatically redirect on 308 "Permanent Redirect" responses.
	 * %Qt does not respect the 308 status code for automatic redirection up to %Qt 5.9.3 (was fixed with QTBUG-63075).
	 * \sa https://bugreports.qt.io/browse/QTBUG-63075
	 * \since 0.3.0
	 */
	Behavior_NoAutomatic308Redirect    = 1<<2,
	/*! Defines that the Manager follows all redirects with a GET request (except the initial request was a HEAD
	 * request in which case it follows with a HEAD request as well).
	 * %Qt up to 5.9.3 followed all redirected requests except HEAD requests with a GET request. QTBUG-63142 fixes this
	 * to not change the request method for 307 and 308 requests.
	 * \sa https://bugreports.qt.io/browse/QTBUG-63142
	 * \since 0.3.0
	 */
	Behavior_RedirectWithGet           = 1<<3,
	/*! Defines that the Manager assumes Latin-1 encoding for username and password for HTTP authentication.
	 * By default, the Manager uses the `charset` parameter of the authentication scheme and defaults to UTF-8 encoding.
	 */
	Behavior_HttpAuthLatin1Encoding    = 1<<4,
	/*! Defines that the Manager rewrites the request verbs OPTIONS and TRACE to GET when following redirects.
	 * [RFC 7231, Section 4.2.1](https://tools.ietf.org/html/rfc7231#section-4.2.1) defines the HTTP verbs OPTIONS and
	 * TRACE as "safe" request methods so it should be fine to use them when automatically following redirects for HTTP
	 * status codes 301 and 302. This behavior defines that the Manager should still redirect with a GET request in that
	 * case.
	 * \note Behavior_RedirectWithGet overrides this flag. So if Behavior_RedirectWithGet is set, this flag is ignored.
	 * \since 0.3.0
	 */
	Behavior_IgnoreSafeRedirectMethods = 1<<5,

	/*! Defines the behavior of %Qt 5.6.
	 */
	Behavior_Qt_5_6_0 = Behavior_FinalUpload00Signal
	                  | Behavior_NoAutomatic308Redirect
	                  | Behavior_RedirectWithGet
	                  | Behavior_HttpAuthLatin1Encoding
	                  | Behavior_IgnoreSafeRedirectMethods,
	/*! Defines the behavior of %Qt 5.2.
	 * \since 0.3.0
	 */
	Behavior_Qt_5_2_0 = Behavior_Qt_5_6_0
};
/*! QFlags type of \ref BehaviorFlag
 */
Q_DECLARE_FLAGS(BehaviorFlags, BehaviorFlag)
Q_ENUM_NS(BehaviorFlag)


// LCOV_EXCL_START
// @sonarcloud-exclude-start
/*! HTTP Status Codes - Qt Variant
 *
 * https://github.com/j-ulrich/http-status-codes-cpp
 *
 * \version 1.5.0
 * \author Jochen Ulrich <jochenulrich@t-online.de>
 * \copyright Licensed under Creative Commons CC0 (http://creativecommons.org/publicdomain/zero/1.0/)
 */
namespace HttpStatus{
#if(QT_VERSION>=QT_VERSION_CHECK(5,8,0))
Q_NAMESPACE
#endif
enum Code{Continue=100,SwitchingProtocols=101,Processing=102,EarlyHints=103,OK=200,Created=201,Accepted=202,NonAuthoritativeInformation=203,NoContent=204,ResetContent=205,PartialContent=206,MultiStatus=207,AlreadyReported=208,IMUsed=226,MultipleChoices=300,MovedPermanently=301,Found=302,SeeOther=303,NotModified=304,UseProxy=305,TemporaryRedirect=307,PermanentRedirect=308,BadRequest=400,Unauthorized=401,PaymentRequired=402,Forbidden=403,NotFound=404,MethodNotAllowed=405,NotAcceptable=406,ProxyAuthenticationRequired=407,RequestTimeout=408,Conflict=409,Gone=410,LengthRequired=411,PreconditionFailed=412,ContentTooLarge=413,PayloadTooLarge=413,URITooLong=414,UnsupportedMediaType=415,RangeNotSatisfiable=416,ExpectationFailed=417,ImATeapot=418,MisdirectedRequest=421,UnprocessableContent=422,UnprocessableEntity=422,Locked=423,FailedDependency=424,TooEarly=425,UpgradeRequired=426,PreconditionRequired=428,TooManyRequests=429,RequestHeaderFieldsTooLarge=431,UnavailableForLegalReasons=451,InternalServerError=500,NotImplemented=501,BadGateway=502,ServiceUnavailable=503,GatewayTimeout=504,HTTPVersionNotSupported=505,VariantAlsoNegotiates=506,InsufficientStorage=507,LoopDetected=508,NotExtended=510,NetworkAuthenticationRequired=511,xxx_max=1023};
#if(QT_VERSION>=QT_VERSION_CHECK(5,8,0))
Q_ENUM_NS(Code)
#endif
inline bool isInformational(int code){return(code>=100&&code<200);}inline bool isSuccessful(int code){return(code>=200&&code<300);}inline bool isRedirection(int code){return(code>=300&&code<400);}inline bool isClientError(int code){return(code>=400&&code<500);}inline bool isServerError(int code){return(code>=500&&code<600);}inline bool isError(int code){return(code>=400);}inline QString reasonPhrase(int code){switch(code){case 100:return QStringLiteral("Continue");case 101:return QStringLiteral("Switching Protocols");case 102:return QStringLiteral("Processing");case 103:return QStringLiteral("Early Hints");case 200:return QStringLiteral("OK");case 201:return QStringLiteral("Created");case 202:return QStringLiteral("Accepted");case 203:return QStringLiteral("Non-Authoritative Information");case 204:return QStringLiteral("No Content");case 205:return QStringLiteral("Reset Content");case 206:return QStringLiteral("Partial Content");case 207:return QStringLiteral("Multi-Status");case 208:return QStringLiteral("Already Reported");case 226:return QStringLiteral("IM Used");case 300:return QStringLiteral("Multiple Choices");case 301:return QStringLiteral("Moved Permanently");case 302:return QStringLiteral("Found");case 303:return QStringLiteral("See Other");case 304:return QStringLiteral("Not Modified");case 305:return QStringLiteral("Use Proxy");case 307:return QStringLiteral("Temporary Redirect");case 308:return QStringLiteral("Permanent Redirect");case 400:return QStringLiteral("Bad Request");case 401:return QStringLiteral("Unauthorized");case 402:return QStringLiteral("Payment Required");case 403:return QStringLiteral("Forbidden");case 404:return QStringLiteral("Not Found");case 405:return QStringLiteral("Method Not Allowed");case 406:return QStringLiteral("Not Acceptable");case 407:return QStringLiteral("Proxy Authentication Required");case 408:return QStringLiteral("Request Timeout");case 409:return QStringLiteral("Conflict");case 410:return QStringLiteral("Gone");case 411:return QStringLiteral("Length Required");case 412:return QStringLiteral("Precondition Failed");case 413:return QStringLiteral("Content Too Large");case 414:return QStringLiteral("URI Too Long");case 415:return QStringLiteral("Unsupported Media Type");case 416:return QStringLiteral("Range Not Satisfiable");case 417:return QStringLiteral("Expectation Failed");case 418:return QStringLiteral("I'm a teapot");case 421:return QStringLiteral("Misdirected Request");case 422:return QStringLiteral("Unprocessable Content");case 423:return QStringLiteral("Locked");case 424:return QStringLiteral("Failed Dependency");case 425:return QStringLiteral("Too Early");case 426:return QStringLiteral("Upgrade Required");case 428:return QStringLiteral("Precondition Required");case 429:return QStringLiteral("Too Many Requests");case 431:return QStringLiteral("Request Header Fields Too Large");case 451:return QStringLiteral("Unavailable For Legal Reasons");case 500:return QStringLiteral("Internal Server Error");case 501:return QStringLiteral("Not Implemented");case 502:return QStringLiteral("Bad Gateway");case 503:return QStringLiteral("Service Unavailable");case 504:return QStringLiteral("Gateway Timeout");case 505:return QStringLiteral("HTTP Version Not Supported");case 506:return QStringLiteral("Variant Also Negotiates");case 507:return QStringLiteral("Insufficient Storage");case 508:return QStringLiteral("Loop Detected");case 510:return QStringLiteral("Not Extended");case 511:return QStringLiteral("Network Authentication Required");default:return QString();}}inline int networkErrorToStatusCode(QNetworkReply::NetworkError error){switch(error){case QNetworkReply::AuthenticationRequiredError:return Unauthorized;case QNetworkReply::ContentAccessDenied:return Forbidden;case QNetworkReply::ContentNotFoundError:return NotFound;case QNetworkReply::ContentOperationNotPermittedError:return MethodNotAllowed;case QNetworkReply::ProxyAuthenticationRequiredError:return ProxyAuthenticationRequired;case QNetworkReply::NoError:return OK;case QNetworkReply::ProtocolInvalidOperationError:return BadRequest;case QNetworkReply::UnknownContentError:return BadRequest;
#if QT_VERSION>=QT_VERSION_CHECK(5,3,0)
case QNetworkReply::ContentConflictError:return Conflict;case QNetworkReply::ContentGoneError:return Gone;case QNetworkReply::InternalServerError:return InternalServerError;case QNetworkReply::OperationNotImplementedError:return NotImplemented;case QNetworkReply::ServiceUnavailableError:return ServiceUnavailable;case QNetworkReply::UnknownServerError:return InternalServerError;
#endif
default:return-1;}}inline QNetworkReply::NetworkError statusCodeToNetworkError(int code){if(!isError(code))return QNetworkReply::NoError;switch(code){case BadRequest:return QNetworkReply::ProtocolInvalidOperationError;case Unauthorized:return QNetworkReply::AuthenticationRequiredError;case Forbidden:return QNetworkReply::ContentAccessDenied;case NotFound:return QNetworkReply::ContentNotFoundError;case MethodNotAllowed:return QNetworkReply::ContentOperationNotPermittedError;case ProxyAuthenticationRequired:return QNetworkReply::ProxyAuthenticationRequiredError;case ImATeapot:return QNetworkReply::ProtocolInvalidOperationError;
#if QT_VERSION>=QT_VERSION_CHECK(5,3,0)
case Conflict:return QNetworkReply::ContentConflictError;case Gone:return QNetworkReply::ContentGoneError;case InternalServerError:return QNetworkReply::InternalServerError;case NotImplemented:return QNetworkReply::OperationNotImplementedError;case ServiceUnavailable:return QNetworkReply::ServiceUnavailableError;
#endif
default:break;}if(isClientError(code))return QNetworkReply::UnknownContentError;
#if QT_VERSION>=QT_VERSION_CHECK(5,3,0)
if(isServerError(code))return QNetworkReply::UnknownServerError;
#endif
return QNetworkReply::ProtocolFailure;}
} // namespace HttpStatus
// @sonarcloud-exclude-end
// LCOV_EXCL_STOP

/*! \internal Implementation details
 */
namespace detail
{

/*! \internal
 * Formats a pointer's address as string.
 * \param pointer The pointer.
 * \return A string representing the \p pointer's address.
 */
inline QString pointerToQString( const void* pointer )
{
	// From https://stackoverflow.com/a/16568641/490560
	const int bytesPerHexDigit = 2;
	const int hexBase = 16;
	return QString::fromLatin1( "0x%1" ).arg( reinterpret_cast<quintptr>( pointer ),
	                                          QT_POINTER_SIZE * bytesPerHexDigit, hexBase,
	                                          QChar::fromLatin1( '0' ) );
}

} // namespace detail


/*! Provides helper methods for tasks related to HTTP.
 *
 * \sa https://tools.ietf.org/html/rfc7230
 */
namespace HttpUtils
{
	/*! The default port of HTTP requests.
	 */
	const int HttpDefaultPort = 80;

	/*! The default port of HTTPS requests.
	 */
	const int HttpsDefaultPort = 443;

	/*! \return The scheme of the Hypertext Transfer Protocol (HTTP) in lower case characters.
	 */
	inline QString httpScheme()
	{
		const QString httpSchemeString = QStringLiteral( "http" );
		return httpSchemeString;
	}

	/*! \return The scheme of the Hypertext Transfer Protocol Secure (HTTPS) in lower case characters.
	 */
	inline QString httpsScheme()
	{
		const QString httpsSchemeString = QStringLiteral( "https" );
		return httpsSchemeString;
	}

	/*! \return The name of the Location header field.
	 */
	inline QByteArray locationHeader()
	{
		const QByteArray locationHeaderKey = QByteArrayLiteral( "Location" );
		return locationHeaderKey;
	}

	/*! \return The name of the WWW-Authenticate header field.
	 */
	inline QByteArray wwwAuthenticateHeader()
	{
		const QByteArray wwwAuthenticateHeaderKey = QByteArrayLiteral( "WWW-Authenticate" );
		return wwwAuthenticateHeaderKey;
	}

	/*! \return The name of the Proxy-Authenticate header field.
	 */
	inline QByteArray proxyAuthenticateHeader()
	{
		const QByteArray proxyAuthenticateHeaderKey = QByteArrayLiteral( "Proxy-Authenticate" );
		return proxyAuthenticateHeaderKey;
	}

	/*! \return The name of the Authorization header field.
	 */
	inline QByteArray authorizationHeader()
	{
		const QByteArray authorizationHeaderKey = QByteArrayLiteral( "Authorization" );
		return authorizationHeaderKey;
	}

	/*! \return The name of the Proxy-Authorization header field.
	 */
	inline QByteArray proxyAuthorizationHeader()
	{
		const QByteArray proxyAuthorizationHeaderKey = QByteArrayLiteral( "Proxy-Authorization" );
		return proxyAuthorizationHeaderKey;
	}

	/*! \return The regular expression pattern to match tokens according to RFC 7230 3.2.6.
	 */
	inline QString tokenPattern()
	{
		const QString token = QStringLiteral( "(?:[0-9a-zA-Z!#$%&'*+\\-.^_`|~]+)" );
		return token;
	}

	/*! \return The regular expression pattern to match token68 according to RFC 7235 2.1.
	 */
	inline QString token68Pattern()
	{
		const QString token68 = QStringLiteral( "(?:[0-9a-zA-Z\\-._~+\\/]+=*)" );
		return token68;
	}
	/*! \return The regular expression pattern to match successive linear whitespace according to RFC 7230 3.2.3.
	 */
	inline QString lwsPattern()
	{
		const QString lws = QStringLiteral( "(?:[ \t]+)" );
		return lws;
	}
	/*! \return The regular expression pattern to match obsolete line folding (obs-fold) according to RFC 7230 3.2.4.
	 */
	inline QString obsFoldPattern()
	{
		const QString obsFold = QStringLiteral( "(?:\r\n[ \t])" );
		return obsFold;
	}
	/*! Returns a version of a string with linear whitespace according to RFC 7230 3.2.3 removed from the
	 * beginning and end of the string.
	 *
	 * \param string The string whose leading and trailing linear whitespace should be removed.
	 * \return A copy of \p string with all horizontal tabs and spaces removed from the beginning and end of the
	 * string.
	 */
	inline QString trimmed(const QString& string)
	{
		const QRegularExpression leadingLwsRegEx( QStringLiteral( "^" ) + lwsPattern() + QStringLiteral( "+" ) );
		const QRegularExpression trailingLwsRegEx( lwsPattern() + QStringLiteral( "+$" ) );

		QString trimmed( string );

		const QRegularExpressionMatch leadingMatch = leadingLwsRegEx.match( trimmed );
		if ( leadingMatch.hasMatch() )
			trimmed.remove( 0, leadingMatch.capturedLength( 0 ) );

		const QRegularExpressionMatch trailingMatch = trailingLwsRegEx.match( trimmed );
		if ( trailingMatch.hasMatch() )
			trimmed.remove( trailingMatch.capturedStart( 0 ), trailingMatch.capturedLength( 0 ) );

		return trimmed;
	}
	/*! Returns a version of a string with obsolete line folding replaced with a space and whitespace trimmed,
	 * both according to RFC 7230.
	 *
	 * \param string The string which should be trimmed and whose obs-folding should be removed.
	 * \return A copy of \p string with all obsolete line foldings (RFC 7230 3.2.4) replaced with a space
	 * and afterwards, trimmed using trimmed().
	 *
	 * \sa trimmed()
	 */
	inline QString whiteSpaceCleaned( const QString& string )
	{
		const QRegularExpression obsFoldRegEx( obsFoldPattern() );
		QString cleaned( string );
		cleaned.replace( obsFoldRegEx, QLatin1String( " " ) );
		return trimmed( cleaned );
	}

	/*! Checks if a given string is a token according to RFC 7230 3.2.6.
	 *
	 * \param string The string to be checked to be a token.
	 * \return \c true if \p string is a valid token or \c false otherwise.
	 */
	inline bool isValidToken(const QString& string)
	{
		const QRegularExpression tokenRegEx( QStringLiteral( "^" ) + tokenPattern() + QStringLiteral( "$" ) );
		return tokenRegEx.match( string ).hasMatch();
	}

	/*! Checks if a character is a visible (printable) US ASCII character.
	 *
	 * @param character The character to be checked.
	 * @return \c true if \p character is a printable US ASCII character.
	 */
	inline bool isVCHAR( const char character )
	{
		const char FirstVCHAR = '\x21';
		const char LastVCHAR = '\x7E';

		return character >= FirstVCHAR && character <= LastVCHAR;
	}

	/*! Checks if a character is an "obs-text" character according to RFC 7230 3.2.6.
	 *
	 * @param character The character to be checked.
	 * @return \c true if \p character falls into the "obs-text" character range.
	 */
	inline bool isObsTextChar( const char character )
	{
		#if CHAR_MIN < 0
			// char is signed so all obs-text characters are negative
			return character < 0;
		#else
			const char FirstObsTextChar = '\x80';

			/* LastObsTextChar would be 0xFF which is the maximum value of char
			 * so there is no need to check if character is smaller.
			 */

			return character >= FirstObsTextChar;
		#endif
	}

	/*! Checks if a character is legal to occur in a header field according to RFC 7230 3.2.6.
	 *
	 * \param character The character to be checked.
	 * \return \c true if \p character is an allowed character for a header field value.
	 */
	inline bool isLegalHeaderCharacter(const char character)
	{
		return ( character == QChar::Tabulation
		      || character == QChar::Space
		      || isVCHAR( character )
		      || isObsTextChar( character ) );
	}

	/*! Checks if a string is a valid quoted-string according to RFC 7230 3.2.6.
	 *
	 * \param string The string to be tested. \p string is expected to *not* contain obsolete line folding (obs-fold).
	 * Use whiteSpaceCleaned() to ensure this.
	 * \return \c true if the \p string is a valid quoted-string.
	 */
	inline bool isValidQuotedString( const QString& string )
	{
		// String needs to contain at least the quotes
		const int minimumStringSize = 2;
		if ( string.size() < minimumStringSize )
			return false;

		// First character must be a quote
		if ( string.at( 0 ).toLatin1() != '"' )
			return false;

		unsigned int backslashCount = 0;
		const int backslashEscapeLength = 2;
		for ( int i = 1, stringContentEnd = string.size() - 1; i < stringContentEnd; ++i )
		{
			// Non-Latin-1 characters will be 0
			const char c = string.at( i ).toLatin1();

			// String must not contain illegal characters
			if ( Q_UNLIKELY( !isLegalHeaderCharacter( c ) ) )
				return false;

			if ( c == '\\' )
				++backslashCount;
			else
			{
				// Other quotes and obs-text must be escaped
				if ( ( c == '"' || isObsTextChar( c ) ) && ( backslashCount % backslashEscapeLength ) == 0 )
					return false;

				backslashCount = 0;
			}
		}

		// Last character must be a quote and it must not be escaped
		if ( string.at( string.size() - 1 ).toLatin1() != '"' || ( backslashCount % backslashEscapeLength ) != 0 )
			return false;

		return true;
	}

	/*! Converts a quoted-string according to RFC 7230 3.2.6 to it's unquoted version.
	 *
	 * \param quotedString The quoted string to be converted to "plain" text.
	 * \return A copy of \p quotedString with all quoted-pairs converted to the second character of the pair and the
	 * leading and trailing double quotes removed. If \p quotedString is not a valid quoted-string, a null
	 * QString() is returned.
	 */
	inline QString unquoteString( const QString& quotedString )
	{
		if ( !isValidQuotedString( quotedString ) )
			return QString();

		QString unquotedString( quotedString.mid( 1, quotedString.size()-2 ) );

		const QRegularExpression quotedPairRegEx( QStringLiteral( "\\\\." ) );
		QStack<QRegularExpressionMatch> quotedPairMatches;
		QRegularExpressionMatchIterator quotedPairIter = quotedPairRegEx.globalMatch( unquotedString );
		while ( quotedPairIter.hasNext() )
			quotedPairMatches.push( quotedPairIter.next() );

		while ( !quotedPairMatches.isEmpty() )
		{
			const QRegularExpressionMatch match = quotedPairMatches.pop();
			unquotedString.remove( match.capturedStart( 0 ), 1 );
		}

		return unquotedString;
	}

	/*! Converts a string to it's quoted version according to RFC 7230 3.2.6.
	 *
	 * \param unquotedString The "plain" text to be converted to a quoted-string.
	 * \return A copy of \p unquotedString surrounded with double quotes and all double quotes, backslashes
	 * and obs-text characters escaped. If the \p unquotedString contains any characters that are not allowed
	 * in a header field value, a null QString() is returned.
	 */
	inline QString quoteString( const QString& unquotedString )
	{
		QString escapedString;

		for ( int i = 0, unquotedStringSize = unquotedString.size(); i < unquotedStringSize; ++i )
		{
			// Non-Latin-1 characters will be 0
			const char c = unquotedString.at( i ).toLatin1();

			if ( Q_UNLIKELY( !isLegalHeaderCharacter( c ) ) )
				return QString();

			if ( c == '"' || c == '\\' || isObsTextChar( c ) )
				escapedString += QChar::fromLatin1( '\\' );
			escapedString += QChar::fromLatin1( c );
		}

		return QStringLiteral( "\"" ) + escapedString + QStringLiteral( "\"" );
	}

	/*! \internal Implementation details
	 */
	namespace detail
	{
		class CommaSeparatedListParser
		{
		public:
			CommaSeparatedListParser()
				: m_inString( false )
				, m_escaped( false )
			{}

			QStringList parse( const QString& commaSeparatedList )
			{
				QString::const_iterator iter = commaSeparatedList.cbegin();
				const QString::const_iterator end = commaSeparatedList.cend();
				for ( ; iter != end; ++iter )
				{
					processCharacter( *iter );
				}

				if ( !checkStateAfterParsing() )
					return QStringList();

				finalizeNextEntry();

				return m_split;
			}

		private:
			void processCharacter( QChar character )
			{
				if ( m_inString )
					processCharacterInString( character );
				else
					processCharacterOutsideString( character );
			}

			void processCharacterInString( QChar character )
			{
				if ( character == QChar::fromLatin1( '\\' ) )
					m_escaped = !m_escaped;
				else
				{
					if ( character == QChar::fromLatin1( '"' ) && !m_escaped )
						m_inString = false;
					m_escaped = false;
				}
				m_nextEntry += character;
			}

			void processCharacterOutsideString( QChar character )
			{
				if ( character == QChar::fromLatin1( ',' ) )
				{
					finalizeNextEntry();
				}
				else
				{
					if ( character == QChar::fromLatin1( '"' ) )
						m_inString = true;
					m_nextEntry += character;
				}
			}

			void finalizeNextEntry()
			{
				const QString trimmedEntry = trimmed( m_nextEntry );
				if ( !trimmedEntry.isEmpty() )
					m_split << trimmedEntry;
				m_nextEntry.clear();
			}

			bool checkStateAfterParsing() const
			{
				return !m_inString;
			}

		private:
			bool m_inString;
			bool m_escaped;
			QString m_nextEntry;
			QStringList m_split;
		};
	}

	/*! Splits a string containing a comma-separated list according to RFC 7230 section 7.
	 *
	 * \param commaSeparatedList A string containing a comma-separated list. The list can contain
	 * quoted strings and commas within quoted strings are not treated as list separators.
	 * \return QStringList consisting of the elements of \p commaSeparatedList.
	 * Empty elements in \p commaSeparatedList are omitted.
	 */
	inline QStringList splitCommaSeparatedList( const QString& commaSeparatedList )
	{
		detail::CommaSeparatedListParser parser;
		return parser.parse( commaSeparatedList );
	}


	/*! Namespace for HTTP authentication related classes.
	 *
	 * \sa https://tools.ietf.org/html/rfc7235
	 */
	namespace Authentication
	{
		/*! Returns the authentication scope of a URL according to RFC 7617 2.2.
		 *
		 * \param url The URL whose authentication scope should be returned.
		 * \return A URL which has the same scheme, host, port and path up to the last slash
		 * as \p url.
		 */
		inline QUrl authenticationScopeForUrl( const QUrl& url )
		{
			QUrl authScope;
			authScope.setScheme( url.scheme() );
			authScope.setHost( url.host() );
			authScope.setPort( url.port() );
			const QFileInfo urlPath( url.path( QUrl::FullyEncoded ) );
			QString path = urlPath.path(); // Remove the part after the last slash using QFileInfo::path()
			if ( path.isEmpty() || path == QLatin1String( "." ) )
				path = QLatin1String( "/" );
			else if ( !path.endsWith( QChar::fromLatin1( '/' ) ) )
				path += QChar::fromLatin1( '/' );
			authScope.setPath( path );
			return authScope;
		}

		/*! \internal Implementation details
		 */
		namespace detail
		{

			inline QByteArray authorizationHeaderKey()
			{
				const QByteArray authHeader = QByteArrayLiteral( "Authorization" );
				return authHeader;
			}

			inline QString authParamPattern()
			{
				const QString authParam =
					QStringLiteral( "(?:(?<authParamName>" ) + HttpUtils::tokenPattern() + QStringLiteral( ")" )
					+ HttpUtils::lwsPattern() + QStringLiteral( "?" )
					+ QStringLiteral( "=" ) + HttpUtils::lwsPattern() + QStringLiteral( "?" )
					+ QStringLiteral( "(?<authParamValue>" ) + HttpUtils::tokenPattern()
					+ QStringLiteral( "|\".*\"))" );
				return authParam;
			}

		} // namespace detail


		/*! Represents an HTTP authentication challenge according to RFC 7235.
		 */
		class Challenge
		{
		public:
			/*! QSharedPointer to a Challenge object.
			 */
			typedef QSharedPointer< Challenge > Ptr;

			/*! Defines the supported HTTP authentication schemes.
			 */
			enum AuthenticationScheme
			{
				/* WARNING: The numerical value defines the preference when multiple
				 * challenges are provided by the server.
				 * The lower the numerical value, the lesser the preference.
				 * So give stronger methods higher values.
				 * See strengthGreater()
				 */
				BasicAuthenticationScheme   = 100, //!< HTTP Basic authentication according to RFC 7617
				UnknownAuthenticationScheme = -1   //!< Unknown authentication scheme
			};

			/*! Creates an invalid authentication challenge.
			 *
			 * Sets the behaviorFlags() to Behavior_Expected.
			 */
			Challenge()
			{
				setBehaviorFlags(Behavior_Expected);
			}
			/*! Enforces a virtual destructor.
			 */
			virtual ~Challenge() {}

			/*! \return The authentication scheme of this Challenge.
			 */
			virtual AuthenticationScheme scheme() const = 0;

			/*! Checks if the Challenge is valid, meaning it contains all
			 * parameters required for the authentication scheme.
			 *
			 * \return \c true if the Challenge is valid.
			 */
			virtual bool isValid() const = 0;
			/*! \return The parameters of the Challenge as a QVariantMap.
			 */
			virtual QVariantMap parameters() const = 0;
			/*! \return The value of an Authenticate header representing this Challenge.
			 */
			virtual QByteArray authenticateHeader() const = 0;
			/*! Compares the cryptographic strength of this Challenge with another
			 * Challenge.
			 *
			 * \param other The Challenge to compare against.
			 * \return \c true if this Challenge is considered cryptographically
			 * stronger than \p other. If they are equal or if \p other is stronger,
			 * \c false is returned.
			 */
			virtual bool strengthGreater(const Challenge::Ptr& other) { return this->scheme() > other->scheme(); }

			/*! Tunes the behavior of this Challenge.
			 *
			 * \param behaviorFlags Combination of BehaviorFlags to define some details of this Challenge's behavior.
			 * \note Only certain BehaviorFlags have an effect on a Challenge.
			 * \sa BehaviorFlag
			 */
			void setBehaviorFlags(BehaviorFlags behaviorFlags)
			{
				m_behaviorFlags = behaviorFlags;
			}

			/*! \return The BehaviorFlags currently active for this Challenge.
			 */
			BehaviorFlags behaviorFlags() const { return m_behaviorFlags; }

			/*! \return The realm of this Challenge according to RFC 7235 2.2.
			 */
			QString realm() const { return m_realm; }

			/*! \return The name of the realm parameter. Also used as key in QVariantMaps.
			 */
			static QString realmKey() { return QStringLiteral( "realm" ); }

			/*! Adds an authorization header for this Challenge to a given request.
			 *
			 * \param request The QNetworkRequest to which the authorization header will be added.
			 * \param operation The HTTP verb.
			 * \param body The message body of the request.
			 * \param authenticator The QAuthenticator providing the credentials to be used for the
			 * authorization.
			 */
			void addAuthorization( QNetworkRequest& request, QNetworkAccessManager::Operation operation,
			                       const QByteArray& body, const QAuthenticator& authenticator )
			{
				const QByteArray authHeaderValue = authorizationHeaderValue( request, operation, body, authenticator );
				request.setRawHeader( detail::authorizationHeaderKey(), authHeaderValue );
			}

			/*! Verifies if a given request contains a valid authorization for this Challenge.
			 *
			 * \param request The request which requests authorization.
			 * \param authenticator The QAuthenticator providing a set of valid credentials.
			 * \return \c true if the \p request contains a valid authorization header matching
			 * this Challenge and the credentials provided by the \p authenticator.
			 * Note that for certain authentication schemes, this method might always return \c false if this Challenge
			 * is invalid (see isValid()).
			 */
			virtual bool verifyAuthorization(const QNetworkRequest& request, const QAuthenticator& authenticator) = 0;

			/*! Implements a "lesser" comparison based on the cryptographic strength of a Challenge.
			 */
			struct StrengthCompare
			{
				/*! Implements the lesser comparison.
				 *
				 * \param left The left-hand side Challenge of the comparison.
				 * \param right The right-hand side Challenge of the comparison.
				 * \return \c true if \p left < \p right regarding the strength of the algorithm
				 * used by the challenges. Otherwise \c false.
				 */
				bool operator() ( const Challenge::Ptr& left, const Challenge::Ptr& right ) const
				{
					return right->strengthGreater(left);
				}
			};

		protected:

			/*! Generates a new authorization header value for this Challenge.
			 *
			 * \note This method is non-const because an authentication scheme might need
			 * to remember parameters from the authorizations it gave (like the \c cnonce in the Digest scheme).
			 *
			 * \param request The request for with the authorization header should be generated.
			 * \param operation The HTTP verb of the request.
			 * \param body The message body of the request.
			 * \param authenticator The QAuthenticator providing the credentials to be used to generate the
			 * authorization header.
			 * \return The value of the Authorization header to request authorization for this Challenge using the
			 * credentials provided by the \p authenticator.
			 *
			 * \sa addAuthorization()
			 */
			virtual QByteArray authorizationHeaderValue(const QNetworkRequest& request,
			                                            QNetworkAccessManager::Operation operation,
			                                            const QByteArray& body,
			                                            const QAuthenticator& authenticator) = 0;

			/*! Splits a list of authentication parameters according to RFC 7235 2.1. into a QVariantMap.
			 *
			 * \param authParams The list of name=value strings.
			 * \param[out] paramsValid If not \c NULL, the value of this boolean will be set to \c false if one of the
			 * parameters in \p authParams was malformed or to \c true otherwise. If \p paramsValid is \c NULL, it is
			 * ignored.
			 * \return A QVariantMap mapping the names of the authentication parameters to their values.
			 * The names of the authentication parameters are converted to lower case.
			 * The values are *not* unquoted in case they are quoted strings.
			 */
			static QVariantMap stringParamListToMap( const QStringList& authParams, bool *paramsValid = Q_NULLPTR )
			{
				QVariantMap result;

				QStringList::const_iterator paramIter = authParams.cbegin();
				const QStringList::const_iterator paramsEnd = authParams.cend();
				const QRegularExpression authParamRegEx( detail::authParamPattern() );

				for ( ; paramIter != paramsEnd; ++paramIter )
				{
					const QRegularExpressionMatch authParamMatch = authParamRegEx.match( *paramIter );
					if ( !authParamMatch.hasMatch() )
					{
						qCWarning( log ) << "Invalid authentication header: malformed auth-param:"
						                 << *paramIter;
						if ( paramsValid )
							*paramsValid = false;
						return QVariantMap();
					}
					const QString authParamName = authParamMatch.captured( QStringLiteral( "authParamName" ) ).toLower();
					const QString authParamValue = authParamMatch.captured( QStringLiteral( "authParamValue" ) );

					if ( result.contains( authParamName ) )
						qCWarning( log ) << "Invalid authentication header: auth-param occurred multiple times:"
						                 << authParamName;

					result.insert( authParamName, authParamValue );
				}

				if (paramsValid)
					*paramsValid = true;
				return result;
			}

			/*! Sets the realm of this Challenge.
			 *
			 * The base class does not use the realm. It just provides the property for convenience.
			 * So derived classes are free to use the realm as they need to.
			 *
			 * \param realm The realm.
			 * \sa realm()
			 */
			void setRealm( const QString& realm )
			{
				m_realm = realm;
			}

		private:
			QString m_realm;
			BehaviorFlags m_behaviorFlags;
		};

		/*! HTTP Basic authentication scheme according to RFC 7617.
		 *
		 * \sa https://tools.ietf.org/html/rfc7617
		 */
		class Basic : public Challenge
		{
		public:
			/*! Creates a Basic authentication Challenge with parameters as a QStringList.
			 *
			 * \param authParams The parameters of the challenge as a list of name=value strings.
			 */
			explicit Basic(const QStringList& authParams)
				: Challenge()
			{
				bool paramsValid = false;
				const QVariantMap authParamsMap = stringParamListToMap(authParams, &paramsValid);
				if (paramsValid)
					readParameters(authParamsMap);
			}

			/*! Creates a Basic authentication Challenge with parameters a QVariantMap.
			 *
			 * \param authParams The parameters of the challenge as a map.
			 */
			explicit Basic(const QVariantMap& authParams)
				: Challenge()
			{
				readParameters(authParams);
			}

			/*! Creates a Basic authentication Challenge with the given realm.
			 *
			 * \param realm The realm.
			 */
			explicit Basic(const QString& realm)
				: Challenge()
			{
				QVariantMap params;
				params.insert(realmKey(), realm);
				readParameters(params);
			}


			/*! \return Challenge::BasicAuthenticationScheme.
			 */
			virtual AuthenticationScheme scheme() const Q_DECL_OVERRIDE { return BasicAuthenticationScheme; }

			/*! \return The identifier string of the Basic authentication scheme.
			 */
			static QByteArray schemeString() { return "Basic"; }
			/*! \return The name of the charset parameter. Also used as key in QVariantMaps.
			 */
			static QString charsetKey() { return QStringLiteral( "charset" ); }

			/*! \return \c true if the realm parameter is defined. Note that the realm can still
			 * be empty (`""`).
			 */
			virtual bool isValid() const Q_DECL_OVERRIDE { return !realm().isNull(); }

			/*! \return A map containing the realm and charset parameters (if given).
			 * \sa realmKey(), charsetKey().
			 */
			virtual QVariantMap parameters() const Q_DECL_OVERRIDE
			{
				QVariantMap params;
				params[realmKey()] = realm();
				if (!m_charset.isEmpty())
					params[charsetKey()] = m_charset;
				return params;
			}
			/*! \copydoc Challenge::authenticateHeader()
			 */
			virtual QByteArray authenticateHeader() const Q_DECL_OVERRIDE
			{
				if ( !isValid() )
					return QByteArray();

				QByteArray result = schemeString() + " "
				                    + realmKey().toLatin1() + "=" + quoteString( realm() ).toLatin1();
				if ( !m_charset.isEmpty() )
					result += ", " + charsetKey().toLatin1() + "=" + quoteString( m_charset ).toLatin1();
				return result;
			}

			/*! \copydoc Challenge::verifyAuthorization(const QNetworkRequest&, const QAuthenticator&)
			 */
			virtual bool verifyAuthorization( const QNetworkRequest& request,
			                                  const QAuthenticator& authenticator ) Q_DECL_OVERRIDE
			{
				/* Since the authorization header of the Basic scheme is very simple, we can simply compare
				 * the textual representations.
				 * Additionally, we can verify the authorization even if this challenge is invalid.
				 */
				const QByteArray reqAuth = request.rawHeader( detail::authorizationHeaderKey() );
				const QByteArray challengeAuth = this->authorizationHeaderValue( QNetworkRequest(),
				                                                                 QNetworkAccessManager::GetOperation,
				                                                                 QByteArray(), authenticator );
				return reqAuth == challengeAuth;
			}


		protected:
			/*! \copydoc Challenge::authorizationHeaderValue()
			 */
			virtual QByteArray authorizationHeaderValue( const QNetworkRequest& request,
			                                             QNetworkAccessManager::Operation operation,
			                                             const QByteArray& body,
			                                             const QAuthenticator& authenticator ) Q_DECL_OVERRIDE
			{
				Q_UNUSED( request )
				Q_UNUSED( operation )
				Q_UNUSED( body )

				QByteArray userName;
				QByteArray password;
				if ( behaviorFlags().testFlag( Behavior_HttpAuthLatin1Encoding ) )
				{
					userName = authenticator.user().toLatin1();
					password = authenticator.password().toLatin1();
				}
				else
				{
					/* No need to check m_charset since UTF-8 is the only allowed encoding at the moment and
					 * we use UTF-8 by default anyway (so even if charset was not specified)
					 */
					userName = authenticator.user().normalized( QString::NormalizationForm_C ).toUtf8();
					password = authenticator.password().normalized( QString::NormalizationForm_C ).toUtf8();
				}

				return schemeString() + " " + ( userName + ":" + password ).toBase64();
			}

		private:
			void readParameters( const QVariantMap& params )
			{
				if ( !params.contains( realmKey() ) )
				{
					setRealm( QString() );
					qCWarning( log ) << "Invalid authentication header: Missing required parameter: \"realm\"";
					return;
				}

				// Realm
				const QString realmValue = params.value( realmKey() ).toString();
				const QString realm = HttpUtils::isValidToken( realmValue ) ? realmValue
				                                                            : HttpUtils::unquoteString( realmValue );
				if ( realm.isNull() )
				{
					qCWarning( log ) << "Invalid authentication header: Missing value for parameter: \"realm\"";
					return;
				}
				setRealm( realm );

				// Charset
				if ( params.contains( charsetKey() ) )
				{
					const QString charsetValue = params.value( charsetKey() ).toString();
					const QString charset = ( HttpUtils::isValidToken( charsetValue)
					                          ? charsetValue
					                          : HttpUtils::unquoteString(charsetValue) ).toLower();
					m_charset = charset;
				}
			}

			QString m_charset;

		};

		/*! \internal Implementation details
		 */
		namespace detail
		{
			inline Challenge::Ptr parseAuthenticateChallenge( const QStringList& challengeParts, const QUrl& )
			{
				const QString& challengeStart = challengeParts.at( 0 );
				const int schemeSeparatorIndex = challengeStart.indexOf( QChar::fromLatin1( ' ' ) );
				const QString authSchemeLower = HttpUtils::trimmed( challengeStart.left( schemeSeparatorIndex ) )
				                                .toLower();
				const QString firstAuthParam = ( schemeSeparatorIndex > 0 )
				                               ? HttpUtils::trimmed( challengeStart.mid( schemeSeparatorIndex + 1 ) )
				                               : QString();

				// Get the first parameter of the challenge
				QStringList authParams;
				if ( !firstAuthParam.isEmpty() )
					authParams << firstAuthParam;
				// Append further parameters of the challenge
				if ( challengeParts.size() > 1 )
					authParams << challengeParts.mid( 1 );

				const QString basicAuthSchemeLower = QString::fromLatin1( Basic::schemeString() ).toLower();
				if ( authSchemeLower == basicAuthSchemeLower )
					return Challenge::Ptr( new Basic( authParams ) );

				qCWarning( log ) << "Unsupported authentication scheme:" << authSchemeLower;
				return Challenge::Ptr();
			}

			inline QVector<QStringList> splitAuthenticateHeaderIntoChallengeParts( const QString& headerValue )
			{
				QVector<QStringList> result;

				const QStringList headerSplit = HttpUtils::splitCommaSeparatedList( headerValue );

				const QRegularExpression challengeStartRegEx( QStringLiteral( "^" ) + HttpUtils::tokenPattern()
					+ QStringLiteral( "(?:" ) + HttpUtils::lwsPattern()
					+ QStringLiteral( "(?:" ) + HttpUtils::token68Pattern() + QStringLiteral( "|" )
					+ detail::authParamPattern() + QStringLiteral( "))?" ) );

				QVector<QPair<int, int> > challengeIndexes;
				int challengeStartIndex = headerSplit.indexOf( challengeStartRegEx );
				if ( challengeStartIndex < 0 )
				{
					qCWarning( log ) << "Invalid authentication header: expected start of authentication challenge";
					return result;
				}
				while ( challengeStartIndex != -1 )
				{
					const int nextChallengeStartIndex = headerSplit.indexOf( challengeStartRegEx, challengeStartIndex + 1 );
					challengeIndexes << ::qMakePair( challengeStartIndex, nextChallengeStartIndex );
					challengeStartIndex = nextChallengeStartIndex;
				}

				QVector<QPair<int, int> >::const_iterator challengeIndexIter = challengeIndexes.cbegin();
				const QVector<QPair<int, int> >::const_iterator challengeIndexesEnd = challengeIndexes.cend();

				for ( ; challengeIndexIter != challengeIndexesEnd; ++challengeIndexIter )
				{
					const int challengePartCount = ( challengeIndexIter->second == -1 )
					                               ? ( headerSplit.size() - challengeIndexIter->first )
					                               : ( challengeIndexIter->second - challengeIndexIter->first );
					const QStringList challengeParts = headerSplit.mid( challengeIndexIter->first, challengePartCount );

					result << challengeParts;
				}

				return result;
			}

			inline QVector<Challenge::Ptr> parseAuthenticateHeader( const QString& headerValue,
			                                                        const QUrl& requestingUrl )
			{
				QVector<Challenge::Ptr> result;

				const QVector<QStringList> challenges = splitAuthenticateHeaderIntoChallengeParts( headerValue );

				QVector<QStringList>::const_iterator challengeIter = challenges.cbegin();
				const QVector<QStringList>::const_iterator challengesEnd = challenges.cend();

				for ( ; challengeIter != challengesEnd; ++challengeIter )
				{
					const Challenge::Ptr authChallenge = parseAuthenticateChallenge( *challengeIter, requestingUrl );

					if ( authChallenge && authChallenge->isValid() )
						result << authChallenge;
				}

				return result;
			}


			inline QVector< Challenge::Ptr > parseAuthenticateHeaders( const QNetworkReply *reply )
			{
				const QByteArray wwwAuthenticateHeaderLower = wwwAuthenticateHeader().toLower();
				QVector< Challenge::Ptr > authChallenges;
				const QUrl requestingUrl = reply->url();

				const QList< QByteArray > rawHeaderList = reply->rawHeaderList();
				QList< QByteArray >::const_iterator headerIter = rawHeaderList.cbegin();
				const QList< QByteArray >::const_iterator headerEnd = rawHeaderList.cend();
				for ( ; headerIter != headerEnd; ++headerIter )
				{
					if ( headerIter->toLower() == wwwAuthenticateHeaderLower )
					{
						const QString headerValue = HttpUtils::whiteSpaceCleaned(
							QString::fromLatin1( reply->rawHeader( *headerIter ) ) );
						if ( headerValue.isEmpty() )
							continue;

						authChallenges << parseAuthenticateHeader( headerValue, requestingUrl );
					}
				}
				return authChallenges;
			}

		} // namespace detail

		/*! Extracts all authentication challenges from a QNetworkReply.
		 *
		 * \param reply The reply object potentially containing authentication challenges.
		 * \return A vector of Challenge::Ptrs. The vector can be empty if \p reply did not
		 * contain any authentication challenges.
		 */
		inline QVector<Challenge::Ptr> getAuthenticationChallenges( const QNetworkReply* reply )
		{
			const HttpStatus::Code statusCode = static_cast<HttpStatus::Code>(
				reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt()
			);
			switch ( statusCode )
			{
			case HttpStatus::Unauthorized:
				return detail::parseAuthenticateHeaders( reply );

			case HttpStatus::ProxyAuthenticationRequired:
				// TODO: Implement proxy authentication
				qCWarning( log ) << "Proxy authentication is not supported at the moment";
				break;

			// LCOV_EXCL_START
			default:
				Q_ASSERT_X( false, Q_FUNC_INFO, "MockNetworkAccessManager: Internal error: trying to authenticate"
				            "request which doesn't require authentication" );
				break;
			// LCOV_EXCL_STOP
			}

			return QVector<Challenge::Ptr>();
		}

	} // namespace Authentication

} // namespace HttpUtils

/*! Provides helper methods for tasks related to FTP.
 *
 * \since 0.6.0
 */
namespace FtpUtils
{

	/*! The default port of FTP requests.
	 */
	const int FtpDefaultPort = 21;

	/*! \return The scheme of the File Transfer Protocol (FTP) in lower case characters.
	 * \since 0.6.0
	 */
	inline QString ftpScheme()
	{
		const QString ftpSchemeString = QStringLiteral( "ftp" );
		return ftpSchemeString;
	}

	/*! \return The scheme of the File Transfer Protocol over SSL (FTPS) in lower case characters.
	 * \since 0.6.0
	 */
	inline QString ftpsScheme()
	{
		const QString ftpsSchemeString = QStringLiteral( "ftps" );
		return ftpsSchemeString;
	}

} // namespace FtpUtils



/*! Provides helper methods for tasks related to data: URLs.
 *
 * \since 0.9.0
 */
namespace DataUrlUtils
{
	/*! \return The scheme of data: URLs in lower case characters.
	 * \since 0.9.0
	 */
	inline QString dataScheme()
	{
		const QString dataSchemeString = QStringLiteral( "data" );
		return dataSchemeString;
	}
}


/*! Provides helper methods for tasks related to file: and qrc: URLs.
 *
 * \since 0.9.0
 */
namespace FileUtils
{
	/*! \return The scheme of file: URLs in lower case characters.
	 * \since 0.9.0
	 */
	inline QString fileScheme()
	{
		const QString fileSchemeString = QStringLiteral( "file" );
		return fileSchemeString;
	}

	/*! \return The scheme of qrc: URLs in lower case characters.
	 * \since 0.9.0
	 */
	inline QString qrcScheme()
	{
		const QString qrcSchemeString = QStringLiteral( "qrc" );
		return qrcSchemeString;
	}

	#if defined( Q_OS_ANDROID )
		inline QString assetsScheme()
		{
			const QString assetsSchemeString = QStringLiteral( "assets" );
			return assetsSchemeString;
		}
	#endif

	/*! Checks if a scheme behaves like the file scheme.
	 * \param scheme The scheme to be checked to behave like the file scheme.
	 * \return \c true if the \p url has a `file:`, `qrc:` or on Android `assets:` scheme. \c false otherwise.
	 */
	inline bool isFileLikeScheme( const QString& scheme )
	{
		#if defined( Q_OS_ANDROID )
			if( scheme == assetsScheme() )
				return true;
		#endif
		return scheme == fileScheme() || scheme == qrcScheme();
	}

	/*! Checks if a URL has a file-like scheme.
	 * \param url The URL to be checked for a file-like scheme.
	 * \return \c true if the \p url has a file: or qrc: scheme. \c false otherwise.
	 */
	inline bool isFileLikeScheme( const QUrl& url )
	{
		return isFileLikeScheme( url.scheme() );
	}

}


/*! Represents a version number.
 * A version number is a sequence of (dot separated) unsigned integers potentially followed by a suffix.
 *
 * \since 0.3.0
 */
struct VersionNumber
{
	/*! The container type holding the version segments.
	 * \sa segments
	 */
	typedef std::vector<unsigned int> SegmentVector;

	/*! The numeric segments that make up the version number.
	 */
	SegmentVector segments;
	/*! The non-numeric suffix of the version number.
	 */
	QString suffix;

	/*! \return `"."` which is the string separating the version segments in the string representation of the version
	 * number.
	 */
	static QString segmentSeparator()
	{
		const QString separator = QStringLiteral( "." );
		return separator;
	}

	/*! Creates an empty VersionNumber.
	 */
	VersionNumber()
	{
	}

	/*! Creates a VersionNumber from three segments.
	 * \param major The major version number.
	 * \param minor The minor version number.
	 * \param patch The patch version number.
	 * \param suffix An	optional version suffix.
	 */
	explicit VersionNumber( unsigned int major, unsigned int minor, unsigned int patch,
	                        const QString& suffix = QString() )
	{
		segments.push_back( major );
		segments.push_back( minor );
		segments.push_back( patch );
		this->suffix = suffix;
	}

	/*! Creates a VersionNumber from a string representation.
	 * \param versionStr The string representing the version number.
	 * \return A VersionNumber object corresponding to the \p versionStr or an
	 * empty VersionNumber object if the \p versionStr could not be parsed.
	 */
	static VersionNumber fromString(const QString& versionStr)
	{
		VersionNumber version;
		const QStringList split = versionStr.split(segmentSeparator());

		version.segments.reserve(static_cast<SegmentVector::size_type>(split.size()));

		bool converted = true;
		QStringList::const_iterator iter = split.cbegin();
		const QStringList::const_iterator splitEnd = split.cend();
		for ( ; iter != splitEnd; ++iter )
		{
			const unsigned int number = iter->toUInt(&converted);
			if (!converted)
				break;
			version.segments.push_back(number);
		}

		if (!converted)
		{
			// There is a suffix
			const QString lastSegment = *iter;
			const QRegularExpression digitRegEx( QStringLiteral( "^\\d+" ) );
			const QRegularExpressionMatch match = digitRegEx.match(lastSegment);
			if (match.hasMatch())
				version.segments.push_back(match.captured().toUInt());
			version.suffix = lastSegment.mid(match.capturedLength());
		}

		return version;
	}

	/*! \return The string representation of this version number.
	 */
	QString toString() const
	{
		QString result;
		const SegmentVector& segs = segments;
		SegmentVector::const_iterator segIter = segs.begin();
		const SegmentVector::const_iterator segsEnd = segs.end();
		for ( ; segIter != segsEnd; ++segIter )
			result += QString::number(*segIter) + segmentSeparator();
		result.chop(segmentSeparator().size());
		result += suffix;
		return result;
	}

	/*! Compares two VersionNumbers for equality.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left and \p right represent the same version number.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator==( const VersionNumber& left, const VersionNumber& right )
	{
		if ( &left == &right )
			return true;

		SegmentVector leftSegments = left.segments;
		SegmentVector rightSegments = right.segments;

		const SegmentVector::size_type maxSize = std::max( leftSegments.size(), rightSegments.size() );
		leftSegments.resize( maxSize );
		rightSegments.resize( maxSize );

		return leftSegments == rightSegments && left.suffix == right.suffix;
	}

	/*! Compares two VersionNumbers for inequality.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left and \p right represent different version numbers.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator!=(const VersionNumber& left, const VersionNumber& right)
	{
		return !( left == right );
	}

	/*! Compares if a VersionNumber is lesser than another VersionNumber.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left is lesser than \p right.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator<(const VersionNumber& left, const VersionNumber& right)
	{
		std::vector<unsigned int>::const_iterator leftIter = left.segments.begin();
		const std::vector<unsigned int>::const_iterator leftEnd = left.segments.end();
		std::vector<unsigned int>::const_iterator rightIter = right.segments.begin();
		const std::vector<unsigned int>::const_iterator rightEnd = right.segments.end();

		while ( leftIter != leftEnd || rightIter != rightEnd )
		{
			const unsigned int leftPart = (leftIter != leftEnd) ? *leftIter : 0;
			const unsigned int rightPart = (rightIter != rightEnd) ? *rightIter : 0;

			if (leftPart < rightPart)
				return true;
			if (leftPart > rightPart)
				return false;

			if( leftIter != leftEnd )
				++leftIter;
			if( rightIter != rightEnd )
				++rightIter;
		}

		if (left.suffix.isEmpty() && !right.suffix.isEmpty())
			return false;
		if (!left.suffix.isEmpty() && right.suffix.isEmpty())
			return true;
		return left.suffix < right.suffix;
	}

	/*! Compares if a VersionNumber is greater than another VersionNumber.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left is greater than \p right.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator>(const VersionNumber& left, const VersionNumber& right)
	{
		std::vector<unsigned int>::const_iterator leftIter = left.segments.begin();
		const std::vector<unsigned int>::const_iterator leftEnd = left.segments.end();
		std::vector<unsigned int>::const_iterator rightIter = right.segments.begin();
		const std::vector<unsigned int>::const_iterator rightEnd = right.segments.end();

		while ( leftIter != leftEnd || rightIter != rightEnd )
		{
			const unsigned int leftPart = (leftIter != leftEnd)? *leftIter : 0;
			const unsigned int rightPart = (rightIter != rightEnd)? *rightIter : 0;

			if (leftPart > rightPart)
				return true;
			if (leftPart < rightPart)
				return false;

			if( leftIter != leftEnd )
				++leftIter;
			if( rightIter != rightEnd )
				++rightIter;
		}

		if (left.suffix.isEmpty() && !right.suffix.isEmpty())
			return true;
		if (!left.suffix.isEmpty() && right.suffix.isEmpty())
			return false;
		return left.suffix > right.suffix;
	}

	/*! Compares if a VersionNumber is greater than or equal to another VersionNumber.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left is greater than or equal to \p right.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator>=(const VersionNumber& left, const VersionNumber& right)
	{
		return !(left < right);
	}

	/*! Compares if a VersionNumber is lesser than or equal to another VersionNumber.
	 * \param left One VersionNumber.
	 * \param right Another VersionNumber.
	 * \return \c true if \p left is lesser than or equal to \p right.
	 * \note Missing parts in a VersionNumber are interpreted as 0.
	 */
	friend bool operator<=(const VersionNumber& left, const VersionNumber& right)
	{
		return !(left > right);
	}
};

/*! Wrapper class providing a common interface for string/text decoding.
 *
 * This class is an implementation of the bridge pattern combined with the adapter
 * pattern (wrapper). Its implementation is either realized by a QTextCodec or
 * by a QStringDecoder. Which implementation is used depends on the availability.
 * If both are available, QTextCodec is used unless the StringDecoder is
 * constructed with a QStringDecoder.
 *
 * This class mainly exists to provide compatibility for both %Qt 5 and %Qt 6 since
 * the QTextCodec class was deprecated in %Qt 6.
 *
 * \warning A StringDecoder must be valid to be used. Trying to decode with an invalid
 * decoder might result in undefined behavior. See isValid().
 *
 * \since 0.5.0
 */
class StringDecoder
{
public:
	/*! Creates a StringDecoder with an optional codec.
	 *
	 * \param codec The name of the codec which this StringDecoder should decode.
	 * If \p codec is empty or unknown to the implementation, the StringDecoder will
	 * be invalid.
	 *
	 * \sa isValid()
	 * \sa setCodec()
	 */
	explicit StringDecoder( const QString& codec = QString() )
	{
		if( !codec.isEmpty() )
			setCodec( codec );
	}

	#if defined( MOCKNETWORKACCESSMANAGER_QT_HAS_TEXTCODEC )
		/*! Creates a StringDecoder which uses the given QTextCodec as implementation.
		 *
		 * \param codec The QTextCodec to be used to decode the data.
		 * If \p codec is `NULL`, the constructed StringDecoder will be invalid.
		 */
		StringDecoder( QTextCodec* codec );
	#endif

	#if QT_VERSION >= QT_VERSION_CHECK( 6,0,0 )
		/*! Creates a StringDecoder which uses the given QStringDecoder as implementation.
		 *
		 * \note Since StringDecoder is stateless, it will call QStringDecoder::resetState()
		 * on the \p decoder every time before it decodes data.
		 *
		 * \param decoder The QStringDecoder to be used to decode the data. If \p decoder
		 * contains a `nullptr`, the constructed StringDecoder will be invalid.
		 */
		StringDecoder( std::unique_ptr<QStringDecoder>&& decoder );
	#endif

	/*! Creates a copy of another StringDecoder.
	 *
	 * The constructed StringDecoder will use the same implementation
	 * as \p other.
	 *
	 * \param other The StringDecoder to be copied.
	 */
	StringDecoder( const StringDecoder& other )
	{
		if( other.m_impl )
			m_impl.reset( other.m_impl->clone() );
	}

	/*! Creates a StringDecoder by moving another one.
	 *
	 * \param other The StringDecoder to be moved.
	 */
	StringDecoder( StringDecoder&& other ) = default;

	/*! Destroys this StringDecoder and its implementation.
	 */
	~StringDecoder()
	{
		// unique_ptr takes care of clean up
		// This destructor just exists to fix SonarCloud cpp:S3624
	}

	/*! Makes this StringDecoder use the same implementation as another one.
	 *
	 * \param other The StringDecoder whose implementation is copied.
	 * \return A reference to this StringDecoder.
	 */
	StringDecoder& operator=( StringDecoder other )
	{
		m_impl.swap( other.m_impl );
		return *this;
	}

	/*! Makes this StringDecoder use the implementation of another one.
	 *
	 * \param other The StringDecoder whose implementation is moved.
	 * \return A reference to this StringDecoder.
	 */
	StringDecoder& operator=( StringDecoder&& other ) = default;

	/*! Checks if this StringDecoder can decode data.
	 *
	 * Trying to decode data with an invalid StringDecoder may result in undefined
	 * behavior.
	 *
	 * \return \c true if this StringDecoder contains a valid implementation
	 * and can decode data.
	 */
	bool isValid() const
	{
		return m_impl && m_impl->isValid();
	}

	/*! Sets the codec used by this StringDecoder.
	 *
	 * \param codec The name of the codec to be used to decode data.
	 * If \p codec is empty or unknown to the implementation, this StringDecoder
	 * becomes invalid.
	 *
	 * \sa isValid()
	 */
	void setCodec( const QString& codec )
	{
		ensureImpl();
		m_impl->setCodec( codec );
	}

	/*! Sets the codec by trying to detect the codec of given data.
	 *
	 * If the codec cannot be detected and \p fallbackCodec is empty or
	 * unknown to the implementation, this StringDecoder becomes invalid.
	 *
	 * \param data The data whose codec should be detected.
	 * \param fallbackCodec If the codec of \p data cannot be detected,
	 * this \p fallbackCodec is used instead.
	 *
	 * \sa isValid()
	 */
	void setCodecFromData( const QByteArray& data, const QString& fallbackCodec )
	{
		ensureImpl();
		m_impl->setCodecFromData( data, fallbackCodec );
	}

	/*! Decodes data with the configured codec.
	 *
	 * \warning The StringDecoder must be valid when calling decode() or undefined
	 * behavior might be invoked.
	 *
	 * \param data The data to be decoded.
	 * \return
	 *
	 * \sa QTextCodec::toUnicode()
	 * \sa QStringDecoder::decode()
	 */
	QString decode( const QByteArray& data ) const
	{
		Q_ASSERT_X( isValid(), Q_FUNC_INFO, "Trying to use invalid StringDecoder" );
		return m_impl->decode( data );
	}

private:
	//! \cond PRIVATE_IMPLEMENTATION
	class Impl
	{
	public:
		virtual ~Impl() {}
		virtual bool isValid() const = 0;
		virtual void setCodec( const QString& codec ) = 0;
		virtual void setCodecFromData( const QByteArray& data, const QString& fallbackCodec ) = 0;
		virtual QString decode( const QByteArray& data ) const = 0;
		virtual Impl* clone() const = 0;
	};

	#if defined( MOCKNETWORKACCESSMANAGER_QT_HAS_TEXTCODEC )
		class TextCodecImpl : public Impl
		{
		public:
			explicit TextCodecImpl( const QTextCodec* codec = Q_NULLPTR )
				: m_codec( codec )
			{}
			virtual bool isValid() const Q_DECL_OVERRIDE
			{
				return m_codec != Q_NULLPTR;
			}
			virtual void setCodec( const QString& codec ) Q_DECL_OVERRIDE
			{
				m_codec = QTextCodec::codecForName( codec.toUtf8() );
			}
			virtual void setCodecFromData( const QByteArray& data, const QString& fallbackCodec ) Q_DECL_OVERRIDE
			{
				m_codec = QTextCodec::codecForUtfText( data, Q_NULLPTR );
				if( !m_codec )
					setCodec( fallbackCodec );
			}
			virtual QString decode( const QByteArray& data ) const Q_DECL_OVERRIDE
			{
				Q_ASSERT( m_codec );
				return m_codec->toUnicode( data );
			}
			virtual Impl* clone() const Q_DECL_OVERRIDE
			{
				return new TextCodecImpl( m_codec );
			}
		private:
			const QTextCodec* m_codec;
		};
	#endif

	#if QT_VERSION >= QT_VERSION_CHECK( 6,0,0 )
		class StringDecoderImpl : public Impl
		{
		public:
			StringDecoderImpl()
			{
			}
			explicit StringDecoderImpl( std::unique_ptr<QStringDecoder>&& decoder )
				: m_decoder( std::move( decoder ) )
			{
			}
			virtual bool isValid() const Q_DECL_OVERRIDE
			{
				return m_decoder && m_decoder->isValid();
			}
			virtual void setCodec( const QString& codec ) Q_DECL_OVERRIDE
			{
				auto encoding = QStringConverter::encodingForName( codec.toUtf8().constData() );
				if( encoding )
				{
					constructQStringDecoder( encoding.value() );
					return;
				}
				m_decoder.reset();
			}
			virtual void setCodecFromData( const QByteArray& data, const QString& fallbackCodec ) Q_DECL_OVERRIDE
			{
				auto encoding = QStringConverter::encodingForData( data );
				if( encoding )
				{
					constructQStringDecoder( encoding.value() );
					return;
				}
				setCodec( fallbackCodec );
			}
			virtual QString decode( const QByteArray& data ) const Q_DECL_OVERRIDE
			{
				Q_ASSERT( m_decoder );
				m_decoder->resetState();
				return m_decoder->decode( data );
			}
			virtual Impl* clone() const Q_DECL_OVERRIDE
			{
				if( !isValid() )
					return new StringDecoderImpl{};

				const auto* encodingName = m_decoder->name();
				Q_ASSERT( encodingName );
				const auto encoding = QStringConverter::encodingForName( encodingName );
				Q_ASSERT( encoding );
				auto cloned = std::make_unique<StringDecoderImpl>();
				cloned->constructQStringDecoder( encoding.value() );
				return cloned.release();
			}
		private:
			void constructQStringDecoder( QStringConverter::Encoding encoding )
			{
				m_decoder = std::make_unique<QStringDecoder>( encoding, QStringConverter::Flag::Stateless );
			}
			std::unique_ptr<QStringDecoder> m_decoder;
		};
	#endif
	//! \endcond

private:

	void ensureImpl()
	{
		if( !m_impl )
		{
			#if defined( MOCKNETWORKACCESSMANAGER_QT_HAS_TEXTCODEC )
				m_impl.reset( new TextCodecImpl() );
			#else
				m_impl.reset( new StringDecoderImpl() );
			#endif
		}
	}

	std::unique_ptr<Impl> m_impl;
};

class Rule;
class MockReplyBuilder;
template<class Base>
class Manager;

/*! QList of QByteArray. */
typedef QList<QByteArray> ByteArrayList;
/*! QSet of [QNetworkRequest::Attribute].
 * [QNetworkRequest::Attribute]: http://doc.qt.io/qt-5/qnetworkrequest.html#Attribute-enum
 */
typedef QSet<QNetworkRequest::Attribute> AttributeSet;
/*! QHash holding [QNetworkRequest::KnowHeaders] and their corresponding values.
 * \sa QNetworkRequest::header()
 * [QNetworkRequest::KnowHeaders]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
 */
typedef QHash<QNetworkRequest::KnownHeaders, QVariant> HeaderHash;
/*! QSet holding [QNetworkRequest::KnowHeaders].
 * [QNetworkRequest::KnowHeaders]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
 */
typedef QSet<QNetworkRequest::KnownHeaders> KnownHeadersSet;
/*! QHash holding raw headers and their corresponding values.
 * \sa QNetworkRequest::rawHeader()
 */
typedef QHash<QByteArray, QByteArray> RawHeaderHash;
/*! QHash holding query parameter names and their corresponding values.
 * \sa QUrlQuery
 */
typedef QHash<QString, QString> QueryParameterHash;
/*! QHash holding query parameter names and their corresponding values.
 * \sa QUrlQuery
 * \since 0.4.0
 */
typedef QHash<QString, QStringList> MultiValueQueryParameterHash;
/*! QVector of QRegularExpression QPairs.
 */
typedef QVector<QPair<QRegularExpression, QRegularExpression> > RegExPairVector;

/*! Determines the MIME type of data.
 * \param url The URL of the \p data.
 * \param data The data itself.
 * \return The MIME type of the \p data located at \p url.
 * \sa QMimeDatabase::mimeTypeForFileNameAndData()
 */
inline QMimeType guessMimeType(const QUrl& url, const QByteArray& data)
{
	const QFileInfo fileInfo(url.path());
	return QMimeDatabase().mimeTypeForFileNameAndData(fileInfo.fileName(), data);
}

/*! Provides access to the request data.
 *
 * This mainly groups all the request data into a single struct for convenience.
 */
struct Request
{
	/*! The HTTP request verb.
	 */
	QNetworkAccessManager::Operation operation;
	/*! The QNetworkRequest object.
	 * This provides access to the details of the request like URL, headers and attributes.
	 */
	QNetworkRequest qRequest;
	/*! The body data.
	 */
	QByteArray body;
	/*! The timestamp when the Manager began handling the request.
	 * For requests received through the public API of QNetworkAccessManager,
	 * this can be considered the time when the Manager received the request.
	 */
	QDateTime timestamp;

	/*! Creates an invalid Request object.
	 * \sa isValid()
	 */
	Request() : operation(QNetworkAccessManager::CustomOperation) {}

	/*! Creates a Request struct.
	 * \param op The Request::operation.
	 * \param req The Request:.qRequest.
	 * \param data The Request::body.
	 * \note The Request::timestamp will be set to the current date and time.
	 */
	Request(QNetworkAccessManager::Operation op,
	        const QNetworkRequest& req,
	        const QByteArray& data = QByteArray())
		: operation(op)
		, qRequest(req)
		, body(data)
		, timestamp(QDateTime::currentDateTime())
	{}

	/*! Creates a Request struct.
	 * \param req The Request:.qRequest.
	 * \param op The Request::operation.
	 * \param data The Request::body.
	 * \note The Request::timestamp will be set to the current date and time.
	 */
	Request(const QNetworkRequest& req,
	        QNetworkAccessManager::Operation op = QNetworkAccessManager::GetOperation,
	        const QByteArray& data = QByteArray())
		: operation(op)
		, qRequest(req)
		, body(data)
		, timestamp(QDateTime::currentDateTime())
	{}

	/*! \return \c true if the Request specifies a valid HTTP verb and the qRequest contains a valid URL.
	 * The HTTP is not valid if operation is QNetworkAccessManager::CustomOperation
	 * and the [QNetworkRequest::CustomVerbAttribute] of qRequest is empty.
	 * [QNetworkRequest::CustomVerbAttribute]: http://doc.qt.io/qt-5/qnetworkrequest.html#Attribute-enum
	 */
	bool isValid() const
	{
		return qRequest.url().isValid()
		    && (operation != QNetworkAccessManager::CustomOperation
		        || !qRequest.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray().trimmed().isEmpty());
	}

	/*! Checks if two Request structs are equal.
	 * \param left One Request struct to be compared.
	 * \param right The other Request struct to be compared with \p left.
	 * \return \c true if all fields of \p left and \c right are equal (including the Request::timestamp).
	 * \c false otherwise.
	 */
	friend bool operator==(const Request& left, const Request& right)
	{
		return left.operation == right.operation
		    && left.qRequest  == right.qRequest
		    && left.body      == right.body
		    && left.timestamp == right.timestamp;
	}

	/*! Checks if two Request structs differ.
	 * \param left One Request struct to be compared.
	 * \param right The other Request struct to be compared with \p left.
	 * \return \c true if at least one field of \p left and \c right differs (including the Request::timestamp).
	 * \c false if \p left and \p right are equal.
	 */
	friend bool operator!=(const Request& left, const Request& right)
	{
		return !(left == right);
	}

	/*! Returns the operation (HTTP verb) of the request as a string.
	 * \return The Request::operation of the Request as a QString or a null `QString()` if the operation is unknown
	 * or it is `QNetworkAccessManager::CustomOperation` but the `QNetworkRequest::CustomVerbAttribute` was not set
	 * on the Request::qRequest. For the standard operations, the verb is returned in all uppercase letters. For a
	 * `CustomOperation`, the verb is return as set in the `QNetworkRequest::CustomVerbAttribute`.
	 */
	QString verb() const
	{
		switch( this->operation )
		{
		case QNetworkAccessManager::GetOperation:    return QStringLiteral( "GET" );
		case QNetworkAccessManager::HeadOperation:   return QStringLiteral( "HEAD" );
		case QNetworkAccessManager::PostOperation:   return QStringLiteral( "POST" );
		case QNetworkAccessManager::PutOperation:    return QStringLiteral( "PUT" );
		case QNetworkAccessManager::DeleteOperation: return QStringLiteral( "DELETE" );
		case QNetworkAccessManager::CustomOperation:
			return this->qRequest.attribute( QNetworkRequest::CustomVerbAttribute ).toString();
		// LCOV_EXCL_START
		default:
			qCWarning( log ) << "Unknown operation:" << this->operation;
			return QString();
		// LCOV_EXCL_STOP
		}
	}
};

/*! QList of Request structs.*/
typedef QList<Request> RequestList;

/*! Holds the information necessary to make a signal connection.
 *
 * \sa QObject::connect()
 */
class SignalConnectionInfo
{
public:
	/*! Creates an invalid SignalConnectionInfo object.
	 */
	SignalConnectionInfo()
		: m_sender( Q_NULLPTR )
		, m_connectionType( Qt::AutoConnection )
	{}

	/*! Creates a SignalConnectionInfo for a given object and signal.
	 *
	 * \param sender The QObject which is the sender of the signal.
	 * \param metaSignal The QMetaMethod of the signal.
	 * \param connectionType The type of the connection.
	 */
	SignalConnectionInfo( QObject* sender, const QMetaMethod& metaSignal,
	                      Qt::ConnectionType connectionType = Qt::AutoConnection )
		: m_sender( sender )
		, m_signal( metaSignal )
		, m_connectionType( connectionType )
	{}

	/*! \return The sender QObject.
	 */
	QObject* sender() const
	{
		return m_sender;
	}

	/*! \return The QMetaMethod of the signal.
	 */
	// @sonarcloud-exclude-start
	QMetaMethod signal() const
	// @sonarcloud-exclude-end
	{
		return m_signal;
	}

	/*! \return The type of the connection.
	 */
	Qt::ConnectionType connectionType() const
	{
		return m_connectionType;
	}

	/*! \return \c true if this SignalConnectionInfo object contains information allowing to make a valid signal
	 * connection. This means that there must be a sender object set and a signal which belongs to this sender object.
	 */
	bool isValid() const
	{
		return m_sender && m_signal.isValid() && m_signal.methodType() == QMetaMethod::Signal
			&& m_sender->metaObject()->method( m_signal.methodIndex() ) == m_signal;
	}

	/*! Creates a connection to the signal described by this %SignalConnectionInfo.
	 *
	 * \note If this %SignalConnectionInfo object is not valid, the connection will not be established and an invalid
	 * QMetaObject::Connection object is returned.
	 *
	 * \param receiver The receiver QObject.
	 * \param slotOrSignal The QMetaMethod of the signal or slot which is connected to the signal described by the this
	 * %SignalConnectionInfo.
	 * \return The QMetaObject::Connection object as returned by QObject::connect().
	 *
	 * \sa isValid()
	 * \sa QObject::connect()
	 */
	QMetaObject::Connection connect(QObject* receiver, const QMetaMethod& slotOrSignal) const
	{
		return QObject::connect( m_sender, m_signal, receiver, slotOrSignal, m_connectionType );
	}

	/*! Compares two SignalConnectionInfo objects for equality.
	 *
	 * \param left One SignalConnectionInfo object.
	 * \param right Another SignalConnectionInfo object.
	 * \return \c true if \p left and \p right contain the same data.
	 */
	friend bool operator==( const SignalConnectionInfo& left, const SignalConnectionInfo& right )
	{
		return left.m_sender == right.m_sender
		    && left.m_signal == right.m_signal
		    && left.m_connectionType == right.m_connectionType;
	}

	/*! Compares two SignalConnectionInfo objects for inequality.
	 *
	 * \param left One SignalConnectionInfo object.
	 * \param right Another SignalConnectionInfo object.
	 * \return \c true if \p left and \p right contain different data.
	 */
	friend bool operator!=( const SignalConnectionInfo& left, const SignalConnectionInfo& right )
	{
		return !(left == right);
	}

private:
	QObject* m_sender;
	QMetaMethod m_signal;
	Qt::ConnectionType m_connectionType;

};

/*! \internal Implementation details
 */
namespace detail {

#if QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 )

inline bool usesSafeRedirectCustomRequestMethod( const Request& request );

/* RFC-7231 defines the request methods GET, HEAD, OPTIONS, and TRACE to be safe
 * for automatic redirection using the same method.
 * See https://tools.ietf.org/html/rfc7231#section-6.4
 * and https://tools.ietf.org/html/rfc7231#section-4.2.1
 */
inline bool usesSafeRedirectRequestMethod( const Request& request )
{
	switch( request.operation )
	{
	case QNetworkAccessManager::GetOperation:
	case QNetworkAccessManager::HeadOperation:
		return true;
	case QNetworkAccessManager::CustomOperation:
		return usesSafeRedirectCustomRequestMethod( request );
	default:
		return false;
	}
}

inline bool usesSafeRedirectCustomRequestMethod( const Request& request )
{
	const QString customVerb = request.qRequest.attribute( QNetworkRequest::CustomVerbAttribute )
	                           .toString().toLower();
	return ( customVerb == QLatin1String( "options" ) || customVerb == QLatin1String( "trace" ) );
}

#endif // Qt >= 5.6.0

inline bool isDataUrlRequest( const Request& request )
{
	return request.qRequest.url().scheme() == DataUrlUtils::dataScheme();
}


} // namespace detail

/*! Mocked QNetworkReply.
 *
 * The MockReply is returned by the Manager instead of a real QNetworkReply.
 * Instead of sending the request to the server and returning the reply,
 * the MockReply returns the predefined (mocked) data.
 *
 * A MockReply behaves like an HTTP based QNetworkReply, except that it doesn't emit
 * the implementation specific signals like QNetworkReplyHttpImpl::startHttpRequest() or
 * QNetworkReplyHttpImpl::abortHttpRequest().
 */
class MockReply : public QNetworkReply
{
	Q_OBJECT

	template<class Base>
	friend class Manager;
	friend class MockReplyBuilder;
	friend class Rule;

public:

	/*! \return The message body of this reply. */
	QByteArray body() const { return m_body.data(); }

	/*! \return The set of attributes defined on this reply.
	 */
	QSet<QNetworkRequest::Attribute> attributes() const { return m_attributeSet; }

	/*! \return The signal connection that is used to delay and trigger the finished() signal.
	 * If the return signal connection is invalid, the finished() signal is note delayed.
	 */
	SignalConnectionInfo finishDelaySignal() const { return m_finishDelaySignal; }

	/*! \return \c true
	 * \sa QIODevice::isSequential()
	 */
	virtual bool isSequential() const Q_DECL_OVERRIDE { return true; }
	/*! \return The number of bytes available for reading.
	 *\sa QIODevice::bytesAvailable()
	 */
	virtual qint64 bytesAvailable() const Q_DECL_OVERRIDE { return m_body.bytesAvailable(); }

	/*! Aborts the simulated network communication.
	 * \note At the moment, this method does nothing else than calling close()
	 * since the MockReply is already finished before it is returned by the Manager.
	 * However, future versions might simulate network communication and then,
	 * this method allows aborting that.\n
	 * See issue \issue{4}.
	 */
	virtual void abort() Q_DECL_OVERRIDE
	{
		if (this->isRunning())
		{
			this->setError(QNetworkReply::OperationCanceledError);
			setFinished(true);
			// TODO: Need to actually finish including emitting signals
		}
		close();
	}

	/*! Prevents reading further body data from the reply.
	 * \sa QNetworkReply::close()
	 */
	virtual void close() Q_DECL_OVERRIDE
	{
		m_body.close();
		QNetworkReply::close();
	}

	/*! Creates a clone of this reply.
	 *
	 * \return A new MockReply which has the same properties as this MockReply.
	 * The caller takes ownership of the returned object.
	 */
	virtual MockReply* clone() const
	{
		MockReply* clone = new MockReply();
		clone->setBody( this->body() );
		clone->setRequest( this->request() );
		clone->setUrl( this->url() );
		clone->setOperation( this->operation() );
		if( m_useDefaultErrorString )
			clone->setError( this->error() );
		else
			clone->setError( this->error(), this->errorString() );
		clone->setSslConfiguration( this->sslConfiguration() );
		clone->setReadBufferSize( this->readBufferSize() );
		clone->setBehaviorFlags( this->m_behaviorFlags );

		clone->copyHeaders( this );
		clone->copyAttributes( this );

		if ( this->isOpen() )
			clone->open( this->openMode() );

		clone->setFinished( this->isFinished() );

		clone->m_finishDelaySignal = this->m_finishDelaySignal;

		return clone;
	}

private:
	void copyHeaders( const MockReply* other )
	{
		const QByteArray setCookieHeader = QByteArrayLiteral( "Set-Cookie" );
		KnownHeadersSet copyKnownHeaders;

		const ByteArrayList rawHeaders = other->rawHeaderList();
		ByteArrayList::const_iterator rawIter = rawHeaders.cbegin();
		const ByteArrayList::const_iterator rawEnd = rawHeaders.cend();
		for ( ; rawIter != rawEnd; ++rawIter )
		{
			if ( *rawIter == setCookieHeader )
			{
				/* Qt doesn't properly concatenate Set-Cookie entries when returning
				 * rawHeader(). Therefore, we need to copy that header using header()
				 * (see below).
				 */
				copyKnownHeaders.insert( QNetworkRequest::SetCookieHeader );
				continue;
			}
			if ( *rawIter == HttpUtils::locationHeader() )
			{
				const QUrl locationHeader = other->locationHeader();
				if ( locationHeader.isValid() && locationHeader.scheme().isEmpty()
				    && locationHeader == other->header( QNetworkRequest::LocationHeader ) )
				{
					/* Due to QTBUG-41061, relative location headers are not set correctly when using
					 * setRawHeader(). Therefore, we need to copy that header using header()
					 * (see below).
					 */
					copyKnownHeaders.insert( QNetworkRequest::LocationHeader );
					continue;
				}
			}
			this->setRawHeader( *rawIter, other->rawHeader( *rawIter ) );
		}

		KnownHeadersSet::const_iterator knownIter = copyKnownHeaders.cbegin();
		const KnownHeadersSet::const_iterator knownEnd = copyKnownHeaders.cend();
		for ( ; knownIter != knownEnd; ++knownIter )
		{
			this->setHeader( *knownIter, other->header( *knownIter ) );
		}
	}

	void copyAttributes( const MockReply* other )
	{
		AttributeSet::const_iterator iter = other->m_attributeSet.cbegin();
		const AttributeSet::const_iterator end = other->m_attributeSet.cend();
		for ( ; iter != end; ++iter )
		{
			this->setAttribute( *iter, other->attribute( *iter ) );
		}
	}

public:

	/*! Checks if this reply indicates a redirect that can be followed automatically.
	 * \return \c true if this reply's HTTP status code and \c Location header
	 * are valid and the status code indicates a redirect that can be followed automatically.
	 */
	bool isRedirectToBeFollowed() const
	{
		const QVariant statusCodeAttr = this->attribute( QNetworkRequest::HttpStatusCodeAttribute );
		if ( ! statusCodeAttr.isValid() )
			return false;

		const QUrl locationHeaderUrl = this->locationHeader();
		if ( ! locationHeaderUrl.isValid() )
			return false;

		switch ( statusCodeAttr.toInt() )
		{
		case HttpStatus::MovedPermanently:  // 301
		case HttpStatus::Found:             // 302
		case HttpStatus::SeeOther:          // 303
		case HttpStatus::UseProxy:          // 305
		case HttpStatus::TemporaryRedirect: // 307
			return true;
		case HttpStatus::PermanentRedirect: // 308
			if ( m_behaviorFlags.testFlag( Behavior_NoAutomatic308Redirect ) )
				return false; // Qt doesn't recognize 308 for automatic redirection
			else
				return true;
		default:
			return false;
		}
	}

	/*! Checks if this reply indicates that the request requires authentication.
	 * \return \c true if the HTTP status code indicates that the request must be resend
	 * with appropriate authentication to succeed.
	 */
	bool requiresAuthentication() const
	{
		switch ( this->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt() )
		{
		case HttpStatus::Unauthorized:                // 401
		case HttpStatus::ProxyAuthenticationRequired: // 407
			return true;
		default:
			return false;
		}
	}

	/*! Returns the URL of the HTTP Location header field of a given QNetworkReply.
	 * This is a workaround for QTBUG-4106 which prevents that the QNetworkReply::header() method returns a valid
	 * QUrl for relative redirection URLs.
	 * \param reply The QNetworkReply for which the Location header should be returned.
	 * \return The value of the Location header field as a QUrl.
	 * \sa https://bugreports.qt.io/browse/QTBUG-41061
	 * \since 0.4.0
	 */
	static QUrl locationHeader( const QNetworkReply* reply )
	{
		const QByteArray rawHeader = reply->rawHeader( HttpUtils::locationHeader() );
		if ( rawHeader.isEmpty() )
			return QUrl();
		else
			return QUrl::fromEncoded( rawHeader, QUrl::StrictMode );
	}

	/*! Returns the URL of the HTTP Location header field.
	 *
	 * \return The value of the Location header field as a QUrl.
	 * \sa locationHeader(const QNetworkReply*)
	 * \since 0.4.0
	 */
	QUrl locationHeader() const
	{
		return locationHeader( this );
	}

protected:
	/*! Creates a MockReply object.
	 * \param parent Parent QObject.
	 */
	explicit MockReply( QObject* parent = Q_NULLPTR )
		: QNetworkReply( parent )
		, m_behaviorFlags( Behavior_Expected )
		, m_redirectCount( 0 )
		, m_userDefinedError( false )
		, m_useDefaultErrorString( true )
	{
	}

	/*! Reads bytes from the reply's body.
	 * \param[out] data A pointer to an array where the bytes will be written to.
	 * \param maxlen The maximum number of bytes that should be read.
	 * \return The number of bytes read or -1 if an error occurred.
	 * \sa QIODevice::readData()
	 */
	virtual qint64 readData( char *data, qint64 maxlen ) Q_DECL_OVERRIDE
	{
		return m_body.read( data, maxlen );
	}

	/*! Sets the message body of this reply.
	 * \param data The body data.
	 */
	void setBody( const QByteArray& data )
	{
		m_body.setData( data );
	}

	/*! Sets an attribute for this reply.
	 * \param attribute The attribute key.
	 * \param value The value for the attribute.
	 * \sa QNetworkReply::setAttribute()
	 */
	void setAttribute( QNetworkRequest::Attribute attribute, const QVariant& value )
	{
		m_attributeSet.insert( attribute );
		QNetworkReply::setAttribute( attribute, value );
	}

	/*! Sets the error for this reply.
	 *
	 * \param error The error code.
	 * \param errorString A human-readable string describing the error.
	 */
	void setError( QNetworkReply::NetworkError error, const QString& errorString )
	{
		m_userDefinedError = true;
		if( !errorString.isNull() )
			m_useDefaultErrorString = false;
		QNetworkReply::setError( error, errorString );
	}

	/*! \overload
	 * This overload uses %Qt's default error strings for the given \p error code.
	 * \param error The error code to be set for this reply.
	 */
	void setError( QNetworkReply::NetworkError error )
	{
		m_userDefinedError = true;
		QNetworkReply::setError( error, this->errorString() );
	}


protected Q_SLOTS:


	/*! Prepares the MockReply for returning to the caller of the Manager.
	 *
	 * This method ensures that this reply has proper values set for the required headers, attributes and properties.
	 * For example, it will set the [QNetworkRequest::ContentLengthHeader] and try to guess the correct value for the
	 * QNetworkRequest::ContentTypeHeader.
	 * However, it will *not* override headers and attributes which have been set explicitly.
	 *
	 * \param request The request this reply is answering.
	 *
	 * [QNetworkRequest::ContentLengthHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	void prepare( const Request& request )
	{
		if( FileUtils::isFileLikeScheme( request.qRequest.url() ) )
		{
			prepareFileLikeReply( request );
			return;
		}

		prepareHttpLikeReply( request );
	}

private:
	void prepareFileLikeReply( const Request& request )
	{
		this->prepareUrlForFileLikeReply( request );
		this->copyPropertiesFromRequest( request );
		this->setAttribute( QNetworkRequest::ConnectionEncryptedAttribute, false );

		switch( request.operation )
		{
		case QNetworkAccessManager::GetOperation:
		case QNetworkAccessManager::HeadOperation:
			this->updateContentLengthHeader();
			break;
		case QNetworkAccessManager::PutOperation:
			if( request.qRequest.url().scheme() == FileUtils::qrcScheme() )
				this->setAccessDeniedErrorForQrcPutReply( request );
			break;
		default:
			this->setProtocolUnknownError( request );
			break;
		}

		this->updateErrorString( request );
	}

	void prepareUrlForFileLikeReply( const Request& request )
	{
		QUrl url = request.qRequest.url();
		if ( url.host() == QLatin1String( "localhost" ) )
			url.setHost( QString() );
		this->setUrl( url );
	}

	void prepareHttpLikeReply( const Request& request )
	{
		this->copyPropertiesFromRequest( request );

		this->setEncryptedAttribute();

		this->updateContentTypeHeader();
		this->updateContentLengthHeader();
		this->updateHttpStatusCode();
		this->updateHttpReasonPhrase();
		this->updateRedirectionTargetAttribute();
		this->updateErrorString( request );
	}


	void copyPropertiesFromRequest( const Request& request )
	{
		this->setRequest( request.qRequest );
		if ( ! this->url().isValid() )
			this->setUrl( request.qRequest.url() );
		this->setOperation( request.operation );
		this->setSslConfiguration( request.qRequest.sslConfiguration() );
	}

	void setAccessDeniedErrorForQrcPutReply( const Request& request )
	{
		if( m_userDefinedError )
		{
			if( this->error() == QNetworkReply::ContentAccessDenied && !m_useDefaultErrorString )
				return;

			qCWarning( log ) << "Reply was configured to reply with error" << this->error()
				<< "but a qrc request does not support writing operations and therefore has to reply with"
				<< QNetworkReply::ContentAccessDenied << ". Overriding configured behavior.";
		}

		QNetworkReply::setError( QNetworkReply::ContentAccessDenied,
		                         defaultErrorString( QNetworkReply::ContentAccessDenied, request ) );
	}

	void setProtocolUnknownError( const Request& request )
	{
		if( m_userDefinedError )
		{
			if( this->error() == QNetworkReply::ProtocolUnknownError && !m_useDefaultErrorString )
				return;

			if( FileUtils::isFileLikeScheme( request.qRequest.url() ) )
			{
				qCWarning( log ) << "Reply was configured to reply with error" << this->error()
					<< "but a request" << request.verb().toUtf8().constData()
					<< request.qRequest.url().toString().toUtf8().constData()
					<< "must be replied with" << QNetworkReply::ProtocolUnknownError
					<< ". Overriding configured behavior.";
			}
		}

		QNetworkReply::setError( QNetworkReply::ProtocolUnknownError,
		                         defaultErrorString( QNetworkReply::ProtocolUnknownError, request ) );
	}


	void setEncryptedAttribute()
	{
		const QString scheme = this->url().scheme().toLower();
		const bool isEncrypted =  scheme == HttpUtils::httpsScheme();
		this->setAttribute( QNetworkRequest::ConnectionEncryptedAttribute,
		                    QVariant::fromValue( isEncrypted ) );
	}

	void updateContentTypeHeader()
	{
		if (   ! this->header( QNetworkRequest::ContentTypeHeader ).isValid()
		    && ! this->body().isEmpty() )
		{
			const QMimeType mimeType = guessMimeType( this->url(), m_body.data() );
			this->setHeader( QNetworkRequest::ContentTypeHeader, QVariant::fromValue( mimeType.name() ) );
		}
	}

	void updateContentLengthHeader()
	{
		if (     this->rawHeader( "Transfer-Encoding" ).isEmpty()
		    && ! this->header( QNetworkRequest::ContentLengthHeader ).isValid()
		    && ! this->body().isNull() )
		{
			this->setHeader( QNetworkRequest::ContentLengthHeader, QVariant::fromValue( this->body().length() ) );
		}

		#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
			if ( ! this->attribute( QNetworkRequest::OriginalContentLengthAttribute ).isValid() )
			{
				this->setAttribute( QNetworkRequest::OriginalContentLengthAttribute,
				                    this->header( QNetworkRequest::ContentLengthHeader ) );
			}
		#endif // Qt >= 5.9.0
	}

	void updateHttpStatusCode()
	{
		QVariant statusCodeAttr = this->attribute( QNetworkRequest::HttpStatusCodeAttribute );
		if ( statusCodeAttr.isValid() )
		{
			bool canConvertToInt = false;
			statusCodeAttr.toInt( &canConvertToInt );
			if ( ! canConvertToInt )
			{
				qCWarning( log ) << "Invalid type for HttpStatusCodeAttribute:" << statusCodeAttr.typeName();
				statusCodeAttr = QVariant();
				this->setAttribute( QNetworkRequest::HttpStatusCodeAttribute, statusCodeAttr );
			}
		}
		if ( ! statusCodeAttr.isValid() )
		{
			const int statusCode = HttpStatus::networkErrorToStatusCode( this->error() );
			if ( statusCode > 0 )
			{
				statusCodeAttr = QVariant::fromValue( statusCode );
				this->setAttribute( QNetworkRequest::HttpStatusCodeAttribute, statusCodeAttr );
			}
		}
	}

	void updateHttpReasonPhrase()
	{
		const QVariant statusCodeAttr = this->attribute( QNetworkRequest::HttpStatusCodeAttribute );
		if ( ! this->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).isValid() && statusCodeAttr.isValid() )
		{
			this->setAttribute( QNetworkRequest::HttpReasonPhraseAttribute,
			                    HttpStatus::reasonPhrase( statusCodeAttr.toInt() ).toUtf8() );
		}
	}

	void updateRedirectionTargetAttribute()
	{
		/* Qt doesn't set the RedirectionTargetAttribute for 305 redirects.
		 * See QNetworkReplyHttpImplPrivate::checkForRedirect(const int statusCode)
		 */
		const QVariant statusCodeAttr = this->attribute( QNetworkRequest::HttpStatusCodeAttribute );
		if ( this->isRedirectToBeFollowed() && statusCodeAttr.toInt() != static_cast< int >( HttpStatus::UseProxy ) )
		{
			const QUrl locationHeaderUrl = this->locationHeader();
			this->setAttribute( QNetworkRequest::RedirectionTargetAttribute, locationHeaderUrl );
		}
	}

	void updateErrorString( const Request& request )
	{
		if( m_useDefaultErrorString )
		{
			this->setError( this->error(), defaultErrorString( this->error(), request ) );
		}
	}

	QString defaultErrorString( QNetworkReply::NetworkError error, const Request& request ) const
	{
		const QString scheme = request.qRequest.url().scheme();

		if ( !FileUtils::isFileLikeScheme( scheme ) && useDefaultStatusCodeErrorString( error ) )
			return defaultStatusCodeErrorString( request );

		const QString protocol = protocolFromScheme( scheme );

		const QString protocolSpecificError = protocolSpecificErrorString( error, protocol, request );
		if ( !protocolSpecificError.isNull() )
			return protocolSpecificError;

		return protocolCommonErrorString( error, protocol, request );
	}

	static bool useDefaultStatusCodeErrorString( QNetworkReply::NetworkError errorCode )
	{
		switch( errorCode )
		{
		case QNetworkReply::UnknownContentError:               // other 4xx
		case QNetworkReply::ProtocolInvalidOperationError:     // 400
		case QNetworkReply::ContentAccessDenied:               // 403
		case QNetworkReply::ContentNotFoundError:              // 404
		case QNetworkReply::ContentOperationNotPermittedError: // 405

		#if QT_VERSION >= QT_VERSION_CHECK( 5,3,0 )
			case QNetworkReply::ContentConflictError:              // 409
			case QNetworkReply::ContentGoneError:                  // 410
			case QNetworkReply::UnknownServerError:                // other 5xx
			case QNetworkReply::InternalServerError:               // 500
			case QNetworkReply::OperationNotImplementedError:      // 501
			case QNetworkReply::ServiceUnavailableError:           // 503
		#endif // Qt >= 5.3.0

			return true;

		default:
			return false;
		}
	}

	QString defaultStatusCodeErrorString( const Request& request ) const
	{
		#if QT_VERSION < QT_VERSION_CHECK( 5,6,0 )
			const QString prefix = QStringLiteral( "Error downloading " );
		#else
			const QString prefix = QStringLiteral( "Error transferring " );
		#endif

		return prefix + request.qRequest.url().toDisplayString()
			   + QStringLiteral( " - server replied: " )
			   + this->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).toString();
	}

	static QString protocolFromScheme( const QString& scheme )
	{
		if( scheme == HttpUtils::httpsScheme() || scheme == HttpUtils::httpScheme() )
			return HttpUtils::httpScheme();
		if( scheme == FtpUtils::ftpsScheme() || scheme == FtpUtils::ftpScheme() )
			return FtpUtils::ftpScheme();
		if( scheme == FileUtils::fileScheme() || scheme == FileUtils::qrcScheme() )
			return FileUtils::fileScheme();

		return QString();
	}

	static QString protocolSpecificErrorString( QNetworkReply::NetworkError error,
	                                            const QString& protocol,
	                                            const Request& request )
		{
			if ( protocol == FtpUtils::ftpScheme() )
				return ftpErrorString( error, request );
			if ( protocol == FileUtils::fileScheme() )
				return fileErrorString( error, request );
			return fallbackProtocolErrorString( error, request, protocol );
		}

	static QString ftpErrorString( QNetworkReply::NetworkError error, const Request& request )
	{
		const QString hostName = request.qRequest.url().host();

		switch( error )
		{
		case QNetworkReply::ConnectionRefusedError:
			return QCoreApplication::translate( "QFtp", "Connection refused to host %1" ).arg( hostName );
		case QNetworkReply::TimeoutError:
			return QCoreApplication::translate( "QFtp", "Connection timed out to host %1" ).arg( hostName );
		case QNetworkReply::AuthenticationRequiredError:
			return QCoreApplication::translate( "QNetworkAccessFtpBackend",
			                                    "Logging in to %1 failed: authentication required" ).arg( hostName );
		default:
			return QString();
		}
	}

	static QString fileErrorString( QNetworkReply::NetworkError error, const Request& request )
	{
		const QString scheme = request.qRequest.url().scheme();
		switch( error )
		{
		case QNetworkReply::ContentOperationNotPermittedError:
			return QCoreApplication::translate( "QNetworkAccessFileBackend", "Cannot open %1: Path is a directory" )
			                                  .arg( request.qRequest.url().toString() );
		case QNetworkReply::ProtocolUnknownError:
			return QCoreApplication::translate( "QNetworkReply", "Protocol \"%1\" is unknown"  ).arg( scheme );
		case QNetworkReply::ContentAccessDenied:
		case QNetworkReply::ContentNotFoundError:
		case QNetworkReply::ProtocolFailure:
		default:
			return fileOperationErrorString( error, request );
		}
	}

	static QString fileOperationErrorString( QNetworkReply::NetworkError error, const Request& request )
	{
		const char* const fileTranslationContext = translationContextForProtocol( FileUtils::fileScheme() );
		const QString unknownError = QStringLiteral( "Unknown error" );
		const QUrl requestUrl = request.qRequest.url();
		if ( error == QNetworkReply::ContentNotFoundError ||
			( error == QNetworkReply::ContentAccessDenied && request.operation == QNetworkAccessManager::GetOperation ) )
		{
			QString detailErrorString = QStringLiteral( "No such file or directory" );
			if( error == QNetworkReply::ContentAccessDenied )
			{
				if( requestUrl.scheme() == FileUtils::qrcScheme() )
					detailErrorString = unknownError;
				else
					detailErrorString = QStringLiteral( "Access denied" );
			}
			return QCoreApplication::translate( fileTranslationContext,
			                                    "Error opening %1: %2" ).arg( requestUrl.toString(), detailErrorString );

		}

		if( error == QNetworkReply::ProtocolFailure )
		{
			if( request.operation == QNetworkAccessManager::PutOperation )
			{
				return QCoreApplication::translate( fileTranslationContext, "Write error writing to %1: %2" )
				                                  .arg( requestUrl.toString(), unknownError );
			}
			return QCoreApplication::translate( fileTranslationContext, "Read error reading from %1: %2" )
			                                  .arg( requestUrl.toString(), unknownError );
		}

		return QCoreApplication::translate( "QIODevice", "Unknown error" );
	}

	static QString fallbackProtocolErrorString( QNetworkReply::NetworkError error, const Request&, const QString& protocol )
	{
		const char* protocolTrContext = translationContextForProtocol( protocol );

		switch( error )
		{
		case QNetworkReply::ConnectionRefusedError:
			return QCoreApplication::translate( protocolTrContext, "Connection refused" );
		case QNetworkReply::TimeoutError:
			return QCoreApplication::translate( "QAbstractSocket", "Socket operation timed out" );
		case QNetworkReply::AuthenticationRequiredError: // 401
			return QCoreApplication::translate( protocolTrContext, "Host requires authentication" );
		default:
			return QString();
		}
	}

	static const char* translationContextForProtocol( const QString& protocol )
	{
		if( protocol == HttpUtils::httpScheme() )
			return "QHttp";
		if( protocol == FtpUtils::ftpScheme() )
			return "QFtp";
		if( protocol == FileUtils::fileScheme() )
			return "QNetworkAccessFileBackend";
		return "QNetworkReply";
	}

	static QString protocolCommonErrorString( QNetworkReply::NetworkError error, const QString& protocol,
	                                          const Request& request )
	{
		const QString hostName = request.qRequest.url().host();

		switch( error )
		{
		case QNetworkReply::RemoteHostClosedError: return protocolTr( protocol, "Connection closed" );
		case QNetworkReply::HostNotFoundError:     return protocolTr( protocol, "Host %1 not found" ).arg( hostName );
		case QNetworkReply::OperationCanceledError:
			return QCoreApplication::translate( "QNetworkReplyImpl", "Operation canceled" );
		case QNetworkReply::SslHandshakeFailedError:      return protocolTr( protocol, "SSL handshake failed" );
		case QNetworkReply::TemporaryNetworkFailureError: return qNetworkReplyTr( "Temporary network failure." );
		case QNetworkReply::NetworkSessionFailedError:    return qNetworkReplyTr( "Network session error." );
		case QNetworkReply::BackgroundRequestNotAllowedError:
			return qNetworkReplyTr( "Background request not allowed." );

		#if QT_VERSION >= QT_VERSION_CHECK( 5,6,0 )
			case QNetworkReply::TooManyRedirectsError: return protocolTr( protocol, "Too many redirects" );
			case QNetworkReply::InsecureRedirectError: return protocolTr( protocol, "Insecure redirect" );
		#endif // Qt >= 5.6.0

		case QNetworkReply::ProxyConnectionRefusedError:
			return qHttpSocketEngineTr( "Proxy connection refused" );
		case QNetworkReply::ProxyConnectionClosedError:
			return qHttpSocketEngineTr( "Proxy connection closed prematurely" );
		case QNetworkReply::ProxyNotFoundError:
			return protocolTr( protocol, "No suitable proxy found" );
		case QNetworkReply::ProxyTimeoutError:
			return qHttpSocketEngineTr( "Proxy server connection timed out" );
		case QNetworkReply::ProxyAuthenticationRequiredError:
			return protocolTr( protocol, "Proxy requires authentication" );

		case QNetworkReply::ProtocolUnknownError: return protocolTr( protocol, "Unknown protocol specified" );
		case QNetworkReply::ProtocolFailure:      return protocolTr( protocol, "Data corrupted" );
		case QNetworkReply::UnknownNetworkError:  return QStringLiteral( "Unknown network error" );
		case QNetworkReply::UnknownProxyError:    return QStringLiteral( "Unknown proxy error" );

		default:
			return QCoreApplication::translate( "QIODevice", "Unknown error" );
		}
	}

	static QString protocolTr( const QString& protocol, const char* sourceText )
	{
		const char* protocolTrContext = translationContextForProtocol( protocol );
		return QCoreApplication::translate( protocolTrContext, sourceText );
	}

	static QString qNetworkReplyTr( const char* sourceText )
	{
		return QCoreApplication::translate( "QNetworkReply", sourceText );
	}

	static QString qHttpSocketEngineTr( const char* sourceText )
	{
		return QCoreApplication::translate( "QHttpSocketEngine", sourceText );
	}

protected Q_SLOTS:
	/*! Finishes the MockReply and emits signals accordingly.
	 *
	 * This method will set the reply to finished (see setFinished()), open it for reading and emit the QNetworkReply
	 * signals according to the properties of this reply:
	 * - QNetworkReply::uploadProgress() to indicate that uploading has finished (if applicable)
	 * - QNetworkReply::metaDataChanged() to indicate that the headers of the reply are available
	 * - QIODevice::readyRead() and QNetworkReply::downloadProgress() to indicate that the downloading has finished
	 * (if applicable)
	 * - QNetworkReply::error() to indicate an error (if applicable)
	 * - QIODevice::readChannelFinished() and QNetworkReply::finished() to indicate that the reply has finished and is
	 * ready to be read
	 * - QNetworkReply::finished() to indicate that the reply is complete. Note that this signal is delayed if there
	 * is a finish delay configured.
	 *
	 * \param request The request this reply is answering.
	 */
	void finish( const Request& request )
	{
		m_finalRequest = request;
		if( FileUtils::isFileLikeScheme( m_finalRequest.qRequest.url() ) )
		{
			finishFileLikeRequest( m_finalRequest );
		}
		else
		{
			finishHttpLikeRequest( m_finalRequest );
		}

		if( this->m_finishDelaySignal.isValid() )
		{
			const int communicateFinishSlotIndex = staticMetaObject.indexOfSlot( "communicateFinish()" );
			Q_ASSERT( communicateFinishSlotIndex != -1 );
			const QMetaMethod communicateFinishSlot = staticMetaObject.method( communicateFinishSlotIndex );

			m_finishDelayConnection = m_finishDelaySignal.connect( this, communicateFinishSlot );
		}
		else
		{
			communicateFinish();
		}
	}


	/*! Tunes the behavior of this MockReply.
	 *
	 * \param behaviorFlags Combination of BehaviorFlas to define some details of this MockReply's behavior.
	 * \note Only certain BehaviorFlags have an effect on a MockReply.
	 * \sa BehaviorFlag
	 */
	void setBehaviorFlags(BehaviorFlags behaviorFlags) { m_behaviorFlags = behaviorFlags; }

private Q_SLOTS:
	void communicateFinish()
	{
		if( m_finishDelayConnection )
		{
			QObject::disconnect( m_finishDelayConnection );
		}

		this->setFinished( true );
		if( emitsFinishedSignals() )
		{
			this->emitFinishedSignals();
		}
	}

private:

	void finishFileLikeRequest( const Request& request )
	{
		if( this->error() == QNetworkReply::ProtocolUnknownError )
		{
			this->finishFileLikeRequestWithProtocolError( request );
		}
		else if( request.operation == QNetworkAccessManager::PutOperation )
		{
			this->finishFileLikePutRequest( request );
		}
		else if( request.operation == QNetworkAccessManager::HeadOperation )
		{
			this->finishFileLikeHeadRequest( request );
		}

		this->openIODeviceForRead();
	}

	void openIODeviceForRead()
	{
		// Preserve error string because it is reset by QNetworkReply::open() (see issues #37).
		const QString errorString = this->errorString();

		m_body.open( QIODevice::ReadOnly );
		QNetworkReply::open( QIODevice::ReadOnly );

		this->setError( this->error(), errorString );
	}

	void finishFileLikeRequestWithProtocolError( const Request& )
	{
		this->emitErrorSignal();
		this->emitDownloadProgressSignal( 0, 0 );
		if ( m_behaviorFlags.testFlag( Behavior_FinalUpload00Signal ) )
		{
			this->emitUploadProgressSignal( 0, 0 );
		}
	}

	void finishFileLikePutRequest( const Request& request )
	{
		m_body.setData( QByteArray() );

		const bool hasError = this->error() != QNetworkReply::NoError;
		if( hasError )
		{
			this->emitErrorSignal();
		}
		if( !hasError && !request.body.isEmpty() )
		{
			this->emitUploadProgressSignal( request );
			this->emitDownloadProgressSignal( 0, 0 );
		}
		else
		{
			this->emitDownloadProgressSignal( 0, 0 );
			this->emitUploadProgressSignal( 0, 0 );
		}
	}

	void finishFileLikeHeadRequest( const Request& )
	{
		m_body.setData( QByteArray() );
	}

	void finishHttpLikeRequest( const Request& request )
	{
		if ( !request.body.isEmpty() )
		{
			this->emitUploadProgressSignal( request );
		}

		QMetaObject::invokeMethod( this, "metaDataChanged", Qt::QueuedConnection );

		this->openIODeviceForRead();

		const qint64 replyBodySize = m_body.size();
		if ( replyBodySize > 0 )
		{
			QMetaObject::invokeMethod( this, "readyRead", Qt::QueuedConnection );
			this->emitDownloadProgressSignal( replyBodySize, replyBodySize );
		}

		if ( this->error() != QNetworkReply::NoError )
		{
			emitErrorSignal();
		}

		this->emitDownloadProgressSignal( replyBodySize, replyBodySize );
		if ( m_behaviorFlags.testFlag( Behavior_FinalUpload00Signal ) && !request.body.isEmpty() )
		{
			this->emitUploadProgressSignal( 0, 0 );
		}
	}

	void emitDownloadProgressSignal( qint64 received, qint64 total )
	{
		QMetaObject::invokeMethod( this, "downloadProgress", Qt::QueuedConnection,
				                   Q_ARG( qint64, received ), Q_ARG( qint64, total ) );
	}

	void emitUploadProgressSignal( qint64 sent, qint64 total )
	{
		QMetaObject::invokeMethod( this, "uploadProgress", Qt::QueuedConnection,
		                           Q_ARG( qint64, sent ),
		                           Q_ARG( qint64, total ) );
	}

	void emitUploadProgressSignal( const Request& request )
	{
		this->emitUploadProgressSignal( request.body.size(), request.body.size() );
	}

	void emitErrorSignal()
	{
		#if QT_VERSION < QT_VERSION_CHECK( 5,15,0 )
			QMetaObject::invokeMethod( this, "error", Qt::QueuedConnection,
									   Q_ARG( QNetworkReply::NetworkError, this->error() ) );
		#else
			QMetaObject::invokeMethod( this, "errorOccurred", Qt::QueuedConnection,
									   Q_ARG( QNetworkReply::NetworkError, this->error() ) );
		#endif
	}

	void emitFinishedSignals()
	{
		QMetaObject::invokeMethod( this, "readChannelFinished", Qt::QueuedConnection );
		QMetaObject::invokeMethod( this, "finished", Qt::QueuedConnection );
	}

	bool emitsFinishedSignals() const
	{
		if( !FileUtils::isFileLikeScheme( m_finalRequest.qRequest.url() ) )
		{
			return true;
		}

		if( m_finalRequest.operation == QNetworkAccessManager::PutOperation
			|| this->error() == QNetworkReply::ProtocolUnknownError )
		{
			return true;
		}
		return false;
	}

	QBuffer m_body;
	AttributeSet m_attributeSet;
	BehaviorFlags m_behaviorFlags;
	int m_redirectCount;
	QVector<QUrl> m_followedRedirects;
	bool m_userDefinedError;
	bool m_useDefaultErrorString;
	Request m_finalRequest;
	SignalConnectionInfo m_finishDelaySignal;
	QMetaObject::Connection m_finishDelayConnection;
};


/*! Creates MockReply objects with predefined properties.
 *
 * This class is a configurable factory for MockReply objects.
 * The \c with*() methods configure the properties of the created replies.
 * To create a reply according to the configured properties, call createReply().
 *
 * Similar to the Rule class, the MockReplyBuilder implements a chainable interface for the configuration.
 */
class MockReplyBuilder
{
public:
	/*! Creates an unconfigured MockReplyBuilder.
	 *
	 * \note Calling createReply() on an unconfigured MockReplyBuilder will return a \c Q_NULLPTR.
	 */
	MockReplyBuilder()
		: m_replyPrototype( Q_NULLPTR )
		, m_userDefinedError( false )
	{}

	/*! Creates a MockReplyBuilder by copying another one.
	 * \param other The MockReplyBuilder which is being copied.
	 */
	MockReplyBuilder( const MockReplyBuilder& other )
	{
		if ( other.m_replyPrototype )
			m_replyPrototype.reset( other.m_replyPrototype->clone() );
		m_userDefinedError = other.m_userDefinedError;
	}

	/*! Creates a MockReplyBuilder by moving another one.
	 * \param other The MockReplyBuilder which is being moved.
	 */
	MockReplyBuilder( MockReplyBuilder&& other ) noexcept : MockReplyBuilder()
	{ swap( other ); }

	/*! Destroys this MockReplyBuilder.
	 */
	~MockReplyBuilder()
	{
		// unique_ptr takes care of clean up
		// This destructor just exists to fix SonarCloud cpp:S3624
	}

	/*! Swaps this MockReplyBuilder with another one.
	 * \param other The MockReplyBuilder to be exchanged with this one.
	 */
	void swap( MockReplyBuilder& other )
	{
		m_replyPrototype.swap( other.m_replyPrototype );
		std::swap( m_userDefinedError, other.m_userDefinedError );
	}

	/*! Swaps two MockReplyBuilders.
	 * \param left One MockReplyBuilder to be exchanged.
	 * \param right The other MockReplyBuilder to be exchanged.
	 */
	friend void swap( MockReplyBuilder& left, MockReplyBuilder& right )
	{ left.swap( right ); }

	/*! Configures this MockReplyBuilder identical to another one.
	 * \param other The MockReplyBuilder whose configuration is being copied.
	 * \return \c this
	 */
	MockReplyBuilder& operator=( const MockReplyBuilder& other )
	{
		if ( this != &other )
		{
			if ( other.m_replyPrototype)
				m_replyPrototype.reset( other.m_replyPrototype->clone() );
			else
				m_replyPrototype.reset();
			m_userDefinedError = other.m_userDefinedError;
		}
		return *this;
	}

	/*! Configures this MockReplyBuilder identical to another one by moving the other one.
	 * \param other The MockReplyBuilder which is being moved.
	 * \return \c this
	 */
	MockReplyBuilder& operator=( MockReplyBuilder&& other ) noexcept
	{
		swap( other );
		return *this;
	}

	/*! Compares two MockReplyBuilders for equality.
	 * \param left One MockReplyBuilder to be compared.
	 * \param right The other MockReplyBuilder to be compared.
	 * \return \c true if \p left and \p right have the same properties configured
	 * and thus create equal MockReply objects.
	 */
	friend bool operator==( const MockReplyBuilder& left, const MockReplyBuilder& right )
	{
		if ( &left == &right )
			return true;

		const MockReply* leftReply = left.m_replyPrototype.get();
		const MockReply* rightReply = right.m_replyPrototype.get();

		if ( leftReply == rightReply )
			return true;

		if ( !leftReply || !rightReply )
			return false;

		if (   leftReply->body()           != rightReply->body()
		    || leftReply->rawHeaderPairs() != rightReply->rawHeaderPairs()
		    || leftReply->attributes()     != rightReply->attributes()
		    || leftReply->error()          != rightReply->error()
		    || leftReply->errorString()    != rightReply->errorString()
		    || leftReply->finishDelaySignal() != rightReply->finishDelaySignal() )
			return false;

		const QSet<QNetworkRequest::Attribute> attributes = leftReply->attributes().unite( rightReply->attributes() );
		QSet<QNetworkRequest::Attribute>::const_iterator iter = attributes.cbegin();
		const QSet<QNetworkRequest::Attribute>::const_iterator attributeEnd = attributes.cend();
		for ( ; iter != attributeEnd; ++iter )
		{
			if ( leftReply->attribute( *iter ) != rightReply->attribute( *iter ) )
				return false;
		}


		return true;
	}

	/*! Compares two MockReplyBuilders for inequality.
	 * \param left One MockReplyBuilder to be compared.
	 * \param right The other MockReplyBuilder to be compared.
	 * \return \c true if \p left and \p right have different properties configured
	 * and thus create different MockReply objects.
	 */
	friend bool operator!=( const MockReplyBuilder& left, const MockReplyBuilder& right )
	{
		return !( left == right );
	}

	/*! Configures this MockReplyBuilder identical to another one.
	 * This method is identical to the copy operator and exists just to provide a consistent, chainable interface.
	 * \param other The MockReplyBuilder which is being copied.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& with( const MockReplyBuilder& other )
	{ *this = other; return *this; }

	/*! Configures this MockReplyBuilder identical to another one by moving the other one.
	 *
	 * This method is identical to the move operator and exists just to provide a consistent, chainable interface.
	 *
	 * \param other The MockReplyBuilder which is being moved.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& with( MockReplyBuilder&& other )
	{ swap( other ); return *this; }

	/*! Sets the body for the replies.
	 * \param data The data used as the message body for the replies.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& withBody( const QByteArray& data )
	{
		ensureReplyPrototype()->setBody( data );
		return *this;
	}

	/*! Sets the body for the replies to a JSON document.
	 * \param json The data used as the message body for the replies.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& withBody( const QJsonDocument& json )
	{
		MockReply* proto = ensureReplyPrototype();
		proto->setBody( json.toJson( QJsonDocument::Compact ) );
		proto->setHeader( QNetworkRequest::ContentTypeHeader,
		                  QVariant::fromValue( QStringLiteral( "application/json" ) ) );
		return *this;
	}

	/*! Sets the body for the replies to the content of a file.
	 *
	 * The file needs to exist at the time this method is called because the file's
	 * content is read and stored in this MockReplyBuilder by this method.
	 *
	 * This method also tries to determine the file's MIME type using
	 * QMimeDatabase::mimeTypeForFileNameAndData() and sets
	 * the [QNetworkRequest::ContentTypeHeader] accordingly.
	 * If this does not determine the MIME type correctly or if you want to set the
	 * MIME type explicitly, use withHeader() or withRawHeader() *after* calling this method.
	 *
	 * \param filePath The path to the file whose content is used as the message body for the replies.
	 * \return A reference to this %MockReplyBuilder.
	 * \sa [QNetworkRequest::ContentTypeHeader]
	 * \sa withHeader()
	 * [QNetworkRequest::ContentTypeHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	MockReplyBuilder& withFile( const QString& filePath )
	{
		MockReply* proto = ensureReplyPrototype();

		QFile file( filePath );
		if ( file.open( QIODevice::ReadOnly ) )
		{
			const QByteArray data = file.readAll();
			file.close();
			proto->setBody( data );
			const QMimeType mimeType = QMimeDatabase().mimeTypeForFileNameAndData( filePath, data );
			proto->setHeader( QNetworkRequest::ContentTypeHeader, QVariant::fromValue( mimeType.name() ) );
		}
		return *this;
	}

	/*! Sets the status code and reason phrase for the replies.
	 *
	 * \note \parblock
	 * If the \p statusCode is an error code, this will also set the corresponding QNetworkReply::NetworkError unless
	 * it was already set using withError(). If no error string is set explicitly, a default error string based on
	 * the reason phrase will be set by the Manager before returning the reply.
	 * \endparblock
	 *
	 * \param statusCode The HTTP status code.
	 * \param reasonPhrase The HTTP reason phrase. If it is a null QString(), the default reason phrase for the
	 * \p statusCode will be used, if available and unless a reason phrase was already set.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa withError()
	 */
	MockReplyBuilder& withStatus( int statusCode = static_cast< int >( HttpStatus::OK ),
	                              const QString& reasonPhrase = QString() )
	{
		MockReply* proto = ensureReplyPrototype();
		proto->setAttribute( QNetworkRequest::HttpStatusCodeAttribute, QVariant::fromValue( statusCode ) );

		QString phrase = reasonPhrase;
		if( !phrase.isNull() || !proto->attribute( QNetworkRequest::HttpReasonPhraseAttribute ).isValid() )
		{
			if ( phrase.isNull() )
				phrase = HttpStatus::reasonPhrase( statusCode );
			proto->setAttribute( QNetworkRequest::HttpReasonPhraseAttribute, QVariant::fromValue( phrase.toUtf8() ) );
		}

		if ( HttpStatus::isError( statusCode ) && !m_userDefinedError )
			proto->setError( HttpStatus::statusCodeToNetworkError( statusCode ) );
		checkErrorAndStatusCodeConsistency();

		return *this;
	}

	/*! Sets a header for the replies.
	 *
	 * Calling this method with the same header again will override the previous value.
	 *
	 * \param header The header.
	 * \param value The value for the header.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa QNetworkReply::setHeader()
	 */
	MockReplyBuilder& withHeader( QNetworkRequest::KnownHeaders header, const QVariant& value )
	{
		ensureReplyPrototype()->setHeader( header, value );
		return *this;
	}

	/*! Sets a raw header for the replies.
	 *
	 * Calling this method with the same header again will override the previous value.
	 * To add multiple header values for the same header, concatenate the values
	 * separated by comma. A notable exception from this rule is the \c Set-Cookie
	 * header which should be separated by newlines (`\\n`).
	 *
	 * \param header The header.
	 * \param value The value for the header.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa QNetworkReply::setRawHeader()
	 */
	MockReplyBuilder& withRawHeader( const QByteArray& header, const QByteArray& value )
	{
		ensureReplyPrototype()->setRawHeader( header, value );
		return *this;
	}

	/*! Sets an attribute for the replies.
	 *
	 * Calling this method with the same attribute again will override the previous value.
	 *
	 * \param attribute The attribute.
	 * \param value The value for the attribute.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& withAttribute( QNetworkRequest::Attribute attribute, const QVariant& value )
	{
		ensureReplyPrototype()->setAttribute( attribute, value );
		return *this;
	}

	/*! Sets the error for the replies.
	 *
	 * \note \parblock
	 * If the \p error corresponds to a known HTTP status code, the reply returned by the Manager will have the
	 * corresponding HTTP status code attribute set if no status code was set explicitly (see withStatus()).\n
	 * If both the error code and the HTTP status code are set and they do not match, a warning is issued because this
	 * is a state which cannot happen with a real QNetworkReply.
	 *
	 * If no error string is set explicitly using withError( QNetworkReply::NetworkError, const QString& ), a
	 * default error string based on the reason phrase will be set by the Manager before returning the reply.
	 *
	 * Note that both the automatic setting of the HTTP status code and the error string are not reflected by the
	 * MockReply returned by createReply(). Both things are handled by the Manager class and therefore are only
	 * reflected by the replies returned from a Manager instance.
	 * \endparblock
	 *
	 * \param error The [QNetworkReply::NetworkError] code.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa withStatus()
	 * [QNetworkReply::NetworkError]: https://doc.qt.io/qt-5/qnetworkreply.html#NetworkError-enum
	 */
	MockReplyBuilder& withError( QNetworkReply::NetworkError error )
	{
		m_userDefinedError = true;
		ensureReplyPrototype()->setError( error );

		checkErrorAndStatusCodeConsistency();
		return *this;
	}

	/*! Sets the error and error string for the replies.
	 *
	 * \note In many cases, it is neither necessary nor desirable to set the error string for the reply explicitly. The
	 * Manager sets suitable default error strings for error codes when using withError( QNetworkReply::NetworkError ).
	 * However, there can be cases where the default error strings do not match those of a real QNetworkAccessManager
	 * (for example when a custom network access manager is used). In such cases, this overload allows setting an
	 * explicit error string.
	 *
	 * \param error The [QNetworkReply::NetworkError] code.
	 * \param errorString A message used as error string (see QNetworkReply::errorString()).
	 * \return A reference to this %MockReplyBuilder.
	 * [QNetworkReply::NetworkError]: https://doc.qt.io/qt-5/qnetworkreply.html#NetworkError-enum
	 *
	 * \sa withError( QNetworkReply::NetworkError )
	 */
	MockReplyBuilder& withError( QNetworkReply::NetworkError error, const QString& errorString )
	{
		m_userDefinedError = true;
		ensureReplyPrototype()->setError( error, errorString );
		checkErrorAndStatusCodeConsistency();
		return *this;
	}

	/*! Convenience method to configure redirection for the replies.
	 *
	 * This sets the [QNetworkRequest::LocationHeader] and the HTTP status code.
	 * \note Due to QTBUG-41061, the [QNetworkRequest::LocationHeader] returned by QNetworkReply::header() will be an
	 * empty (invalid) URL when \p targetUrl is relative. The redirection will still work as expected.
	 * QNetworkReply::rawHeader() always returns the correct value for the Location header.
	 *
	 * \param targetUrl The URL of the redirection target. Can be relative or absolute.
	 * If it is relative, it will be made absolute using the URL of the requests that matched the Rule as base.
	 * \param statusCode The HTTP status code to be used. Should normally be in the 3xx range.
	 * \return A reference to this %MockReplyBuilder.
	 * \sa https://bugreports.qt.io/browse/QTBUG-41061
	 * [QNetworkRequest::LocationHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	MockReplyBuilder& withRedirect( const QUrl& targetUrl, HttpStatus::Code statusCode = HttpStatus::Found )
	{
		ensureReplyPrototype()->setRawHeader( HttpUtils::locationHeader(), targetUrl.toEncoded() );
		withStatus( static_cast<int>( statusCode ) );
		return *this;
	}

	/*! Adds an HTTP authentication challenge to the replies and sets their HTTP status code to 401 (Unauthorized).
	 *
	 * \param authChallenge The authentication challenge to be added to the replies. Must be a valid Challenge or
	 * this method does not add the authentication challenge.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa HttpUtils::Authentication::Challenge::isValid()
	 * \sa QNetworkReply::setRawHeader()
	 */
	MockReplyBuilder& withAuthenticate(const HttpUtils::Authentication::Challenge::Ptr& authChallenge)
	{
		MockReply* proto = ensureReplyPrototype();
		if (authChallenge && authChallenge->isValid())
		{
			proto->setRawHeader(HttpUtils::wwwAuthenticateHeader(), authChallenge->authenticateHeader());
			withStatus(static_cast<int>(HttpStatus::Unauthorized));
		}
		return *this;
	}

	/*! Adds an HTTP Basic authentication challenge to the replies and sets their HTTP status code to
	 * 401 (Unauthorized).
	 *
	 * \param realm The realm to be used for the authentication challenge.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa withAuthenticate(const HttpUtils::Authentication::Challenge::Ptr&)
	 */
	MockReplyBuilder& withAuthenticate( const QString& realm )
	{
		HttpUtils::Authentication::Challenge::Ptr authChallenge( new HttpUtils::Authentication::Basic( realm ) );
		return withAuthenticate( authChallenge );
	}

	/*! Adds a cookie to the replies.
	 *
	 * \note \parblock
	 * - The cookie will be appended to the current list of cookies.
	 * To replace the complete list of cookies, use withHeader() and set the
	 * [QNetworkRequest::SetCookieHeader] to a QList<QNetworkCookie>.
	 * - This method does *not* check if a cookie with the same name already
	 * exists in the [QNetworkRequest::SetCookieHeader].
	 * RFC 6265 says that replies SHOULD NOT contain multiple cookies with the
	 * same name. However, to allow simulating misbehaving servers, this method
	 * still allows this.
	 * \endparblock
	 *
	 * \param cookie The cookie to be added to the replies.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 * \sa [QNetworkRequest::SetCookieHeader]
	 * [QNetworkRequest::SetCookieHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	MockReplyBuilder& withCookie( const QNetworkCookie& cookie )
	{
		MockReply* proto = ensureReplyPrototype();
		QList< QNetworkCookie > cookies = proto->header( QNetworkRequest::SetCookieHeader )
		                                       .value< QList< QNetworkCookie > >();
		cookies.append( cookie );
		proto->setHeader( QNetworkRequest::SetCookieHeader, QVariant::fromValue( cookies ) );
		return *this;
	}


	/*! Adds a delay before the QNetworkReply::finished() signal is emitted.
	 *
	 * The `finished()` signal of the replies is delay until a given signal is emitted.
	 *
	 * \note It is important that the given signal is emitted **after** the reply was returned
	 * from the manager. If the signal is emitted before the reply is returned from the manager, the reply will
	 * never emit the `finished()` signal.
	 *
	 * \param sender The QObject which emits the signal to wait for with the `finished()` signal.
	 * \param signalSignature The signature of the signal to wait for. Note that this should be given **without** using
	 * the SIGNAL() macro. So for example simply `builder.withInitialDelayUntil( someObject, "someSignal()" )`.
	 * \param connectionType The type of the connection.
	 * \return A reference to this %MockReplyBuilder.
	 *
	 */
	MockReplyBuilder& withFinishDelayUntil( QObject* sender, const char* signalSignature,
											 Qt::ConnectionType connectionType = Qt::AutoConnection )
	{
		Q_ASSERT( sender );
		const int signalIndex = sender->metaObject()->indexOfSignal( QMetaObject::normalizedSignature( signalSignature )
		                                                             .constData() );
		Q_ASSERT( signalIndex != -1 );
		return withFinishDelayUntil( sender, sender->metaObject()->method( signalIndex ), connectionType );
	}

	/*! \overload
	 *
	 * \param sender The QObject which emits the signal to wait for with the `finished()` signal.
	 * \param metaSignal The QMetaMethod of the signal.
	 * \param connectionType The type of the connection.
	 * \return A reference to this %MockReplyBuilder.
	 */
	MockReplyBuilder& withFinishDelayUntil( QObject* sender, const QMetaMethod& metaSignal,
	                                         Qt::ConnectionType connectionType = Qt::AutoConnection )
	{
		SignalConnectionInfo signalConnection( sender, metaSignal, connectionType );
		Q_ASSERT( signalConnection.isValid() );
		ensureReplyPrototype()->m_finishDelaySignal = signalConnection;
		return *this;
	}

	/*! \overload
	 *
	 * \tparam PointerToMemberFunction The type of the \p signal.
	 * \param sender The QObject which emits the signal to wait for with the `finished()` signal.
	 * \param signalPointer The signal to wait for as a function pointer.
	 * \param connectionType The type of the connection.
	 * \return A reference to this %MockReplyBuilder.
	 */
	template<typename PointerToMemberFunction>
	MockReplyBuilder& withFinishDelayUntil( QObject* sender, PointerToMemberFunction signalPointer,
	                                        Qt::ConnectionType connectionType = Qt::AutoConnection )
	{
		const QMetaMethod signalMetaMethod = QMetaMethod::fromSignal( signalPointer );
		Q_ASSERT_X( sender->metaObject()->method( signalMetaMethod.methodIndex() ) == signalMetaMethod, Q_FUNC_INFO,
		            QStringLiteral( "Signal '%1' does not belong to class '%2' of sender object." )
		            .arg( signalMetaMethod.name(), sender->metaObject()->className() ).toLatin1().constData() );
		return withFinishDelayUntil( sender, signalMetaMethod, connectionType );
	}


	/*! Creates a reply using the configured properties.
	 * \return A new MockReply with properties as configured in this factory or a Q_NULLPTR if no properties have been
	 * configured. The caller is responsible for deleting the object when it is not needed anymore.
	 */
	MockReply* createReply() const
	{
		if ( m_replyPrototype )
			return m_replyPrototype->clone();
		else
			return Q_NULLPTR;
	}

protected:
	/*! Creates a MockReply as prototype if necessary and returns it.
	 * \return A MockReply which acts as a prototype for the replies created by createReply().
	 * Modify the properties of the returned reply to change the configuration of this factory.
	 * The ownership of the returned reply stays with the MockReplyBuilder so do not delete it.
	 */
	MockReply* ensureReplyPrototype()
	{
		if ( !m_replyPrototype )
		{
			m_replyPrototype.reset( new MockReply() );
		}
		return m_replyPrototype.get();
	}

private:

	void checkErrorAndStatusCodeConsistency() const
	{
		Q_ASSERT( m_replyPrototype );
		const QVariant statusCodeVariant = m_replyPrototype->attribute( QNetworkRequest::HttpStatusCodeAttribute );
		if( !statusCodeVariant.isNull() && m_userDefinedError )
		{
			const int statusCode = statusCodeVariant.toInt();
			const QNetworkReply::NetworkError expectedError = HttpStatus::statusCodeToNetworkError( statusCode );
			if( expectedError != m_replyPrototype->error() )
			{
				qCWarning( log ) << "HTTP status code and QNetworkReply::error() do not match!"
				                 << "Status code is" << statusCode << "which corresponds to error" << expectedError
				                 << "but actual error is" << m_replyPrototype->error();
			}
		}

	}
	std::unique_ptr< MockReply > m_replyPrototype;
	bool m_userDefinedError;
};


/*! Configuration object for the Manager.
 *
 * The Rule combines predicates for matching requests with a MockReplyBuilder which generates MockReplies when the
 * predicates match.
 *
 * ### Usage ###
 * The Rule implements a chainable interface. This means that the methods return a reference to the Rule
 * itself to allow calling its methods one after the other in one statement.
 * Additionally, the Manager provides convenience methods to create Rule objects.
 * So the typical way to work with Rules is:
\code
using namespace MockNetworkAccess;
using namespace MockNetworkAccess::Predicates;
Manager< QNetworkAccessManager > mockNAM;

mockNAM.whenGet( QUrl( "http://example.com" ) )
       .has( HeaderMatching( QNetworkRequest::UserAgentHeader, QRegularExpression( ".*MyWebBrowser.*" ) ) )
       .reply().withBody( QJsonDocument::fromJson( "{\"response\": \"hello\"}" ) );
\endcode
 *
 * \note Rule objects cannot be copied but they can be cloned. See clone().
 *
 * ### Matching ###
 * To add predicates to a Rule, use the has() and hasNot() methods.
 * For a Rule to match a request, all its predicates must match. So the predicates have "and" semantics.
 * To achieve "or" semantics, configure multiple Rule in the Manager or implement a dynamic predicate (see
 * \ref page_dynamicMockNam_dynamicPredicates).
 * Since the first matching Rule in the Manager will be used to create a reply, this provides "or" semantics.
 * In addition to negating single Predicates (see hasNot() or Predicate::negate()), the matching of the whole Rule
 * object can be negated by calling negate().
 * \note
 * \parblock
 * The order of the Predicates in a Rule has an impact on the performance of the matching.
 * So, fast Predicates should be added before complex Predicates (for example, Predicates::Header before
 * Predicates::BodyMatching).
 * \endparblock
 *
 * ### Creating Replies ###
 * When a Rule matches a request, the Manager will request it to create a reply for the request.
 * The actual creation of the reply will be done by the Rule's MockReplyBuilder which can be accessed through the
 * reply() method.
 *
 * ### Extending Rule ###
 * Both the matching of requests and the generation of replies can be extended and customized.
 * To extend the matching, implement new Predicate classes.
 * To extend or customize the generation of replies, override the createReply() method. You can then use a
 * MockReplyBuilder to create a reply based on the request.
 * These extension possibilities allow implementing dynamic matching and dynamic replies. That is, depending on the
 * concrete values of the request, the matching behaves differently or the reply has different properties.
 * This also allows introducing state and effectively evolves the Rule into a simple fake server.\n
 * See \ref page_dynamicMockNam for further details.
 */
class Rule
{
	template< class Base >
	friend class Manager;

public:

	/*! Smart pointer to a Rule object. */
	typedef QSharedPointer< Rule > Ptr;

	/*! Abstract base class for request matching.
	 * A Predicate defines a condition which a request must match.
	 * If all Predicates of a Rule match the request, the Rule is
	 * considered to match the request.
	 *
	 * To create custom Predicates, derive from this class and implement the private match() method.
	 */
	class Predicate
	{
	public:
		/*! Smart pointer to a Predicate. */
		typedef QSharedPointer< Predicate > Ptr;

		/*! Default constructor
		 */
		Predicate() : m_negate(false) {}

		/*! Default destructor
		 */
		virtual ~Predicate() {}

		/*! Matches a request against this Predicate.
		 * \param request The request to test against this predicate.
		 * \return \c true if the Predicate matches the \p request.
		 */
		bool matches( const Request& request )
		{
			return match( request ) != m_negate;
		}

		/*! Negates the matching of this Predicate.
		 * \param negate If \c true, the result of matches() is negated before returned.
		 */
		void negate( bool negate = true )
		{
			m_negate = negate;
		}


	private:
		/*! Performs the actual matching.
		 * This method is called by matches() to do the actual matching.
		 * \param request The request to be tested to match this %Predicate.
		 * \return Must return \c true if the Predicate matches the \p request. Otherwise, \c false.
		 */
		virtual bool match( const Request& request ) = 0;

		bool m_negate;
	};

	/*! This enum defines the behaviors of a Rule regarding passing matching requests through to the next network access
	 * manager.
	 */
	enum PassThroughBehavior
	{
		DontPassThrough,                /*!< The rule consumes matching requests and the Manager returns a MockReply
		                                 * generated by the MockReplyBuilder of the rule (see reply()).
		                                 * The request is **not** passed through.\n
		                                 * This is the default behavior.
		                                 */
		PassThroughReturnMockReply,     /*!< The rule passes matching requests through to the next network access
		                                 * manager but the Manager still returns a MockReply generated by the
		                                 * MockReplyBuilder of the rule (see reply()).
		                                 * The reply returned by the next network access manager is discarded.
		                                 * \note If the rule has no reply() configured, matching requests will not
		                                 * be passed through since the Rule is considered "invalid" by the Manager.
		                                 */
		PassThroughReturnDelegatedReply /*!< The rule passes matching requests through to the next network access
		                                 * manager and the Manager returns the reply returned by the next network
		                                 * access manager.
		                                 */
	};

	/*! Creates a Rule which matches every request but creates no replies.
	 *
	 * In regard to the Manager, such a Rule is invalid and is ignored by the Manager.
	 * To make it valid, configure the MockReplyBuilder returned by reply().
	 * \sa Manager
	 */
	Rule()
		: m_negate( false )
		, m_passThroughBehavior( DontPassThrough )
	{
	}

	/*! Deleted copy constructor.
	 */
	Rule( const Rule& ) = delete;

	/*! Default move operator.
	 */
	Rule( Rule&& ) = default;

	/*! Default destructor.
	 */
	virtual ~Rule() = default;

	/*! Deleted assignment operator.
	 */
	Rule& operator=( const Rule& ) = delete;

public:
	/*! Negates the matching of this rule.
	 * \param negate If \c true, the result of the matching is negated, meaning if _any_ of the predicates does _not_
	 * match, this Rule matches.
	 * If \c false, the negation is removed reverting to normal "and" semantics.
	 * \return A reference to this %Rule.
	 * \sa matches()
	 */
	Rule& negate( bool negate = true )
	{ m_negate = negate; return *this; }

	/*! \return \c true if this rule negates the matching. \c false otherwise.
	 *
	 * \sa negate()
	 */
	bool isNegated() const
	{
		return m_negate;
	}

	/*! Adds a Predicate to the Rule.
	 * \tparam PredicateType The type of the \p predicate. \p PredicateType must be move-constructable (if
	 * \p predicate is an rvalue reference) or copy-constructable (if \p predicate is an lvalue reference) for
	 * this method to work.
	 * \param predicate The Predicate to be added to the Rule.
	 * Note that \p predicate will be copied/moved and the resulting Predicate is actually added to the Rule.
	 * \return A reference to this %Rule.
	 */
	template< class PredicateType >
	Rule& has( PredicateType&& predicate )
	{
		m_predicates.append( Predicate::Ptr(
			new typename std::remove_const< typename std::remove_reference< PredicateType >::type >::type(
			std::forward< PredicateType >( predicate ) ) ) );
		return *this;
	}


	/*! Adds a Predicate to the Rule.
	 * \param predicate Smart pointer to the Predicate to be added to the Rule.
	 * \return A reference to this %Rule.
	 */
	Rule& has( const Predicate::Ptr& predicate )
	{
		m_predicates.append( predicate );
		return *this;
	}

	/*! Negates a Predicate and adds it to the Rule.
	 * \tparam PredicateType The type of the \p predicate. \p PredicateType must be move-constructable (if
	 * \p predicate is an rvalue reference) or copy-constructable (if \p predicate is an lvalue reference) for
	 * this method to work.
	 * \param predicate The Predicate to be negated and added to the Rule.
	 * Note that \p predicate will be copied and the copy is negated and added.
	 * \return A reference to this %Rule.
	 */
	template< class PredicateType >
	Rule& hasNot( PredicateType&& predicate )
	{
		Predicate::Ptr copy(
			new typename std::remove_const< typename std::remove_reference< PredicateType >::type >::type(
			std::forward< PredicateType >( predicate ) ) );
		copy->negate();
		m_predicates.append( copy );
		return *this;
	}

	/*! Negates a Predicate and adds it to the Rule.
	 * \param predicate Smart pointer to the Predicate to be negated and added to the Rule.
	 * \return A reference to this %Rule.
	 * \sa Predicate::negate()
	 */
	Rule& hasNot( const Predicate::Ptr& predicate )
	{
		predicate->negate();
		m_predicates.append( predicate );
		return *this;
	}

	/*! Creates a \link Predicates::Generic Generic Predicate \endlink and adds it to this Rule.
	 *
	 * Example:
	 * \code
	 * Manager< QNetworkAccessManager > mnam;
	 * mnam.whenPost( QUrl( "http://example.com/json" ) )
	 *     .isMatching( [] ( const Request& request ) -> bool {
	 *         if ( request.body.isEmpty()
	 *           || request.qRequest.header( QNetworkRequest::ContentTypeHeader ).toString() != "application/json" )
	 *             return true;
	 *         QJsonDocument jsonDoc = QJsonDocument::fromJson( request.body );
	 *         return jsonDoc.isNull();
	 *     } )
	 *     .reply().withError( QNetworkReply::ProtocolInvalidOperationError, "Expected a JSON body" );
	 * \endcode
	 *
	 * \tparam Matcher The type of the callable object.
	 * \param matcher The callable object used to create the Generic predicate.
	 * \return A reference to this %Rule.
	 * \sa isNotMatching()
	 * \sa Predicates::Generic
	 * \sa Predicates::createGeneric()
	 */
	template<class Matcher>
	Rule& isMatching( const Matcher& matcher );

	/*! Creates a \link Predicates::Generic Generic Predicate \endlink, negates it and adds it to this Rule.
	 *
	 * See isMatching() for a usage example.
	 *
	 * \tparam Matcher The type of the callable object.
	 * \param matcher The callable object used to create the Generic predicate.
	 * \return A reference to this %Rule.
	 * \sa isMatching()
	 * \sa Predicates::Generic
	 * \sa Predicates::createGeneric()
	 */
	template<class Matcher>
	Rule& isNotMatching( const Matcher& matcher );

	/*! \return The predicates of this Rule.
	 */
	QVector< Predicate::Ptr > predicates() const { return m_predicates; }

	/*! Sets the predicates of this Rule.
	 * This removes all previous Predicates of this Rule.
	 * \param predicates The new Predicates for this Rule.
	 */
	void setPredicates( const QVector< Predicate::Ptr >& predicates ) { m_predicates = predicates; }

	/*! \return The MockReplyBuilder used to create replies in case this Rule matches. Use the returned builder to
	 * configure the replies.
	 */
	MockReplyBuilder& reply()
	{ return m_replyBuilder; }

	/*! Defines whether matching requests should be passed through to the next network access manager.
	 * \param behavior How the Rule should behave in regard to passing requests through.
	 * \param passThroughManager The network access manager to which requests are passed through.
	 * If this is null, the pass through manager of this Rule's manager is used to pass requests through (see
	 * Manager::setPassThroughNam()).
	 * \n **Since** 0.4.0
	 * \return A reference to this %Rule.
	 * \sa PassThroughBehavior
	 * \sa \ref page_passThrough
	 */
	Rule& passThrough( PassThroughBehavior behavior = PassThroughReturnDelegatedReply,
	                   QNetworkAccessManager* passThroughManager = Q_NULLPTR )
	{
		m_passThroughBehavior = behavior;
		m_passThroughManager = passThroughManager;
		return *this;
	}

	/*! \return Whether this rule passes matching requests through to the next network access manager and what
	 * is returned by the Manager if the request is passed through.
	 *
	 * \sa PassThroughBehavior
	 */
	PassThroughBehavior passThroughBehavior() const
	{
		return m_passThroughBehavior;
	}

	/*! \return The network access manager to which matching requests are passed through.
	 * \sa passThrough()
	 * \sa PassThroughBehavior
	 * \since 0.4.0
	 */
	QNetworkAccessManager* passThroughManager() const
	{
		return m_passThroughManager;
	}

	/*! Matches a request against the predicates of this Rule.
	 * \param request The request to be tested against the predicates.
	 * \return \c true if the \p request matches all predicates.
	 */
	bool matches( const Request& request ) const
	{
		bool returnValue = true;

		QVector< Predicate::Ptr >::const_iterator iter = m_predicates.cbegin();
		const QVector< Predicate::Ptr >::const_iterator end = m_predicates.cend();
		for ( ; iter != end; ++iter )
		{
			if ( ! ( *iter )->matches( request) )
			{
				returnValue = false;
				break;
			}
		}

		return returnValue != m_negate;
	}

	/*! Creates a MockReply using the MockReplyBuilder of this Rule.
	 *
	 * The base implementation simply calls MockReplyBuilder::createReply().
	 *
	 * \note When you reimplement this method, you can also return a null pointer. In that case, it is treated as if the
	 * Rule didn't match the request. This is useful if you create the replies dynamically and get into a
	 * situation where you cannot generate an appropriate reply.
	 *
	 * \param request The request to be answered.
	 * \return A new MockReply object created by the MockReplyBuilder (see reply()).
	 * The caller takes ownership of the returned MockReply and should delete it
	 * when it is not needed anymore.
	 */
	virtual MockReply* createReply( const Request& request )
	{
		Q_UNUSED( request )
		return m_replyBuilder.createReply();
	}

	/*! Creates a clone of this Rule.
	 *
	 * \return A Rule object with the same properties as this Rule except for the matchedRequests().
	 * Note that the predicates() are shallow copied meaning that this Rule and the clone will have pointers to
	 * the same Predicate objects. All other properties except for the matchedRequests() are copied.
	 * The caller is taking ownership of the returned Rule object and should delete it when it is not needed
	 * anymore.
	 */
	virtual Rule* clone() const
	{
		Rule* cloned = new Rule();
		cloned->m_predicates = m_predicates;
		cloned->m_replyBuilder = m_replyBuilder;
		cloned->m_negate = m_negate;
		cloned->m_passThroughBehavior = m_passThroughBehavior;
		cloned->m_passThroughManager = m_passThroughManager;
		return cloned;
	}

	/*! \return The requests that matched this Rule.
	 */
	QVector< Request > matchedRequests() const
	{
		return m_matchedRequests;
	}


private:
	QVector< Predicate::Ptr > m_predicates;
	MockReplyBuilder m_replyBuilder;
	bool m_negate;
	PassThroughBehavior m_passThroughBehavior;
	QVector< Request > m_matchedRequests;
	QPointer< QNetworkAccessManager > m_passThroughManager;
};


/*! Namespace for the matching predicates provided by the MockNetworkAccessManager library.
 * \sa Rule::Predicate
 */
namespace Predicates
{
	/*! Matches any request.
	 * This is useful to handle unexpected requests.
	 */
	class Anything : public Rule::Predicate
	{
	public:
		/*! Creates a predicate which matches any request.
		 */
		Anything() : Predicate() {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& ) Q_DECL_OVERRIDE
		{ return true; }
		//! \endcond
	};

	/*! Matches if a given callable object matches the request.
	 *
	 * Normally, this class does not need to be used directly since there are the
	 * convenience methods Rule::isMatching() and Rule::isNotMatching().
	 *
	 * If this class should still be used directly and the compiler does not support
	 * class template argument deduction, there is the convenience method createGeneric().
	 *
	 * \tparam Matcher A callable type which is used to match the request.
	 * The \p Matcher must accept a `const Request&` as parameter and return a `bool`.
	 * When the predicate is tested against a request, the \p Matcher is invoked
	 * and its return value defines whether this predicate matches.
	 *
	 * \sa createGeneric()
	 * \sa \ref page_dynamicMockNam_dynamicPredicates_examples_2
	 */
	template< class Matcher >
	class Generic : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching using a callable object.
		 * \param matcher The callable object which is invoked to match the request.
		 */
		explicit Generic( const Matcher& matcher ) : Predicate(), m_matcher( matcher ) {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			return m_matcher( request );
		}
		//! \endcond

		Matcher m_matcher;
	};

	/*! Creates a Generic predicate.
	 * This factory method mainly exists to take advantage of template argument deduction when creating a Generic
	 * predicate.
	 * \tparam Matcher The type of the callable object. Must take a single \c const Request& parameter and
	 * return a \c bool.
	 * \param matcher The callable object. Must return \c true if the predicate matches the Request given as parameter.
	 * \return A smart pointer to a Generic predicate created with \p matcher.
	 * \sa Generic
	 */
	template< class Matcher >
	inline Rule::Predicate::Ptr createGeneric( const Matcher& matcher )
	{
		return Rule::Predicate::Ptr( new Generic< Matcher >( matcher ) );
	}

	/*! Matches if the HTTP verb equals a given verb.
	 */
	class Verb : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching the HTTP verb.
		 * \param operation The verb to match.
		 * \param customVerb If \p operation is QNetworkAccessManager::CustomOperation, \p customVerb defines the
		 * custom verb to match.
		 * In other cases, this parameter is ignored.
		 */
		explicit Verb( QNetworkAccessManager::Operation operation,
		               const QByteArray& customVerb = QByteArray() )
			: Predicate(), m_operation(operation)
		{
			if (m_operation == QNetworkAccessManager::CustomOperation)
				m_customVerb = customVerb;
		}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			if (request.operation != m_operation)
				return false;
			if (request.operation == QNetworkAccessManager::CustomOperation
			    && request.qRequest.attribute(QNetworkRequest::CustomVerbAttribute).toByteArray() != m_customVerb)
				return false;
			return true;
		}
		//! \endcond

		QNetworkAccessManager::Operation m_operation;
		QByteArray m_customVerb;
	};

	/*! Matches if the request URL matches a regular expression.
	 * \note To match query parameters, it is typically easier to use the predicate QueryParameters.
	 */
	class UrlMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching the request URL against a regular expression.
		 * \param urlRegEx The regular expression.
		 * \param format QUrl::FormattingOptions to be used to convert the QUrl to a QString when matching the regular
		 * expression.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrl::toString().
		 * Note that QUrl::FullyDecoded does *not* work since QUrl::toString() does not permit it.
		 */
		explicit UrlMatching( const QRegularExpression& urlRegEx,
		                      QUrl::FormattingOptions format = QUrl::FormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_urlRegEx( urlRegEx )
			, m_format( format )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QString url = request.qRequest.url().toString(m_format);
			return m_urlRegEx.match(url).hasMatch();
		}
		//! \endcond

		QRegularExpression m_urlRegEx;
		QUrl::FormattingOptions m_format;
	};

	/*! Matches if the request URL equals a given URL.
	 * \note This predicate does an exact matching of the URL so it is stricter than the other URL predicates.
	 */
	class Url : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching if the request URL equals a given URL.
		 * \note Invalid QUrls are treated like empty QUrls for the comparison.
		 * In other words, the following QUrl objects are all considered equal: `QUrl()`, `QUrl("")`,
		 * `QUrl("http://..")`, `QUrl("http://!!")`
		 * \param url The URL which is compared against the request URL.
		 * \param defaultPort Allows defining a default port to be considered when the request or \p url does not
		 * specify a port explicitly.
		 * The default ports for HTTP (80), HTTPS (443) and FTP (21) are used when no \p defaultPort was
		 * specified (that is, when \p defaultPort is -1) and the \p url has a matching scheme.
		 */
		explicit Url( const QUrl& url, int defaultPort = -1 ) : Predicate(), m_url( url ), m_defaultPort( defaultPort )
		{
			detectDefaultPort();
		}

		/*! \overload
		 *
		 * \param url The URL compared against the request URL. If it is empty, it always matches.
		 * \param defaultPort Allows defining a default port to be considered when the request or \p url does not
		 * specify a port explicitly.
		 * The default ports for HTTP (80) and HTTPS (443) are used when no \p defaultPort was specified.
		 */
		explicit Url( const QString& url, int defaultPort = -1 ) : Predicate(), m_url( url ), m_defaultPort( defaultPort )
		{
			detectDefaultPort();
		}

	private:
		void detectDefaultPort()
		{
			if ( m_defaultPort == -1 )
			{
				const QString urlProtocol = m_url.scheme().toLower();
				if ( urlProtocol == HttpUtils::httpScheme() )
					m_defaultPort = HttpUtils::HttpDefaultPort;
				else if ( urlProtocol == HttpUtils::httpsScheme() )
					m_defaultPort = HttpUtils::HttpsDefaultPort;
				else if ( urlProtocol == FtpUtils::ftpScheme() )
					m_defaultPort = FtpUtils::FtpDefaultPort;
			}
		}

		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QUrl requestUrl = request.qRequest.url();
			return ( requestUrl == m_url )
			    || ( m_defaultPort > -1
			        /* QUrl::matches() could be used here instead of QUrl::adjusted() but it is buggy:
			         * https://bugreports.qt.io/browse/QTBUG-70774
			        && m_url.matches(requestUrl, QUrl::RemovePort)
			         */
			        && m_url.adjusted( QUrl::RemovePort ) == requestUrl.adjusted( QUrl::RemovePort )
			        && m_url.port( m_defaultPort ) == requestUrl.port( m_defaultPort ) );
		}
		//! \endcond

		QUrl m_url;
		int m_defaultPort;
	};

	/*! Matches if the request URL contains a given query parameter.
	 * Note that the URL can contain more query parameters. This predicate just checks that the given parameter exists
	 * with the given value.
	 *
	 * This predicate is especially useful in combination with the regular expression predicate UrlMatching()
	 * since query parameters typically don't have a defined order which makes it very hard to match them with regular
	 * expressions.
	 */
	class QueryParameter : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a URL query parameter.
		 * \param key The name of the query parameter.
		 * \param value The value that the query parameter needs to have.
		 * \param format QUrl::ComponentFormattingOptions used to convert the query parameter value to a QString.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrlQuery::queryItemValue().
		 */
		explicit QueryParameter( const QString& key, const QString& value,
		                         QUrl::ComponentFormattingOptions format
		                             = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_key( key )
			, m_values( value )
			, m_format( format )
		{
		}

		/*! Creates a predicate matching a URL query parameter with a list of values.
		 * \param key The name of the query parameter.
		 * \param values The values that the query parameter needs to have in the order they appear in the query.
		 * \param format QUrl::ComponentFormattingOptions used to convert the query parameter value to a QString.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrlQuery::queryItemValue().
		 * \since 0.4.0
		 */
		explicit QueryParameter( const QString& key, const QStringList& values,
		                         QUrl::ComponentFormattingOptions format
		                             = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_key(key)
			, m_values(values)
			, m_format(format)
		{
		}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QUrlQuery query(request.qRequest.url());
			return query.hasQueryItem(m_key) && query.allQueryItemValues(m_key, m_format) == m_values;
		}
		//! \endcond

		QString m_key;
		QStringList m_values;
		QUrl::ComponentFormattingOptions m_format;
	};

	/*! Matches if the request URL contains a given query parameter with a value matching a given regular expression.
	 * If the query parameter contains multiple values, **all** of its values must match the given regular expression.
	 *
	 * Note that the URL can contain more query parameters. This predicate just checks that the given parameter exists
	 * with a matching value.
	 *
	 * This predicate is especially useful in combination with the regular expression predicate UrlMatching()
	 * since query parameters typically don't have a defined order which makes it very hard to match them with regular
	 * expressions.
	 */
	class QueryParameterMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching an URL query parameter value .
		 * \param key The name of the query parameter.
		 * \param regEx The regular expression matched against the query parameter value.
		 * \param format QUrl::ComponentFormattingOptions to be used to convert the query parameter value to a QString
		 * when matching the regular expression. The default is QUrl::PrettyDecoded since it is also the default for
		 * QUrlQuery::queryItemValue().
		 */
		explicit QueryParameterMatching( const QString& key, const QRegularExpression& regEx,
		                                 QUrl::ComponentFormattingOptions format
		                                     = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_key( key )
			, m_regEx( regEx )
			, m_format( format )
		{
		}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QUrlQuery query( request.qRequest.url() );
			if ( !query.hasQueryItem( m_key ) )
				return false;

			const QStringList values = query.allQueryItemValues( m_key );
			QStringList::const_iterator iter = values.cbegin();
			const QStringList::const_iterator end = values.cend();
			for ( ; iter != end; ++iter )
			{
				if ( ! m_regEx.match( *iter ).hasMatch() )
					return false;
			}
			return true;
		}
		//! \endcond

		QString m_key;
		QRegularExpression m_regEx;
		QUrl::ComponentFormattingOptions m_format;
	};

	/*! Matches if the request URL contains given query parameters.
	 * Note that the URL can contain more query parameters. This predicate just checks that the given parameters exist
	 * with the given values.
	 *
	 * This predicate is especially useful in combination with the regular expression predicate UrlMatching()
	 * since query parameters typically don't have a defined order which makes it very hard to match them with regular
	 * expressions.
	 */
	class QueryParameters : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching URL query parameters.
		 * \param parameters A QHash of query parameters that need to be present in the URL with defined values.
		 * The keys of the hash are the expected parameter names and the corresponding values of the hash are the
		 * expected parameter values.
		 * \param format QUrl::ComponentFormattingOptions used to convert the query parameter value to a QString.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrlQuery::queryItemValue().
		 */
		explicit QueryParameters( const QueryParameterHash& parameters,
		                          QUrl::ComponentFormattingOptions format
		                              = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_format( format )
		{
			QueryParameterHash::const_iterator iter = parameters.cbegin();
			const QueryParameterHash::const_iterator end = parameters.cend();
			for ( ; iter != end; ++iter )
			{
				m_queryParameters.insert( iter.key(), QStringList() << iter.value() );
			}
		}

		/*! Creates a predicate matching URL query parameters.
		 * \param parameters A QHash of query parameters that need to be present in the URL with defined values.
		 * The keys of the hash are the expected parameter names and the corresponding values of the hash are the
		 * expected parameter values in the order they appear in the query.
		 * \param format QUrl::ComponentFormattingOptions used to convert the query parameter value to a QString.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrlQuery::queryItemValue().
		 * \since 0.4.0
		 */
		explicit QueryParameters( const MultiValueQueryParameterHash& parameters,
		                          QUrl::ComponentFormattingOptions format
		                              = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_queryParameters( parameters )
			, m_format( format )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QUrlQuery query( request.qRequest.url() );
			MultiValueQueryParameterHash::const_iterator iter = m_queryParameters.cbegin();
			const MultiValueQueryParameterHash::const_iterator end = m_queryParameters.cend();
			for( ; iter != end; ++iter )
			{
				if ( !query.hasQueryItem( iter.key() )
				    || query.allQueryItemValues( iter.key(), m_format ) != iter.value() )
				{
					return false;
				}
			}
			return true;
		}
		//! \endcond

		MultiValueQueryParameterHash m_queryParameters;
		QUrl::ComponentFormattingOptions m_format;
	};

	/*! Matches if *all* URL query parameters match one of the given regular expression pairs.
	 *
	 * This predicates checks all URL query parameters against the given regular expression pairs in the order
	 * they are given. If the first regular expression of a pair matches the name of the query parameter, then the
	 * second regular expression must match the value of the parameter. If the value does not match or if the parameter
	 * name does not match any of the first regular expressions of the pairs, then the predicate does not match.
	 * If all query parameter names match one of the first regular expressions and the parameter values match the
	 * corresponding second regular expression, then this predicate matches.
	 *
	 * Note that for parameters with multiple values, all values of the parameter need to match the second regular
	 * expression.
	 *
	 * This predicate can be used to ensure that there are not unexpected query parameters.
	 */
	class QueryParameterTemplates : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching all query parameters against regular expression pairs.
		 *
		 * \param templates QVector of QRegularExpression pairs. The first regular expressions are matched against the
		 * query parameter names and the second regular expressions are matched against the query parameter values.
		 * \param format QUrl::ComponentFormattingOptions used to convert the query parameter value to a QString.
		 * The default is QUrl::PrettyDecoded since it is also the default for QUrlQuery::queryItemValue().
		 */
		explicit QueryParameterTemplates( const RegExPairVector& templates,
		                                  QUrl::ComponentFormattingOptions format
		                                      = QUrl::ComponentFormattingOptions( QUrl::PrettyDecoded ) )
			: Predicate()
			, m_templates( templates )
			, m_format( format )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			typedef QList< QPair< QString, QString > > StringPairList;

			const QUrlQuery query( request.qRequest.url() );
			const StringPairList queryParams = query.queryItems( m_format );

			StringPairList::const_iterator queryParamsIter = queryParams.cbegin();
			const StringPairList::const_iterator queryParamsEnd = queryParams.cend();
			for ( ; queryParamsIter != queryParamsEnd; ++queryParamsIter )
			{
				bool matched = false;

				RegExPairVector::const_iterator templateIter = m_templates.cbegin();
				const RegExPairVector::const_iterator templateEnd = m_templates.cend();
				for ( ; templateIter != templateEnd; ++templateIter )
				{
					if ( templateIter->first.match( queryParamsIter->first ).hasMatch() )
					{
						matched = templateIter->second.match( queryParamsIter->second ).hasMatch();
						break;
					}
				}

				if ( !matched )
					return false;
			}

			return true;
		}
		//! \endcond

		RegExPairVector m_templates;
		QUrl::ComponentFormattingOptions m_format;
	};

	/*! Matches if the request body matches a regular expression.
	 *
	 * To match against the regular expression, the body needs to be converted to a QString.
	 * If a \p codec is provided in the constructor, it is used to convert the body.
	 * Else, the predicate tries to determine the codec from the [QNetworkRequest::ContentTypeHeader][]:
	 * - If the content type header contains codec information using the `"charset:<CODEC>"` format, this codec is used,
	 * if supported.
	 *   - If the codec is not supported, a warning is printed and the predicate falls back to Latin-1.
	 * - If the content type header does not contain codec information, the MIME type is investigated.
	 *   - If the MIME type is known and
	 * inherits from `text/plain`, the predicate uses QTextCodec::codecForUtfText() to detect the codec and falls back
	 * to UTF-8 if the codec cannot be detected.
	 * - In all other cases, including the case that there is no content type header at all and the case that the
	 * content is binary, the predicate uses QTextCodec::codecForUtfText() to detect the codec and falls back to
	 * Latin-1 if the codec cannot be detected.
	 * \note
	 * \parblock
	 * When trying to match without using the correct codec, (for example, when matching binary content), the regular
	 * expression patterns must be aware of the codec mismatch. In such cases, the best approach is to use the
	 * numerical value of the encoded character.
	 * For example, matching the character "ç" (LATIN SMALL LETTER C WITH CEDILLA) encoded in UTF-8 when the predicate
	 * uses Latin-1 encoding would require the pattern \c "Ã§" assuming the pattern itself is encoded using UTF-8.
	 * Since this can lead to mistakes easily, one should rather use the pattern \c "\\xC3\\x83".
	 * \endparblock
	 *
	 * \sa QMimeDatabase
	 * [QNetworkRequest::ContentTypeHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	class BodyMatching : public Rule::Predicate
	{
	public:

		/*! Creates a predicate matching the request body using a regular expression.
		 * \param bodyRegEx The regular expression to match against the request body.
		 * \param decoder The decoder to be used to convert the body into a QString. If null, the predicate
		 * tries to determine the codec based on the [QNetworkRequest::ContentTypeHeader] or based on the
		 * request body. The BodyMatching instance does **not** take ownership of the \p decoder.
		 * [QNetworkRequest::ContentTypeHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
		 */
		explicit BodyMatching( const QRegularExpression& bodyRegEx, StringDecoder decoder = StringDecoder() )
			: Predicate()
			, m_bodyRegEx( bodyRegEx )
			, m_decoder( decoder )
			, m_charsetFieldRegEx( QStringLiteral( "charset:(.*)" ) )
		{}

	private:

		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			if ( !m_decoder.isValid() )
				determineDecoder( request );

			const QString decodedBody = m_decoder.decode( request.body );

			return m_bodyRegEx.match( decodedBody ).hasMatch();
		}

		void determineDecoder( const Request& request ) const
		{
			determineDecoderFromContentType( request );

			if ( !m_decoder.isValid() )
				determineDecoderFromBody( request.body );
		}

		void determineDecoderFromContentType( const Request& request ) const
		{
			const QString contentTypeHeader = request.qRequest.header( QNetworkRequest::ContentTypeHeader ).toString();
			if( contentTypeHeader.isEmpty() )
				return;

			QStringList contentTypeFields = contentTypeHeader.split( QChar::fromLatin1( ';' ) );
			const int charsetFieldIndex = contentTypeFields.indexOf( m_charsetFieldRegEx );
			if ( charsetFieldIndex >= 0 )
			{
				const QString& charsetField = contentTypeFields.at( charsetFieldIndex );
				const QString charset = HttpUtils::trimmed( m_charsetFieldRegEx.match( charsetField ).captured( 1 ) );
				determineDecoderFromCharset( charset );
			}
			else
			{
				const QMimeType mimeType = QMimeDatabase().mimeTypeForName( contentTypeFields.first() );
				if ( mimeType.inherits( QStringLiteral( "text/plain" ) ) )
					determineDecoderFromBody( request.body, QStringLiteral( "utf-8" ) );
			}
		}

		void determineDecoderFromCharset( const QString& charset ) const
		{
			m_decoder.setCodec( charset );
			if ( !m_decoder.isValid() )
			{
				qCWarning( log ) << "Unsupported charset:" << charset;
				useFallbackDecoder();
			}
		}

		void determineDecoderFromBody( const QByteArray& body, const QString& fallbackCodec = QStringLiteral( "Latin-1" ) ) const
		{
			m_decoder.setCodecFromData( body, fallbackCodec );
			Q_ASSERT( m_decoder.isValid() );
		}

		void useFallbackDecoder() const
		{
			m_decoder.setCodec( QStringLiteral( "Latin-1" ) );
			Q_ASSERT( m_decoder.isValid() );
		}

		QRegularExpression m_bodyRegEx;
		mutable StringDecoder m_decoder;
		QRegularExpression m_charsetFieldRegEx;
	};

	/*! Match if the request body contains a given snippet.
	 */
	class BodyContaining : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a snippet in the request body.
		 * \param bodySnippet The byte sequence that needs to exist in the request body.
		 */
		explicit BodyContaining(const QByteArray& bodySnippet) : Predicate(), m_bodySnippet(bodySnippet) {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			return request.body.contains(m_bodySnippet);
		}
		//! \endcond

		QByteArray m_bodySnippet;
	};

	/*! Matches if the request body equals a given body.
	 * \note This predicate does an exact matching so it is stricter than the
	 * other body predicates.
	 */
	class Body : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching the request body.
		 * \param body The body to be compared to the request body.
		 */
		explicit Body(const QByteArray& body) : Predicate(), m_body(body) {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			return request.body == m_body;
		}
		//! \endcond

		QByteArray m_body;
	};

	/*! Matches if the request contains given headers.
	 * Note that the request can contain more headers. This predicate just checks that the given headers exist with the
	 * given values.
	 * \note For this predicate to work correctly, the type of the header field must be registered with
	 * qRegisterMetaType() and QMetaType::registerComparators() or QMetaType::registerEqualsComparator().
	 * \sa QNetworkRequest::header()
	 */
	class Headers : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a set of request headers.
		 * \param headers QHash of headers that need to be present in the request
		 * with defined values. The keys of the hash are the names of the expected
		 * headers and the corresponding values of the hash are the expected values
		 * of the headers.
		 */
		explicit Headers( const HeaderHash& headers ) : Predicate(), m_headers( headers ) {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			HeaderHash::const_iterator iter = m_headers.cbegin();
			const HeaderHash::const_iterator end = m_headers.cend();
			for ( ; iter != end; ++iter )
			{
				if ( request.qRequest.header( iter.key() ) != iter.value() )
					return false;
			}
			return true;
		}
		//! \endcond

		HeaderHash m_headers;
	};

	/*! Match if the request contains a given header.
	 * Note that the request can contain more headers. This predicate just checks that the given header exists with the
	 * given value.
	 * \note For this predicate to work correctly, the type of the header field must be registered with
	 * qRegisterMetaType() and QMetaType::registerComparators() or QMetaType::registerEqualsComparator().
	 * \sa QNetworkRequest::header()
	 */
	class Header : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a request header.
		 * \param header The header that needs to be present in the request.
		 * \param value The value that the \p header needs to have.
		 */
		explicit Header(QNetworkRequest::KnownHeaders header, const QVariant& value)
			: Predicate()
			, m_header(header)
			, m_value(value)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QVariant headerValue = request.qRequest.header(m_header);
			return headerValue == m_value;
		}
		//! \endcond

		QNetworkRequest::KnownHeaders m_header;
		QVariant m_value;
	};

	/*! Matches if a header value matches a regular expression.
	 * \note
	 * \parblock
	 * - The \p header's value is converted to a string using QVariant::toString() to match it against the regular
	 *   expression.
	 * - This predicate does not distinguish between the case that the header has not been set and the case that the
	 *   header has been set to an empty value. So both cases match if the \p regEx matches empty strings.
	 * \endparblock
	 * \sa QNetworkRequest::header()
	 */
	class HeaderMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a header value using a regular expression.
		 * \param header The header whose value needs to match.
		 * \param regEx The regular expression matched against the \p header's value.
		 */
		explicit HeaderMatching(QNetworkRequest::KnownHeaders header, const QRegularExpression& regEx)
			: Predicate()
			, m_header(header)
			, m_regEx(regEx)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QVariant headerValue = request.qRequest.header(m_header);
			return m_regEx.match(headerValue.toString()).hasMatch();
		}
		//! \endcond

		QNetworkRequest::KnownHeaders m_header;
		QRegularExpression m_regEx;
	};

	/*! Matches if the request contains given raw headers.
	 * Note that the request can contain more headers. This predicate just checks that the given headers exist with the
	 * given values.
	 * \sa QNetworkRequest::rawHeader()
	 */
	class RawHeaders : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a set of raw headers.
		 * \param rawHeaders QHash of raw headers that need to be present in the request with defined values.
		 * The keys of the hash are the names of the expected headers and
		 * the values of the hash are the corresponding expected values of the headers.
		 */
		explicit RawHeaders( const RawHeaderHash& rawHeaders ) : Predicate(), m_rawHeaders( rawHeaders ) {}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			RawHeaderHash::const_iterator iter = m_rawHeaders.cbegin();
			const RawHeaderHash::const_iterator end = m_rawHeaders.cend();
			for ( ; iter != end; ++iter )
			{
				if (request.qRequest.rawHeader(iter.key()) != iter.value())
					return false;
			}
			return true;
		}
		//! \endcond

		RawHeaderHash m_rawHeaders;
	};

	/*! Matches if the request contains a given raw header.
	 * Note that the request can contain more headers. This predicate just checks that the given header exists with the
	 * given value.
	 * \sa QNetworkRequest::rawHeader()
	 */
	class RawHeader : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a raw request header.
		 * \param header The raw header that needs to be present in the request.
		 * \param value The value that the \p header needs to have.
		 */
		explicit RawHeader(const QByteArray& header, const QByteArray& value)
			: Predicate()
			, m_header(header)
			, m_value(value)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			return request.qRequest.rawHeader(m_header) == m_value;
		}
		//! \endcond

		QByteArray m_header;
		QByteArray m_value;
	};

	/*! Matches if a raw header value matches a regular expression.
	 * \note
	 * \parblock
	 * - The \p header's value is converted to a string using QString::fromUtf8() to match it against the \p regEx.
	 * - This predicate does not distinguish between the case that the header has not been set and the case that the
	 *   header has been set to an empty value. So both cases match if the \p regEx matches empty strings.
	 * \endparblock
	 * \sa QNetworkRequest::rawHeader()
	 */
	class RawHeaderMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching the value of a raw header using a regular expression.
		 * \param header The raw header whose value needs to match.
		 * \param regEx The regular expression matched against the \p header's value.
		 */
		explicit RawHeaderMatching(const QByteArray& header, const QRegularExpression& regEx)
			: Predicate()
			, m_header(header)
			, m_regEx(regEx)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QString headerValue = QString::fromUtf8(request.qRequest.rawHeader(m_header));
			return m_regEx.match(headerValue).hasMatch();
		}
		//! \endcond

		QByteArray m_header;
		QRegularExpression m_regEx;
	};


	/*! Matches if *all* request headers match one of the given regular expression pairs.
	 *
	 * This predicates checks all defined request headers against the given regular expression pairs in the order
	 * they are given. If the first regular expression of a pair matches the name of the header, then the
	 * second regular expression must match the value of the header. If the value does not match or if the header
	 * name does not match any of the first regular expressions of the pairs, then the predicate does not match.
	 * If all header names match one of the first regular expressions and the header values match the
	 * corresponding second regular expression, then this predicate matches.
	 *
	 * This predicate can be used to ensure that there are no unexpected headers.
	 *
	 * \note \parblock
	 * - This predicate also checks the headers defined using QNetworkRequest::setHeader().
	 * - Be aware that the Manager might add QNetworkCookies to the [QNetworkRequest::CookieHeader] in case
	 * [QNetworkRequest::CookieLoadControlAttribute] is set to [QNetworkRequest::Automatic].
	 * \endparblock
	 *
	 * ## Example ##
	 * \code
	 * RegExPairVector headerTemplates;
	 * headerTemplates.append( qMakePair( QRegularExpression( "^Accept.*" ), QRegularExpression( ".*" ) ) );
	 * headerTemplates.append( qMakePair( QRegularExpression( "^Host" ), QRegularExpression( ".*" ) ) );
	 * headerTemplates.append( qMakePair( QRegularExpression( "^User-Agent$" ), QRegularExpression( ".*" ) ) );
	 *
	 * mockNam.whenGet( QUrl( "http://example.com" ) )
	 *        .has( RawHeaderTemplates( headerTemplates ) )
	 *        .reply().withStatus( HttpStatus::OK );
	 * mockNam.whenGet( QUrl( "http://example.com" ) )
	 *        .reply().withError( QNetworkReply::UnknownContentError, "Unexpected header" );
	 * \endcode
	 *
	 * [QNetworkRequest::CookieHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 * [QNetworkRequest::CookieLoadControlAttribute]: http://doc.qt.io/qt-5/qnetworkrequest.html#Attribute-enum
	 * [QNetworkRequest::Automatic]: http://doc.qt.io/qt-5/qnetworkrequest.html#LoadControl-enum
	 */
	class RawHeaderTemplates : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching all headers against regular expression pairs.
		 *
		 * \param templates QVector of QRegularExpression pairs. The first regular expressions are matched against the
		 * header names and the second regular expressions are matched against the header values.
		 */
		explicit RawHeaderTemplates( const RegExPairVector& templates )
			: Predicate()
			, m_templates( templates )
		{
		}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QList<QByteArray> headerList = request.qRequest.rawHeaderList();
			QList<QByteArray>::const_iterator headerIter = headerList.cbegin();
			const QList<QByteArray>::const_iterator headerEnd = headerList.cend();
			for ( ; headerIter != headerEnd; ++headerIter )
			{
				bool matched = false;

				RegExPairVector::const_iterator templateIter = m_templates.cbegin();
				const RegExPairVector::const_iterator templateEnd =  m_templates.cend();
				for ( ; templateIter != templateEnd; ++templateIter )
				{
					if ( templateIter->first.match( QString::fromUtf8( *headerIter ) ).hasMatch() )
					{
						const QByteArray headerValue = request.qRequest.rawHeader( *headerIter );

						matched = templateIter->second.match( QString::fromUtf8( headerValue ) ).hasMatch();
						break;
					}
				}

				if (!matched)
					return false;
			}

			return true;
		}
		//! \endcond

		RegExPairVector m_templates;
	};

	/*! Match if the request has a given attribute.
	 * Note that the request can have more attributes. This predicate just checks that the given attribute exists with
	 * the given value.
	 * \note
	 * \parblock
	 * - This predicate cannot match the default values of the attributes since QNetworkRequest::attribute()
	 * does not return the default values. As a workaround, use the \p matchInvalid flag: when you want to match the
	 * default value, set \p value to the default value and set \p matchInvalid to \c true. Then the predicate will
	 * match either when the attribute has been set to the default value explicitly or when the attribute has not been
	 * set at all and therefore falls back to the default value.
	 * - Since the attributes are an internal feature of %Qt and are never sent to a server, using this predicate means
	 * mocking the behavior of the QNetworkAccessManager instead of the server.
	 * \endparblock
	 * \sa QNetworkRequest::attribute()
	 */
	class Attribute : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a request attribute.
		 * \param attribute The request attribute whose values is matched by this predicate.
		 * \param value The value that the \p attribute needs to have.
		 * \param matchInvalid If \c true, this predicate will match if the attribute has not been specified
		 * on the request. So the predicate matches if either the attribute has been set to the given \p value
		 * or not set at all. If \c false, this predicate will only match if the attribute has been set
		 * to the specified \p value explicitly.
		 */
		explicit Attribute( QNetworkRequest::Attribute attribute, const QVariant& value, bool matchInvalid = false )
			: Predicate()
			, m_attribute( attribute )
			, m_value( value)
			, m_matchInvalid( matchInvalid )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QVariant attribute = request.qRequest.attribute( m_attribute );
			return ( m_matchInvalid && !attribute.isValid() ) || attribute == m_value;
		}
		//! \endcond

		QNetworkRequest::Attribute m_attribute;
		QVariant m_value;
		bool m_matchInvalid;
	};

	/*! Matches if a attribute value matches a regular expression.
	 * \note
	 * \parblock
	 * - The \p attributes's value is converted to a string using QVariant::toString() to match it against the regular
	 *   expression.
	 * - This predicate does not distinguish between the case that the attribute has not been set and the case that the
	 *   attribute has been set to an empty value. So both cases match if the \p regEx matches empty strings.
	 * - Since the attributes are an internal feature of %Qt and are never sent to a server, using this predicate means
	 *   mocking the behavior of the QNetworkAccessManager instead of the server.
	 * \endparblock
	 * \sa QNetworkRequest::attribute()
	 */
	class AttributeMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching an attribute value using a regular expression.
		 * \param attribute The attribute whose value needs to match.
		 * \param regEx The regular expression matched against the \p attribute's value.
		 */
		explicit AttributeMatching( QNetworkRequest::Attribute attribute, const QRegularExpression& regEx )
			: Predicate()
			, m_attribute( attribute )
			, m_regEx( regEx )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QVariant attributeValue = request.qRequest.attribute( m_attribute );
			return m_regEx.match( attributeValue.toString() ).hasMatch();
		}
		//! \endcond

		QNetworkRequest::Attribute m_attribute;
		QRegularExpression m_regEx;
	};

	/*! Matches if the request contains a specified Authorization header.
	 *
	 * In case an unsupported authentication method is required, you might use RawHeaderMatching to "manually" match
	 * authorized requests.
	 *
	 * For example to check for a bearer authorization:
	 * \code
	 * using namespace MockNetworkAccess;
	 *
	 * Rule authorizedRequestsRule;
	 * authorizedRequestsRule.has( Predicates::RawHeaderMatching( HttpUtils::authorizationHeader(), QRegularExpression( "Bearer .*" ) ) );
	 * \endcode
	 * \sa RawHeaderMatching
	 */
	class Authorization : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching an authorization using the HTTP Basic authentication scheme with given username
		 * and password.
		 * \param username The username that must be given.
		 * \param password The password that must be given.
		 */
		explicit Authorization(const QString& username, const QString& password) : Predicate()
		{
			QAuthenticator authenticator;
			authenticator.setUser(username);
			authenticator.setPassword(password);
			m_authenticators.append(authenticator);
			m_authChallenge.reset( new HttpUtils::Authentication::Basic( QStringLiteral( "dummy" ) ) );
		}

		/*! Creates a predicate matching an authorization using the HTTP Basic authentication scheme with
		 * a selection of username and password combinations.
		 * \param credentials QHash of  username and password combinations. The authorization in the request must match
		 * one of these \p credentials.
		 */
		explicit Authorization( const QHash<QString, QString>& credentials ) : Predicate()
		{
			QHash<QString, QString>::const_iterator iter = credentials.cbegin();
			const QHash<QString, QString>::const_iterator end = credentials.cend();
			for ( ; iter != end; ++iter )
			{
				QAuthenticator authenticator;
				authenticator.setUser( iter.key() );
				authenticator.setPassword( iter.value() );
				m_authenticators.append( authenticator );
			}
			m_authChallenge.reset( new HttpUtils::Authentication::Basic( QStringLiteral( "dummy" ) ) );
		}

		/*! Creates a predicate matching an  authorization which matches a given authentication challenge with
		 * credentials defined by a given QAuthenticator.
		 * \param authChallenge The authentication challenge which the authorization in the request must match.
		 * \param authenticators Allowed username and password combinations. The authorization in the request must
		 * match one of these combinations.
		 */
		explicit Authorization( const HttpUtils::Authentication::Challenge::Ptr& authChallenge,
		                        const QVector<QAuthenticator>& authenticators )
			: Predicate()
			, m_authChallenge( authChallenge )
			, m_authenticators( authenticators )
		{
		}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			QVector<QAuthenticator>::const_iterator iter = m_authenticators.cbegin();
			const QVector<QAuthenticator>::const_iterator end = m_authenticators.cend();
			for ( ; iter != end; ++iter )
			{
				if ( m_authChallenge->verifyAuthorization( request.qRequest, *iter ) )
					return true;
			}

			return false;
		}
		//! \endcond


		HttpUtils::Authentication::Challenge::Ptr m_authChallenge;
		QVector<QAuthenticator> m_authenticators;
	};

	/*! Matches if a request contains a cookie with a given value.
	 * Note that the request can contain more cookies. This predicate just checks that the given cookie exists with the
	 * given value.
	 *
	 * \note
	 * \parblock
	 * - If there is no cookie with the given name, this predicate does not match.
	 * - In case there are multiple cookies with the given name, the first one is used and the other ones are ignored.
	 * \endparblock
	 *
	 * \sa [QNetworkRequest::CookieHeader]
	 * [QNetworkRequest::CookieHeader]: http://doc.qt.io/qt-5/qnetworkrequest.html#KnownHeaders-enum
	 */
	class Cookie : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a cookie value.
		 * \param cookie The cookie which should exist. Only the QNetworkCookie::name() and QNetworkCookie::value()
		 * are used to match. Other properties of the cookie (like QNetworkCookie::domain() or
		 * QNetworkCookie::expiryDate()) are ignored.
		 */
		explicit Cookie(const QNetworkCookie& cookie)
			: Predicate()
			, m_cookie(cookie)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			const QList<QNetworkCookie> requestCookies = request.qRequest.header( QNetworkRequest::CookieHeader )
				.value<QList<QNetworkCookie> >();
			QList<QNetworkCookie>::const_iterator iter = requestCookies.cbegin();
			const QList<QNetworkCookie>::const_iterator end = requestCookies.cend();

			for ( ; iter != end; ++iter )
			{
				const QNetworkCookie requestCookie = *iter;
				/* We use the first matching cookie and ignore possible other cookies with the same name.
				 * RFC 6265 does not define a "correct" way to handle this but this seems to be the common practice.
				 * See https://stackoverflow.com/a/24214538/490560
				 */
				if ( requestCookie.name() == m_cookie.name() )
					return ( requestCookie.value() == m_cookie.value() );
			}

			return false;
		}
		//! \endcond

		QNetworkCookie m_cookie;
	};

	/*! Matches if a request contains a cookie with a value matching a regular expression.
	 * \note
	 * \parblock
	 * - The cookies's value is converted to a string using QString::fromUtf8() to match it against the \p regEx.
	 * - If there is no cookie with the given name, this predicate does not match, no matter what \p regEx is.
	 * - If the cookie's value is empty, it is matched against the \p regEx.
	 * - In case there are multiple cookies with the given name, the first one is used and the other ones are ignored.
	 * \endparblock
	 * \sa QNetworkRequest::rawHeader()
	 */
	class CookieMatching : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching the value of a cookie using a regular expression.
		 * \param cookieName The name of the cookie whose value needs to match.
		 * \param regEx The regular expression matched against the \p header's value.
		 */
		explicit CookieMatching(const QByteArray& cookieName, const QRegularExpression& regEx)
			: Predicate()
			, m_cookieName(cookieName)
			, m_regEx(regEx)
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match(const Request& request) Q_DECL_OVERRIDE
		{
			const QList<QNetworkCookie> cookies = request.qRequest.header(QNetworkRequest::CookieHeader)
			                                      .value<QList<QNetworkCookie> >();
			QList<QNetworkCookie>::const_iterator iter = cookies.cbegin();
			const QList<QNetworkCookie>::const_iterator end = cookies.cend();
			for ( ; iter != end; ++iter)
			{
				const QByteArray cookieName = iter->name();
				/* We use the first matching cookie and ignore possible other cookies with the same name.
				 * RFC 6265 does not define a "correct" way to handle this but this seems to be the common practice.
				 * See https://stackoverflow.com/a/24214538/490560
				 */
				if (m_cookieName == cookieName)
				{
					const QString cookieValue = QString::fromUtf8(iter->value());
					return m_regEx.match(cookieValue).hasMatch();
				}
			}
			return false;
		}
		//! \endcond

		QByteArray m_cookieName;
		QRegularExpression m_regEx;
	};


	/*! Matches if a request contains a JSON body equal to a given JSON document.
	 *
	 * If the request body is not a valid JSON document, then this predicate does not
	 * match even if the given JSON document is invalid as well.
	 *
	 * \note This predicate does an exact matching so it is stricter than the
	 * other body predicates.
	 *
	 * \since 0.5.0
	 * \sa JsonBodyContaining
	 */
	class JsonBody : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching a JSON body.
		 * \param body The body to be compared to the request body.
		 */
		explicit JsonBody( const QJsonDocument& body )
			: Predicate()
			, m_body( body )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			QJsonParseError error;
			const QJsonDocument parsedDoc = QJsonDocument::fromJson( request.body, &error );
			if( error.error != QJsonParseError::NoError )
				return false;

			return parsedDoc == m_body;
		}
		//! \endcond

		QJsonDocument m_body;
	};

	/*! Matches if a request contains a JSON body which contains the properties or entries of a given JSON document.
	 *
	 * This predicate does a recursive comparison of JSON object properties and array entries.
	 * So the predicate matches if the body of a request is a JSON document which contains at least the properties
	 * or entries of the JSON document given to the constructor.
	 *
	 * For example: Given the following JSON document as the request body:
	 * \code{.json}
	 * {
	 *     "prop1": "value 1",
	 *     "prop2": true,
	 *     "nested": {
	 *         "sub prop1": "value 2",
	 *         "sub prop2": 17,
	 *         "array prop": [
	 *              "value 3",
	 *              "value 4",
	 *              "value 5"
	 *         ]
	 *     }
	 * }
	 * \endcode
	 *
	 * Then this predicate would match when constructed with the following JSON documents:
	 * \code{.json}
	 * {
	 *     "prop1": "value 1",
	 * }
	 * \endcode
	 * \code{.json}
	 * {
	 *     "nested": {
	 *         "sub prop2": 17
	 *     }
	 * }
	 * \endcode
	 * \code{.json}
	 * {
	 *     "nested": {
	 *         "array prop": [
	 *              "value 4"
	 *         ]
	 *     }
	 * }
	 * \endcode
	 *
	 * However, it would fail when given the following JSON documents:
	 * \code{.json}
	 * [
	 *     "prop1"
	 * ]
	 * \endcode
	 * \code{.json}
	 * {
	 *     "prop2": false,
	 * }
	 * \endcode
	 * \code{.json}
	 * {
	 *     "nested": {
	 *         "array prop": [
	 *              "another value"
	 *         ]
	 *     }
	 * }
	 * \endcode
	 *
	 * \since 0.5.0
	 * \sa JsonBody
	 */
	class JsonBodyContaining : public Rule::Predicate
	{
	public:
		/*! Creates a predicate matching parts of a JSON body.
		 *
		 * \param bodyPart The properties or entries to be expected in the request body.
		 * \param ensureArrayOrder If \c true, array entries must appear in the same (relative) order
		 * in the request body as in \p bodyPart. If \c false, the order of the array entries does not matter,
		 * only the existence of the entries is verified. Note that even if this is \c true, there can still
		 * be other entries in the arrays of the request body.
		 */
		explicit JsonBodyContaining( const QJsonDocument& bodyPart, bool ensureArrayOrder = false )
			: Predicate()
			, m_bodyPart( bodyPart )
			, m_ensureArrayOrder( ensureArrayOrder )
		{}

	private:
		//! \cond PRIVATE_IMPLEMENTATION
		virtual bool match( const Request& request ) Q_DECL_OVERRIDE
		{
			QJsonParseError error;
			const QJsonDocument parsedDoc = QJsonDocument::fromJson( request.body, &error );
			if( error.error != QJsonParseError::NoError )
				return false;

			if( m_bodyPart.isArray() )
			{
				if( !parsedDoc.isArray() )
					return false;
				return matchArrays( parsedDoc.array(), m_bodyPart.array() );
			}
			if( m_bodyPart.isObject() )
			{
				if( !parsedDoc.isObject() )
					return false;
				return matchObjects( parsedDoc.object(), m_bodyPart.object() );
			}

			// LCOV_EXCL_START
			Q_UNREACHABLE();
			return false;
			// LCOV_EXCL_STOP
		}
		//! \endcond

		bool matchValues( const QJsonValue& value, const QJsonValue& expectedValue )
		{
			if( isSimpleValue( value ) )
				return value == expectedValue;

			if( value.isArray() )
			{
				if ( !expectedValue.isArray() )
					return false;

				return matchArrays( value.toArray(), expectedValue.toArray() ); // RECURSION !!!
			}

			if( value.isObject() )
			{
				if( !expectedValue.isObject() )
					return false;

				return matchObjects( value.toObject(), expectedValue.toObject() ); // RECURSION !!!
			}

			// LCOV_EXCL_START
			Q_UNREACHABLE();
			return false;
			// LCOV_EXCL_STOP
		}

		static bool isSimpleValue( const QJsonValue& value )
		{
			return value.isString() || value.isBool() || value.isDouble() || isNullish( value );
		}

		static bool isNullish( const QJsonValue& value )
		{
			return value.isNull() || value.isUndefined();
		}

		bool matchArrays( const QJsonArray& array, const QJsonArray& expectedEntries )
		{
			if( m_ensureArrayOrder )
				return matchArraysEnsureOrder( array, expectedEntries ); // RECURSION !!!
			return matchArraysIgnoreOrder( array, expectedEntries ); // RECURSION !!!
		}

		bool matchArraysIgnoreOrder( const QJsonArray& array, QJsonArray expectedEntries )
		{
			auto iter = array.constBegin();
			const auto end = array.constEnd();

			for( ; iter != end; ++iter )
			{
				auto expectedIter = expectedEntries.begin();
				const auto expectedEnd = expectedEntries.end();
				while( expectedIter != expectedEnd )
				{
					if( matchValues( *iter, *expectedIter ) ) // RECURSION !!!
					{
						expectedIter = expectedEntries.erase( expectedIter );
						break;
					}

					++expectedIter;
				}
				if( expectedEntries.isEmpty() )
					return true;
			}
			return false;
		}

		bool matchArraysEnsureOrder( const QJsonArray& array, QJsonArray expectedEntries )
		{
			auto iter = array.constBegin();
			const auto end = array.constEnd();
			auto expectedIter = expectedEntries.begin();

			for( ; iter != end; ++iter )
			{
				if( matchValues( *iter, *expectedIter ) ) // RECURSION !!!
				{
					expectedIter = expectedEntries.erase( expectedIter );
					if( expectedEntries.isEmpty() )
						return true;
				}
			}
			return false;
		}

		bool matchObjects( const QJsonObject& object, const QJsonObject& expectedProps )
		{
			auto iter = expectedProps.constBegin();
			const auto end = expectedProps.constEnd();

			for( ; iter != end; ++iter )
			{
				if( !object.contains( iter.key() ) || !matchValues( object.value( iter.key() ), iter.value() ) ) // RECURSION !!!
					return false;
			}
			return true;
		}

		QJsonDocument m_bodyPart;
		bool m_ensureArrayOrder;
	};


} // namespace Predicates

/*! Defines the possible behaviors of the Manager when a request does not match any Rule.
 *
 * By default, the Manager returns a predefined reply for unmatched requests. The reply has set
 * QNetworkReply::ContentNotFoundError and an error message indicating that the request did not
 * match any Rule.
 * The default reply can be modified via Manager::unmatchedRequestBuilder().
 */
enum UnmatchedRequestBehavior
{
	PassThrough,    /*!< Unmatched requests are passed through to the next network access manager.
	                 * \sa Manager::setPassThroughNam()
	                 * \sa \ref page_passThrough
	                 */
	PredefinedReply /*!< The manager will return a predefined reply for unmatched requests.
	                 * \since 0.8.0 This is the default behavior.
	                 * \sa Manager::setUnmatchedRequestBuilder()
	                 */
};

} // namespace MockNetworkAccess

Q_DECLARE_METATYPE( MockNetworkAccess::VersionNumber )
Q_DECLARE_METATYPE( MockNetworkAccess::Request )
Q_DECLARE_METATYPE( MockNetworkAccess::Rule::Ptr )

namespace MockNetworkAccess
{

/*! Helper class which emits signals for the Manager.
 *
 * Since template classes cannot use the `Q_OBJECT` macro, they cannot define signals or slots.
 * For this reason, this helper class is needed to allow emitting signals from the Manager.
 *
 * To get the signal emitter, call Manager::signalEmitter().
 *
 * \sa Manager::signalEmitter()
 */
class SignalEmitter : public QObject
{
	Q_OBJECT

	template<class Base>
	friend class Manager;

public:
	/*! Default destructor
	 */
	virtual ~SignalEmitter() {}

private:
	/*! Creates a SignalEmitter object.
	 *
	 * \note This registers the types Request and Rule::Ptr in the %Qt meta type system
	 * using qRegisterMetaType().
	 *
	 * \param parent Parent QObject.
	 */
	explicit SignalEmitter(QObject* parent = Q_NULLPTR) : QObject(parent)
	{
		registerMetaTypes();
	}

Q_SIGNALS:

	/*! Emitted when the Manager receives a request through its public interface (QNetworkAccessManager::get() etc.).
	 * \param request The request.
	 */
	void receivedRequest(const MockNetworkAccess::Request& request);

	/*! Emitted when the Manager handles a request.
	 *
	 * This signal is emitted for requests received through the public interface (see receivedRequest()) as well as
	 * requests created internally by the Manager for example when automatically following redirects or when handling
	 * authentication.
	 *
	 * \param request The request.
	 */
	void handledRequest(const MockNetworkAccess::Request& request);

	/*! Emitted when a request matches a Rule.
	 * \param request The request.
	 * \param rule The matched Rule.
	 */
	void matchedRequest(const MockNetworkAccess::Request& request, MockNetworkAccess::Rule::Ptr rule);

	/*! Emitted when the Manager received a request which did not match any of its Rules.
	 * \param request The request.
	 */
	void unmatchedRequest(const MockNetworkAccess::Request& request);

	/*! Emitted when the Manager passed a request through to the next network access manager.
	 * \param request The request.
	 * \sa Manager::setPassThroughNam()
	 */
	void passedThrough(const MockNetworkAccess::Request& request);

private:
	static void registerMetaTypes()
	{
		static QAtomicInt registered;
		if ( registered.testAndSetAcquire( 0, 1 ) )
		{
			::qRegisterMetaType<Request>();
			::qRegisterMetaType<Rule::Ptr>();
		}
	}
};

/*! \internal Implementation details.
 */
namespace detail
{

/*! \internal
 * Updates the state of a QNetworkAccessManager according to reply headers.
 * This includes updating cookies and HSTS entries.
 */
class ReplyHeaderHandler : public QObject
{
	Q_OBJECT

public:
	ReplyHeaderHandler( QNetworkAccessManager* manager, QObject* parent = Q_NULLPTR )
		: QObject( parent ), m_manager( manager )
	{}

	virtual ~ReplyHeaderHandler() {}

public Q_SLOTS:
	void handleReplyHeaders( QNetworkReply* sender = Q_NULLPTR )
	{
		QNetworkReply* reply = getReply( sender );

		handleKnownHeaders( reply );
		handleRawHeaders( reply );
	}

private:

	QNetworkReply* getReply( QNetworkReply* sender )
	{
		if ( sender )
			return sender;

		QNetworkReply* reply = ::qobject_cast<QNetworkReply*>( this->sender() );
		Q_ASSERT( reply );
		return reply;
	}

	void handleKnownHeaders( QNetworkReply* reply )
	{
		handleSetCookieHeader( reply );
	}

	void handleSetCookieHeader( QNetworkReply* reply )
	{
		QNetworkRequest request = reply->request();
		const bool saveCookies = requestSavesCookies( request );

		QNetworkCookieJar* cookieJar = m_manager->cookieJar();
		if ( saveCookies && cookieJar )
		{
			const QList<QNetworkCookie> cookies = reply->header( QNetworkRequest::SetCookieHeader )
			                                      .value< QList< QNetworkCookie > >();
			if ( !cookies.isEmpty() )
				cookieJar->setCookiesFromUrl( cookies, reply->url() );
		}
	}

	static bool requestSavesCookies( const QNetworkRequest& request )
	{
		const int defaultValue = static_cast<int>( QNetworkRequest::Automatic );
		const int saveCookiesInt = request.attribute( QNetworkRequest::CookieSaveControlAttribute, defaultValue ).toInt();
		return static_cast<QNetworkRequest::LoadControl>( saveCookiesInt ) == QNetworkRequest::Automatic;
	}

	void handleRawHeaders( QNetworkReply* reply )
	{
		const QList<QNetworkReply::RawHeaderPair>& rawHeaderPairs = reply->rawHeaderPairs();
		QList<QNetworkReply::RawHeaderPair>::const_iterator headerIter = rawHeaderPairs.cbegin();
		const QList<QNetworkReply::RawHeaderPair>::const_iterator headerEnd = rawHeaderPairs.cend();
		for ( ; headerIter != headerEnd; ++headerIter )
		{
			// header field-name is ASCII according to RFC 7230 3.2
			const QByteArray headerName = headerIter->first.toLower();

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
			const QByteArray stsHeader( "strict-transport-security" );
			if ( headerName == stsHeader )
			{
				handleStsHeader( headerIter->second, reply );
			}
#endif // Qt >= 5.9.0
		}
	}

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
	void handleStsHeader( const QByteArray& headerValue, const QNetworkReply* reply )
	{
		const QStringList stsPolicies = HttpUtils::splitCommaSeparatedList( QString::fromLatin1( headerValue ) );
		QStringList::const_iterator stsPolicyIter = stsPolicies.constBegin();
		const QStringList::const_iterator stsPolicyEnd = stsPolicies.constEnd();
		for ( ; stsPolicyIter != stsPolicyEnd; ++stsPolicyIter )
		{
			/* If the header has an invalid syntax, we ignore it and continue
			 * until we find a valid STS policy.
			 */
			if ( processStsPolicy( stsPolicyIter->toLatin1(), reply->url() ) )
				break; // following STS policies are ignored
			continue;
		}
	}

	bool processStsPolicy( const QByteArray& header, const QUrl& host )
	{
		const QString headerData = QString::fromLatin1( header );
		const QStringList directives = headerData.split( QChar::fromLatin1( ';' ) );

		QHstsPolicy policy;
		policy.setHost( host.host() );

		QSet< QString > foundDirectives;

		QStringList::const_iterator directiveIter = directives.cbegin();
		const QStringList::const_iterator directiveEnd = directives.cend();
		for ( ; directiveIter != directiveEnd; ++directiveIter )
		{
			const QString cleanDirective = HttpUtils::whiteSpaceCleaned( *directiveIter );
			const QPair< QString, QString > directiveSplit = splitStsDirective( cleanDirective );
			const QString directiveName = directiveSplit.first;
			const QString directiveValue = directiveSplit.second;

			if ( foundDirectives.contains( directiveName ) )
				return false; // Invalid header: duplicate directive
			foundDirectives.insert( directiveName );

			if ( !processStsDirective( policy, directiveName, directiveValue ) )
				return false;
		}

		if ( !foundDirectives.contains( maxAgeDirectiveName() ) )
			return false; // Invalid header: missing required max-age directive

		m_manager->addStrictTransportSecurityHosts( QVector<QHstsPolicy>() << policy );
		return true;
	}

	static QLatin1String maxAgeDirectiveName()
	{
		return QLatin1String( "max-age" );
	}

	static QPair< QString, QString > splitStsDirective( const QString& directive )
	{
		const QRegularExpression basicDirectiveRegEx( QStringLiteral( "^([^=]*)=?(.*)$" ) );

		QRegularExpressionMatch match;

		match = basicDirectiveRegEx.match( directive );
		// This should be impossible since basicDirectiveRegEx matches everything
		Q_ASSERT_X( match.hasMatch(), Q_FUNC_INFO, "Could not parse directive." );

		const QString directiveName = HttpUtils::whiteSpaceCleaned( match.captured( 1 ) ).toLower();
		const QString rawDirectiveValue = HttpUtils::whiteSpaceCleaned( match.captured( 2 ) );
		const QString directiveValue = HttpUtils::isValidToken( rawDirectiveValue )
		                               ? rawDirectiveValue
		                               : HttpUtils::unquoteString( rawDirectiveValue );

		return ::qMakePair( directiveName, directiveValue );
	}

	static bool processStsDirective( QHstsPolicy& policy, const QString& directiveName, const QString& directiveValue )
	{
		if ( directiveName == maxAgeDirectiveName() )
		{
			return processStsMaxAgeDirective( policy, directiveValue );
		}

		if ( directiveName == QLatin1String( "includesubdomains" ) )
		{
			policy.setIncludesSubDomains( true );
			return true;
		}

		// else we check if the directive is legal at all
		if ( !HttpUtils::isValidToken( directiveName ) )
			return false; // Invalid header: illegal directive name

		if ( !HttpUtils::isValidToken( directiveValue ) && !HttpUtils::isValidQuotedString( directiveValue ) )
			return false; // Invalid header: illegal directive value

		// Directive seems legal but simply unknown. So we ignore it.
		return true;
	}

	static bool processStsMaxAgeDirective( QHstsPolicy& policy, const QString& directiveValue )
	{
		const QRegularExpression maxAgeValueRegEx( QStringLiteral( "\\d+" ) );

		const QRegularExpressionMatch match = maxAgeValueRegEx.match( directiveValue );
		if ( !match.hasMatch() )
			return false; // Invalid header: incorrect max-age value
		const qint64 maxAge = match.captured( 0 ).toLongLong();
		policy.setExpiry( QDateTime::currentDateTimeUtc().addSecs( maxAge ) );
		return true;
	}
#endif // Qt >= 5.9.0

	QPointer< QNetworkAccessManager > m_manager;
};


} // namespace detail


/*! Determines the behavior of the %Qt version in use.
 * This is also the default behavior of Manager objects if not overridden using Manager::setBehaviorFlags().
 * \return The BehaviorFlags matching the behavior of the %Qt version used at runtime.
 * \sa [qVersion()](https://doc.qt.io/qt-5/qtglobal.html#qVersion)
 * \sa BehaviorFlag
 * \since 0.3.0
 */
inline BehaviorFlags getDefaultBehaviorFlags()
{
	#if QT_VERSION < QT_VERSION_CHECK( 5,2,0 )
		#error MockNetworkAccessManager requires Qt 5.2.0 or later
	#endif
	const char* qtVersion = ::qVersion();
	const VersionNumber qtVersionInUse = VersionNumber::fromString( QString::fromLatin1( qtVersion ) );

	const VersionNumber qt5_6_0( 5,6,0 );

	if ( qtVersionInUse >= qt5_6_0 )
		return Behavior_Qt_5_6_0;
	else
		return Behavior_Qt_5_2_0;
}


/*! Mixin class to mock network replies from QNetworkAccessManager.
 * %Manager mocks the QNetworkReplys instead of sending the requests over the network.
 * %Manager is a mixin class meaning it can be used "on top" of every class inheriting publicly from
 * QNetworkAccessManager.
 *
 * \tparam Base QNetworkAccessManager or a class publicly derived from QNetworkAccessManager.
 *
 *
 * ## Configuration ##
 * To define which and how requests are answered with mocked replies, the %Manager is configured using
 * \link Rule Rules\endlink:
 * Whenever the %Manager is handed over a request, it matches the request against its rules one after the other.\n
 * - If a rule reports a match for the request, the %Manager requests the rule to create a reply for that request.\n
 *   - If the rule creates a reply, then this reply is returned by the %Manager.\n
 *   - If the rule does not create a reply, the %Manager continues matching the request against the remaining rules.\n
 * - If no rule matches the request or no rule created a reply, the "unmatched request behavior" steps in.\n
 *   This means either:
 *   1. the request is passed through to the next network access manager (see setPassThroughNam()) and the corresponding
 *      QNetworkReply is returned.
 *   2. a predefined reply is returned (see unmatchedRequestBuilder()).
 *
 *   The latter is the default behavior. For more details see \ref UnmatchedRequestBehavior.
 *
 * To define which requests match a rule, the Rule object is configured by adding predicates.
 *
 * To define the properties of the created replies, the %Rule object exposes a MockReplyBuilder via the Rule::reply()
 * method.
 *
 * To add a rule to the %Manager, you can either:
 * - create a %Rule object, configure it and add it using addRule().
 * - use the convenience methods whenGet(), whenPost(), when() etc. and configure the returned %Rule objects.
 *
 * To retrieve or remove Rules or change their order, use the methods rules() and setRules().
 *
 *
 * ### Example ###
 *
 * \code
 * using namespace MockNetworkAccess;
 * using namespace MockNetworkAccess::Predicates;
 *
 * // Create the Manager
 * Manager< QNetworkAccessManager > mockNam;
 *
 * // Simple configuration
 * mockNam.whenGet( QRegularExpression( "https?://example.com/data/.*" ) )
 *        .reply().withBody( QJsonDocument::fromJson( "{ \"id\": 736184, \"data\": \"Hello World!\" }" );
 *
 * // More complex configuration
 * Rule::Ptr accountInfoRequest( new Rule );
 * accountInfoRequest->has( Verb( QNetworkAccessManager::GetOperation ) )
 *                   .has( UrlMatching( QRegularExpression( "https?://example.com/accountInfo/.*" ) ) );
 *
 * Rule::Ptr authorizedAccountInfoRequest( accountInfoRequest->clone() );
 * authorizedAccountInfoRequest->has( RawHeaderMatching( HttpUtils::authorizationHeader(), QRegularExpression( "Bearer: .*" ) ) )
 *                             .reply().withBody( QJsonDocument::fromJson( "{ \"name\": \"John Doe\", \"email\": \"john.doe@example.com\" }" ) );
 *
 * Rule::Ptr unauthorizedAccountInfoRequest( accountInfoRequest->clone() );
 * unauthorizedAccountInfoRequest->reply().withStatus( 401 );
 *
 * // The order is important here since the
 * // first matching rule will create the reply.
 * mockNam.add( authorizedAccountInfoRequest );
 * mockNam.add( unauthorizedAccountInfoRequest );
 *
 * // All other requests
 * mockNam.unmatchedRequestBuilder().withStatus( 404 );
 *
 * // Use the Manager
 * MyNetworkClient myNetworkClient;
 * myNetworkClient.setNetworkManager( &mockNam );
 * myNetworkClient.run();
 * \endcode
 *
 * ### Signals ###
 * Since the Manager is a template class, it cannot define signals due to limitations of %Qt's meta object compiler
 * (moc).
 *
 * To solve this, the Manager provides a SignalEmitter (see signalEmitter()) which emits the signals on behalf of the
 * Manager.
 *
 * [QNetworkRequest::UserVerifiedRedirectPolicy]: http://doc.qt.io/qt-5/qnetworkrequest.html#RedirectPolicy-enum
 * [QNetworkRequest::Attributes]: http://doc.qt.io/qt-5/qnetworkrequest.html#Attribute-enum
 *
 *
 * ## Handling of non-HTTP Protocols ##
 * The Manager also supports FTP, `data:`, `file:` and `qrc:` requests. However, for `data:`, `file:` and `qrc:` requests
 * the Manager behaves differently as for HTTP or FTP requests.
 *
 * ### `data:` Requests ###
 * `data:` requests are always forwarded to the \p Base network access manager. That's the easiest way to implement the
 * handling of such requests and since they are never sent to the network it does not make sense to allow any kind of
 * reply mocking there. This means that requests with a `data:` URL are never matched against any rule and these requests
 * are never contained in the matchedRequests(), unmatchedRequests() or passedThroughRequests(). However, they are contained
 * in the receivedRequests() and handledRequests().
 *
 * ### `file:` and `qrc:` Requests ###
 * Requests with a `file:` URL only support the \link QNetworkAccessManager::get() GET \endlink and
 * \link QNetworkAccessManager::put() PUT \endlink operations. Requests with a `qrc:` URL only support the
 * \link QNetworkAccessManager::get() GET \endlink operation. All other operations will result in a reply
 * with an QNetworkReply::ProtocolUnknownError.
 *
 * If you want to mock a successful `PUT` operation of a `file:` request, you should configure the rule to reply with
 * QNetworkReply::NoError. It is necessary to call one of the `with*()` methods of the MockReplyBuilder for the Rule
 * to be considered valid by the Manager. And setting `withError( QNetworkReply::NoError )` is the only configuration
 * that is applicable for a successful `PUT` operation for a `file:` request. For example:
 *
 * \code
 * using namespace MockNetworkAccess;
 *
 * Manager< QNetworkAccessManager > mnam;
 * mnam.whenPut( QUrl( "file:///path/to/file" ) ).reply().withError( QNetworkReply::NoError );
 * \endcode
 *
 *
 * ## Limitations ##
 * The Manager currently has a few limitations:
 * - When a request with automatic redirect following is passed through and gets redirected,
 *   the rules of the initial Manager are not applied to the redirect
 *   (see \ref page_passThrough_redirects and issue \issue{15}).
 * - When a request is redirected and then passed through to a separate QNetworkAccessManager
 *   (see setPassThroughNam()), the QNetworkReply::metaDataChanged() and
 *   QNetworkReply::redirected() signals of the mocked redirections are emitted out of order (namely after all other
 *   signals).
 * - The mocked replies do not emit the implementation specific signals of a real HTTP based QNetworkReply
 *   (that is the signals of QNetworkReplyHttpImpl).
 * - Out of the box, only HTTP Basic authentication is supported. However, this should not be a problem in most cases
 *   since the handling of authentication is normally done internally between the `MockNetworkAccess::Manager` and the
 *   `MockReply`.\n
 *   This is only a limitation if you manually create `Authorization` headers and have to rely on HTTP Digest or NTLM
 *   authentication.\n
 *   Note that it is still possible to work with any authentication method by matching the `Authorization` header
 *   manually (for example using Predicates::RawHeaderMatching) or by implementing a
 *   \link Rule::Predicate custom predicate\endlink.
 * - The QAuthenticator passed in the `QNetworkAccessManager::authenticationRequired()` signal does not provide the
 *   `realm` parameter via the `QAuthenticator::realm()` method in %Qt before 5.4.0 but only as option with the key
 *   `realm` (for example, via `authenticator->option("realm")`).
 * - Proxy authentication is not supported at the moment.
 * - [QNetworkRequest::UserVerifiedRedirectPolicy] is not supported at the moment.
 * - The error messages of the replies (QNetworkReply::errorString()) may be different from the ones of real
 *   QNetworkReply objects.
 * - QNetworkReply::setReadBufferSize() is ignored at the moment.
 *
 *
 * Some of these limitations might be removed in future versions. Feel free to create a feature (or merge) request if
 * you hit one these limitations.
 *
 * Additionally, the Manager supports only selected [QNetworkRequest::Attributes].
 * The following attributes are supported:
 * - QNetworkRequest::HttpStatusCodeAttribute
 * - QNetworkRequest::HttpReasonPhraseAttribute
 * - QNetworkRequest::RedirectionTargetAttribute
 * - QNetworkRequest::ConnectionEncryptedAttribute
 * - QNetworkRequest::CustomVerbAttribute
 * - QNetworkRequest::CookieLoadControlAttribute
 * - QNetworkRequest::CookieSaveControlAttribute
 * - QNetworkRequest::FollowRedirectsAttribute
 * - QNetworkRequest::OriginalContentLengthAttribute
 * - QNetworkRequest::RedirectPolicyAttribute
 *
 * All other attributes are ignored when specified on a QNetworkRequest and are not set when returning a MockReply.
 * However, if desired, the attributes can be matched on a request using Predicates::Attribute or
 * Predicates::AttributeMatching and can be set on a MockReply using MockReplyBuilder::withAttribute().
 *
 * \note
 * \parblock
 * At the moment, the Manager does not handle large request bodies well since it reads them into
 * memory completely to be able to provide them to all the Rule objects.
 *
 * With setInspectBody(), you can disable this if you need to use the Manager with large request
 * bodies and you do not need to match against the body.
 * \endparblock
 *
 */
template<class Base>
class Manager : public Base
{
	// cannot use Q_OBJECT with template class
public:

	/*! Creates a Manager.
	 * \param parent Parent QObject.
	 */
	explicit Manager( QObject* parent = Q_NULLPTR )
		: Base( parent )
		, m_inspectBody( true )
		, m_behaviorFlags( getDefaultBehaviorFlags() )
		, m_passThroughNam( Q_NULLPTR )
		, m_signalEmitter( Q_NULLPTR )
		, m_unmatchedRequestBehavior( PredefinedReply )
		, m_replyHeaderHandler( new detail::ReplyHeaderHandler( this ) )
	{
		setupDefaultReplyBuilder();
	}

	/*! Default destructor */
	virtual ~Manager() {}

	/*! Defines whether the message body of requests should be used to match requests.
	 * By default, the Manager reads the complete request body into memory to match it against the Rules.
	 * Setting \p inspectBody to \c false prevents that the request body is read into memory.
	 * However, the matching is then done using a null QByteArray() as request body. So Rules with body predicates will
	 * not match unless they match an empty body.
	 * \param inspectBody If \c true (the default), the request body will be read and matched against the predicates of
	 * the Rules. If \c false, the request body will not be read by the Manager but a null QByteArray() will be used
	 * instead.
	 */
	void setInspectBody(bool inspectBody) { m_inspectBody = inspectBody; }

	/*! \return The behavior flags active on this Manager.
	 */
	BehaviorFlags behaviorFlags() const { return m_behaviorFlags; }

	/*! Tunes the behavior of this Manager.
	 *
	 * \param behaviorFlags Combination of BehaviorFlags to define some details of this Manager's behavior.
	 * \sa BehaviorFlag
	 */
	void setBehaviorFlags(BehaviorFlags behaviorFlags) { m_behaviorFlags = behaviorFlags; }

	/*! Defines how the Manager handles requests that do not match any Rule.
	 *
	 * \param unmatchedRequestBehavior An UnmatchedRequestBehavior flag to define the new behavior.
	 *
	 * \sa unmatchedRequestBehavior()
	 */
	void setUnmatchedRequestBehavior( UnmatchedRequestBehavior unmatchedRequestBehavior )
	{
		m_unmatchedRequestBehavior = unmatchedRequestBehavior;
	}

	/*! \return How the Manager handles unmatched requests.
	 *
	 * \sa setUnmatchedRequestBehavior()
	 */
	UnmatchedRequestBehavior unmatchedRequestBehavior() const { return m_unmatchedRequestBehavior; }

	/*! Defines a reply builder being used to create replies for requests that do not match any Rule in the Manager.
	 *
	 * \note This builder is only used when unmatchedRequestBehavior() is PredefinedReply.
	 *
	 * \param builder The MockReplyBuilder creating the replies for unmatched requests.
	 * \sa setUnmatchedRequestBehavior()
	 */
	void setUnmatchedRequestBuilder( const MockReplyBuilder& builder ) { m_unmatchedRequestBuilder = builder; }

	/*! \return The reply builder being used to create replies for requests that do not match any Rule in the Manager.
	 *
	 * \note This builder is only used when unmatchedRequestBehavior() is PredefinedReply.
	 *
	 * \sa setUnmatchedRequestBuilder()
	 * \sa setUnmatchedRequestBehavior()
	 */
	MockReplyBuilder& unmatchedRequestBuilder() { return m_unmatchedRequestBuilder; }

	/*! Defines the QNetworkAccessManager to be used in case requests should be passes through to the network.
	 * By default, the \p Base class of this Manager is used.
	 * \param passThroughNam The network access manager to be used to pass requests through. If this is a null pointer,
	 * the \p Base class of this Manager is used.
	 * \note This could also be another MockNetworkAccess::Manager. This allows building up a hierarchy of Managers.
	 * \sa setUnmatchedRequestBehavior()
	 * \sa Rule::passThrough()
	 * \sa \ref page_passThrough
	 */
	void setPassThroughNam(QNetworkAccessManager* passThroughNam) { m_passThroughNam = passThroughNam; }

	/*! \return The network access manager to which requests are passed through or a \c Q_NULLPTR if the requests are
	 * passed through to the \p Base class of this Manager.
	 */
	QNetworkAccessManager* passThroughNam() const { return m_passThroughNam; }

	/*! \return The Rules of this Manager.
	 */
	QVector< Rule::Ptr > rules() const { return m_rules; }

	/*! Sets the Rules for this Manager.
	 * This will remove all previous Rules.
	 * \param rules the new rules for this Manager.
	 */
	void setRules(const QVector< Rule::Ptr >& rules) { m_rules = rules; }

	/*! Adds a Rule to this Manager.
	 * The rule is appended to the existing list of Rules.
	 * \param rule A QSharedPointer to the Rule to be added to this Manager.
	 */
	void addRule( const Rule::Ptr& rule ) { m_rules.append( rule ); }

	/*! Creates a clone of a Rule and adds it to this Manager.
	 * The clone of the rule is appended to the existing list of Rules.
	 * \param rule The Rule to be added to this Manager.
	 * \return A reference to the clone.
	 * \sa Rule::clone()
	 */
	Rule& addRule( const Rule& rule )
	{
		Rule::Ptr newRule( rule.clone() );
		m_rules.append( newRule );
		return *newRule;
	}

	/*! Creates and adds a Rule which matches \c GET requests with a URL matching a regular expression.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenGet( const QRegularExpression& urlRegEx )
	{
		return when( QNetworkAccessManager::GetOperation, urlRegEx );
	}

	/*! Creates and adds a Rule which matches \c GET requests with a given URL.
	 * \param url The URL matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenGet( const QUrl& url )
	{
		return when( QNetworkAccessManager::GetOperation, url );
	}

	/*! Creates and adds a Rule which matches \c POST requests with a URL matching a regular expression.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenPost( const QRegularExpression& urlRegEx )
	{
		return when( QNetworkAccessManager::PostOperation, urlRegEx );
	}

	/*! Creates and adds a Rule which matches \c POST requests with a given URL.
	 * \param url The URL matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenPost( const QUrl& url )
	{
		return when( QNetworkAccessManager::PostOperation, url );
	}

	/*! Creates and adds a Rule which matches \c PUT requests with a URL matching a regular expression.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenPut( const QRegularExpression& urlRegEx )
	{
		return when( QNetworkAccessManager::PutOperation, urlRegEx );
	}

	/*! Creates and adds a Rule which matches \c PUT requests with a given URL.
	 * \param url The URL matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenPut( const QUrl& url )
	{
		return when( QNetworkAccessManager::PutOperation, url );
	}

	/*! Creates and adds a Rule which matches \c DELETE requests with a URL matching a regular expression.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenDelete( const QRegularExpression& urlRegEx )
	{
		return when( QNetworkAccessManager::DeleteOperation, urlRegEx );
	}

	/*! Creates and adds a Rule which matches \c DELETE requests with a given URL.
	 * \param url The URL matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenDelete( const QUrl& url )
	{
		return when( QNetworkAccessManager::DeleteOperation, url );
	}

	/*! Creates and adds a Rule which matches \c HEAD requests with a URL matching a regular expression.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenHead( const QRegularExpression& urlRegEx )
	{
		return when( QNetworkAccessManager::HeadOperation, urlRegEx );
	}

	/*! Creates and adds a Rule which matches \c HEAD requests with a given URL.
	 * \param url The URL matched against the request's URL.
	 * \return A reference to the created Rule.
	 */
	Rule& whenHead( const QUrl& url )
	{
		return when( QNetworkAccessManager::HeadOperation, url );
	}

	/*! Creates and adds a Rule which matches requests with a given HTTP verb and a URL matching a regular expression.
	 * \param operation The HTTP verb which the request needs to match.
	 * \param urlRegEx The regular expression matched against the request's URL.
	 * \param customVerb The HTTP verb in case \p operation is QNetworkAccessManager::CustomOperation. Else this
	 * parameter is ignored.
	 * \return A reference to the created Rule.
	 */
	Rule& when( QNetworkAccessManager::Operation operation, const QRegularExpression& urlRegEx,
	            const QByteArray& customVerb = QByteArray() )
	{
		using namespace Predicates;
		Rule::Ptr rule( new Rule() );
		rule->has( Verb( operation, customVerb ) );
		rule->has( UrlMatching( urlRegEx ) );
		m_rules.append( rule );
		return *rule;
	}

	/*! Creates and adds a Rule which matches requests with a given HTTP verb and a given URL.
	 * \param operation The HTTP verb which the request needs to match.
	 * \param url The URL matched against the request's URL.
	 * \param customVerb The HTTP verb in case \p operation is QNetworkAccessManager::CustomOperation. Else this
	 * parameter is ignored.
	 * \return A reference to the created Rule.
	 */
	Rule& when( QNetworkAccessManager::Operation operation, const QUrl& url,
	            const QByteArray& customVerb = QByteArray() )
	{
		using namespace Predicates;
		Rule::Ptr rule( new Rule() );
		rule->has( Verb( operation, customVerb ) );
		rule->has( Url( url ) );
		m_rules.append( rule );
		return *rule;
	}

	/*! Provides access to signals of the Manager.
	 *
	 * \return A SignalEmitter object which emits signals on behalf of the Manager.
	 * The ownership of the SignalEmitter stays with the Manager. The caller must not delete it.
	 *
	 * \sa SignalEmitter
	 */
	SignalEmitter* signalEmitter() const
	{
		if (!m_signalEmitter)
			m_signalEmitter.reset( new SignalEmitter() );
		return m_signalEmitter.get();
	}

	/*! \return A vector of all requests which this Manager received through its public interface.
	 */
	QVector< Request > receivedRequests() const { return m_receivedRequests; }

	/*! Returns all requests which were handled by this Manager.
	 *
	 * This includes the requests received through the public interface (see receivedRequests()) as well as requests
	 * created internally by the Manager for example when automatically following redirects or when handling
	 * authentication.
	 *
	 * \return A vector of all requests handled by this Manager.
	 */
	QVector< Request > handledRequests() const { return m_handledRequests; }

	/*! \return A vector of all requests which matched a Rule.
	 */
	QVector< Request > matchedRequests() const { return m_matchedRequests; }

	/*! \return A vector of all requests which did not match any Rule.
	 */
	QVector< Request > unmatchedRequests() const { return m_unmatchedRequests; }

	/*! \return A vector of all requests which where passed through to the next (real) network access manager.
	 * \sa setPassThroughNam()
	 */
	QVector< Request > passedThroughRequests() const { return m_passedThroughRequests; }

protected:
	/*! Implements the creation of mocked replies.
	 *
	 * \param operation The HTTP verb of the operation.
	 * \param origRequest The QNetworkRequest object.
	 * \param body Optional request body.
	 * \return A pointer to a QNetworkReply object. The caller takes ownership of the returned reply object. The reply
	 * can either be a real QNetworkReply or a mocked reply. In case of a mocked reply, it is an instance of MockReply.
	 *
	 * \sa QNetworkAccessManager::createRequest()
	 */
	virtual QNetworkReply* createRequest( QNetworkAccessManager::Operation operation, const QNetworkRequest& origRequest,
	                                      QIODevice* body ) Q_DECL_OVERRIDE;


private:

	void setupDefaultReplyBuilder()
	{
		m_unmatchedRequestBuilder.withError( QNetworkReply::ContentNotFoundError,
		                                     QStringLiteral( "MockNetworkAccessManager: Request did not match any rule" )
		                                     );
	}


	QNetworkRequest prepareRequest(const QNetworkRequest& origRequest);
	QNetworkReply* handleRequest( const Request& request );
	QIODevice* createIODevice(const QByteArray& data) const;
	QNetworkReply* passThrough( const Request& request, QNetworkAccessManager* overridePassThroughNam = Q_NULLPTR );
	QNetworkReply* authenticateRequest(MockReply* unauthedReply, const Request& unauthedReq);
	QAuthenticator getAuthenticator(MockReply* unauthedReply, const Request& unauthedReq,
	                                const HttpUtils::Authentication::Challenge::Ptr& authChallenge);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	QNetworkReply* followRedirect(MockReply* prevReply, const Request& prevReq);
#endif // Qt >= 5.6.0
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
	bool applyRedirectPolicy(QNetworkRequest::RedirectPolicy policy, MockReply* prevReply,
	                         const QNetworkRequest& prevRequest, const QUrl& redirectTarget);
	void prepareHstsHash();
	bool elevateHstsUrl(const QUrl& url);
#endif // Qt >= 5.9.0
	QNetworkReply* createDataUrlReply( const Request& request );
	void prepareReply(MockReply* reply, const Request& request) const;
	void finishReply(QNetworkReply* reply, const Request& initialRequest) const;
	void addReceivedRequest( const Request& request );
	void addHandledRequest( const Request& request );
	void addMatchedRequest( const Request& request, const Rule::Ptr& matchedRule );
	void addUnmatchedRequest( const Request& request );
	void addPassedThroughRequest( const Request& request );

	bool m_inspectBody;
	BehaviorFlags m_behaviorFlags;
	QPointer<QNetworkAccessManager> m_passThroughNam;
	QVector<Rule::Ptr> m_rules;
	QVector<Request> m_receivedRequests;
	QVector<Request> m_handledRequests;
	QVector<Request> m_matchedRequests;
	QVector<Request> m_unmatchedRequests;
	QVector<Request> m_passedThroughRequests;
	mutable std::unique_ptr< SignalEmitter > m_signalEmitter;
	UnmatchedRequestBehavior m_unmatchedRequestBehavior;
	MockReplyBuilder m_unmatchedRequestBuilder;
	std::unique_ptr< detail::ReplyHeaderHandler > m_replyHeaderHandler;
	QHash<QString, QAuthenticator> m_authenticationCache;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
	std::unique_ptr< QHash< QString, QHstsPolicy > > m_hstsHash;
#endif // Qt >= 5.9.0
};

/*! \internal Implementation details.
 */
namespace detail {

inline const char* followedRedirectsPropertyName()
{
	return "MockNetworkAccess::FollowedRedirects";
}

} // namespace detail


//####### Implementation #######

#if defined( MOCKNETWORKACCESSMANAGER_QT_HAS_TEXTCODEC )
	inline StringDecoder::StringDecoder( QTextCodec* codec )
		: m_impl( new TextCodecImpl( codec ) )
	{
	}
#endif

#if QT_VERSION >= QT_VERSION_CHECK( 6,0,0 )
	inline StringDecoder::StringDecoder( std::unique_ptr<QStringDecoder>&& decoder )
		: m_impl{ new StringDecoderImpl( std::move( decoder ) ) }
	{
	}
#endif

namespace detail {

inline bool requestLoadsCookies( const QNetworkRequest& request )
{
	const QVariant defaultValue = QVariant::fromValue( static_cast<int>( QNetworkRequest::Automatic ) );
	const int requestValue = request.attribute( QNetworkRequest::CookieLoadControlAttribute, defaultValue ).toInt();
	return static_cast<QNetworkRequest::LoadControl>( requestValue ) == QNetworkRequest::Automatic;
}

}

template< class Matcher >
Rule& Rule::isMatching( const Matcher& matcher )
{
	m_predicates.append( Predicates::createGeneric( matcher ) );
	return *this;
}

template< class Matcher >
Rule& Rule::isNotMatching( const Matcher& matcher )
{
	Predicate::Ptr predicate = Predicates::createGeneric( matcher );
	predicate->negate();
	m_predicates.append( predicate );
	return *this;
}

template<class Base>
QNetworkReply* Manager<Base>::createRequest( QNetworkAccessManager::Operation operation,
                                             const QNetworkRequest& origRequest, QIODevice* body )
{
	QByteArray data;
	if ( m_inspectBody && body )
		data = body->readAll();
	const QNetworkRequest preparedRequest = prepareRequest( origRequest );
	const Request request( operation, preparedRequest, data );

	addReceivedRequest( request );

	QNetworkReply* reply = handleRequest( request );
	finishReply( reply, request );
	return reply;
}

template<class Base>
QNetworkRequest Manager<Base>::prepareRequest( const QNetworkRequest& origRequest )
{
	QNetworkRequest request( origRequest );

	#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
		if ( this->isStrictTransportSecurityEnabled() && elevateHstsUrl( request.url() ) )
		{
			QUrl url = request.url();
			url.setScheme( HttpUtils::httpsScheme() );
			if ( url.port() == HttpUtils::HttpDefaultPort )
				url.setPort( HttpUtils::HttpsDefaultPort );
			request.setUrl( url );
		}
	#endif // Qt >= 5.9.0

	const bool loadCookies = detail::requestLoadsCookies( request );
	if ( loadCookies )
	{
		QNetworkCookieJar* cookieJar = this->cookieJar();
		if ( cookieJar )
		{
			QUrl requestUrl = request.url();
			if ( requestUrl.path().isEmpty() )
				requestUrl.setPath( QStringLiteral( "/" ) );
			QList<QNetworkCookie> cookies = cookieJar->cookiesForUrl( requestUrl );
			if ( !cookies.isEmpty() )
				request.setHeader( QNetworkRequest::CookieHeader, QVariant::fromValue( cookies ) );
		}
	}

	return request;
}

template< class Base >
void Manager< Base >::addReceivedRequest( const Request& request )
{
	m_receivedRequests.append( request );
	if ( m_signalEmitter )
		Q_EMIT m_signalEmitter->receivedRequest( request );
}

template<class Base>
QNetworkReply* Manager<Base>::handleRequest( const Request& request )
{
	addHandledRequest( request );

	if( detail::isDataUrlRequest( request ) )
		return createDataUrlReply( request );

	std::unique_ptr< MockReply > mockedReply;
	QVector<Rule::Ptr>::iterator ruleIter = m_rules.begin();
	const QVector<Rule::Ptr>::iterator rulesEnd = m_rules.end();
	for ( ; ruleIter != rulesEnd; ++ruleIter )
	{
		Rule::Ptr rule = *ruleIter;
		if ( rule->matches( request ) )
		{
			if ( rule->passThroughBehavior() != Rule::PassThroughReturnDelegatedReply )
			{
				mockedReply.reset( rule->createReply( request ) );
				if ( !mockedReply )
					continue;
			}

			addMatchedRequest( request, rule );

			if ( rule->passThroughBehavior() != Rule::DontPassThrough )
			{
				std::unique_ptr< QNetworkReply > passThroughReply( passThrough( request, rule->passThroughManager() ) );
				switch ( rule->passThroughBehavior() )
				{
				case Rule::PassThroughReturnMockReply:
					QObject::connect( passThroughReply.get(), SIGNAL( finished() ),
					                  passThroughReply.get(), SLOT( deleteLater() ) );
					passThroughReply.release()->setParent( this );
					break;
				case Rule::PassThroughReturnDelegatedReply:
					return passThroughReply.release();
				// LCOV_EXCL_START
				default:
					Q_ASSERT_X( false, Q_FUNC_INFO, "MockNetworkAccessManager: Internal error: "
					                                "Unknown Rule::PassThroughBehavior" );
					break;
				// LCOV_EXCL_STOP
				}
			}

			prepareReply( mockedReply.get(), request );

			if ( mockedReply->requiresAuthentication() )
			{
				// POTENTIAL RECURSION
				std::unique_ptr< QNetworkReply > authedReply( authenticateRequest( mockedReply.get(), request ) );
				if ( authedReply ) // Did we start a new, authenticated request?
					return authedReply.release();
			}

		#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 6, 0 ) )
			if ( mockedReply->isRedirectToBeFollowed() )
			{
				// POTENTIAL RECURSION
				std::unique_ptr< QNetworkReply > redirectedReply( followRedirect( mockedReply.get(), request ) );
				if ( redirectedReply ) // Did we actually redirect?
					return redirectedReply.release();
			}
		#endif // Qt >= 5.6.0

			break;
		}
	}

	if ( mockedReply )
	{
		return mockedReply.release();
	}
	else
	{
		addUnmatchedRequest( request );
		switch ( m_unmatchedRequestBehavior )
		{
		case PredefinedReply:
			return m_unmatchedRequestBuilder.createReply();
		case PassThrough:
			return passThrough( request );
		// LCOV_EXCL_START
		default:
			Q_ASSERT_X( false, Q_FUNC_INFO, QStringLiteral(
			            "MockNetworkAccessManager: Unknown behavior for unmatched request: %1" )
			            .arg( static_cast<int>( m_unmatchedRequestBehavior ) )
			            .toLatin1().constData() );
			return Q_NULLPTR;
		// LCOV_EXCL_STOP
		}
	}
}

template< class Base >
void Manager< Base >::addHandledRequest( const Request& request )
{
	m_handledRequests.append( request );
	if ( m_signalEmitter )
		Q_EMIT m_signalEmitter->handledRequest( request );
}

template< class Base >
void Manager< Base >::addMatchedRequest( const Request& request, const Rule::Ptr& matchedRule )
{
	m_matchedRequests.append( request );
	matchedRule->m_matchedRequests.append( request );
	if ( m_signalEmitter )
		Q_EMIT m_signalEmitter->matchedRequest( request, matchedRule );
}

template< class Base >
void Manager< Base >::addUnmatchedRequest( const Request& request )
{
	m_unmatchedRequests.append( request );
	if ( m_signalEmitter )
		Q_EMIT m_signalEmitter->unmatchedRequest( request );
}

#if ( QT_VERSION >= QT_VERSION_CHECK( 5,9,0 ) )

template<class Base>
void Manager<Base>::prepareHstsHash()
{
	if ( !m_hstsHash )
	{
		m_hstsHash.reset( new QHash<QString, QHstsPolicy>() );
		QVector<QHstsPolicy> hstsPolicies = this->strictTransportSecurityHosts();

		QVector<QHstsPolicy>::const_iterator policyIter = hstsPolicies.cbegin();
		const QVector<QHstsPolicy>::const_iterator policyEnd = hstsPolicies.cend();
		for ( ; policyIter != policyEnd; ++policyIter )
		{
			if ( !policyIter->isExpired() )
				m_hstsHash->insert( policyIter->host(), *policyIter );
		}
	}
}

template<class Base>
bool Manager<Base>::elevateHstsUrl(const QUrl& url)
{
	if ( ! url.isValid() || url.scheme().toLower() != HttpUtils::httpScheme() )
		return false;

	QString host = url.host();
	const QRegularExpression ipAddressRegEx(
		QStringLiteral( "^\\[.*\\]$|^((25[0-5]|2[0-4][0-9]|1?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|1?[0-9][0-9]?)$" ) );
	if ( ipAddressRegEx.match( host ).hasMatch() )
		return false; // Don't elevate IP address URLs

	prepareHstsHash();

	// Check if there is a policy for the full host name
	QHash<QString, QHstsPolicy>::Iterator hstsHashIter = m_hstsHash->find( host );

	if ( hstsHashIter != m_hstsHash->end() )
	{
		if ( hstsHashIter.value().isExpired() )
			hstsHashIter = m_hstsHash->erase( hstsHashIter );
		else
			return true;
	}

	// Check if there is a policy for a parent domain
	QStringList domainParts = host.split( QChar::fromLatin1( '.' ), Qt::SplitBehaviorFlags::SkipEmptyParts );
	domainParts.pop_front();

	while (!domainParts.isEmpty())
	{
		hstsHashIter = m_hstsHash->find( domainParts.join( QChar::fromLatin1( '.' ) ) );
		if ( hstsHashIter != m_hstsHash->end() )
		{
			if ( hstsHashIter.value().isExpired() )
				hstsHashIter = m_hstsHash->erase( hstsHashIter );
			else if ( hstsHashIter.value().includesSubDomains() )
				return true;
			// else we continue because there could be a policy for a another parent domain that includes sub domains
		}
		domainParts.pop_front();
	}

	return false;
}

#endif // Qt >= 5.9.0

template<class Base>
QNetworkReply* Manager<Base>::createDataUrlReply( const Request& request )
{
	std::unique_ptr< QIODevice > ioDevice( createIODevice( request.body ) );
	return Base::createRequest( request.operation, request.qRequest, ioDevice.get() );
}


template<class Base>
void Manager<Base>::prepareReply(MockReply* reply, const Request& request) const
{
	reply->setBehaviorFlags(m_behaviorFlags);
	reply->prepare(request);
}

template<class Base>
void Manager<Base>::finishReply( QNetworkReply* reply, const Request& initialRequest ) const
{
	// Do we want to read out the headers synchronously for mocked replies?
	MockReply* mockedReply = ::qobject_cast<MockReply*>( reply );
	if ( mockedReply )
		m_replyHeaderHandler->handleReplyHeaders( reply );
	else
		QObject::connect( reply, SIGNAL( metaDataChanged() ), m_replyHeaderHandler.get(), SLOT( handleReplyHeaders() ) );

	const RequestList followedRedirects = reply->property( detail::followedRedirectsPropertyName() ).value<RequestList>();
	/* In case of a real QNetworkReply, we simulate the mocked redirects on the real reply.
	 * This would not work with file: or data: URLs since their real signals would have already been emitted.
	 * But automatic redirection works only for http: and https: anyway so this is not a problem.
	 */
	RequestList::const_iterator redirectIter = followedRedirects.cbegin();
	const RequestList::const_iterator redirectEnd = followedRedirects.cend();
	for ( ; redirectIter != redirectEnd; ++redirectIter )
	{
		const qint64 bodySize = redirectIter->body.size();
		if ( bodySize > 0 )
			QMetaObject::invokeMethod( reply, "uploadProgress", Qt::QueuedConnection,
			                          Q_ARG( qint64, bodySize ), Q_ARG( qint64, bodySize ) );
		QMetaObject::invokeMethod( reply, "metaDataChanged", Qt::QueuedConnection );
		QMetaObject::invokeMethod( reply, "redirected", Qt::QueuedConnection, Q_ARG( QUrl, redirectIter->qRequest.url() ) );
	}
	reply->setProperty( detail::followedRedirectsPropertyName(), QVariant() );

	#if QT_VERSION >= QT_VERSION_CHECK( 5,14,0 )
		if( this->autoDeleteReplies()
			|| initialRequest.qRequest.attribute( QNetworkRequest::AutoDeleteReplyOnFinishAttribute ).toBool() )
		{
			QObject::connect( reply, SIGNAL( finished() ), reply, SLOT( deleteLater() ) );
		}
	#endif // Qt >= 5.14.0

	if ( mockedReply )
	{
		if ( ! followedRedirects.isEmpty() )
			mockedReply->finish( followedRedirects.last() );
		else
			mockedReply->finish( initialRequest );
	}
}

template<class Base>
QIODevice* Manager<Base>::createIODevice( const QByteArray& data ) const
{
	QBuffer* buffer = Q_NULLPTR;
	if( m_inspectBody && !data.isNull() )
	{
		buffer = new QBuffer();
		buffer->setData( data );
		buffer->open( QIODevice::ReadOnly );
	}
	return buffer;
}

template<class Base>
QNetworkReply* Manager<Base>::passThrough( const Request& request, QNetworkAccessManager* overridePassThroughNam )
{
	std::unique_ptr< QIODevice > ioDevice( createIODevice( request.body ) );

	QNetworkAccessManager* passThroughNam = overridePassThroughNam
	                                        ? overridePassThroughNam
	                                        : static_cast<QNetworkAccessManager*>( m_passThroughNam );
	QNetworkReply* reply;
	if ( passThroughNam )
	{
		switch ( request.operation )
		{
		case QNetworkAccessManager::GetOperation:
			reply = passThroughNam->get( request.qRequest );
			break;
		case QNetworkAccessManager::PostOperation:
			reply = passThroughNam->post( request.qRequest, ioDevice.get() );
			break;
		case QNetworkAccessManager::PutOperation:
			reply = passThroughNam->put( request.qRequest, ioDevice.get() );
			break;
		case QNetworkAccessManager::HeadOperation:
			reply = passThroughNam->head( request.qRequest );
			break;
		case QNetworkAccessManager::DeleteOperation:
			reply = passThroughNam->deleteResource( request.qRequest );
			break;
		case QNetworkAccessManager::CustomOperation:
		default:
			reply = passThroughNam->sendCustomRequest( request.qRequest,
			                                           request.qRequest.attribute( QNetworkRequest::CustomVerbAttribute )
			                                           .toByteArray(),
			                                           ioDevice.get() );
			break;
		}
	}
	else
		reply = Base::createRequest( request.operation, request.qRequest, ioDevice.get() );
	if ( ioDevice )
	{
		QObject::connect( reply, SIGNAL( finished() ), ioDevice.get(), SLOT( deleteLater() ) );
		ioDevice.release()->setParent( reply );
	}
	QObject::connect( reply, SIGNAL( metaDataChanged() ), m_replyHeaderHandler.get(), SLOT( handleReplyHeaders() ) );
	addPassedThroughRequest( request );
	return reply;
}

template< class Base >
void Manager< Base >::addPassedThroughRequest( const Request& request )
{
	m_passedThroughRequests.append( request );
	if ( m_signalEmitter )
		Q_EMIT m_signalEmitter->passedThrough( request );
}

template<class Base>
QNetworkReply* Manager<Base>::authenticateRequest(MockReply* unauthedReply, const Request& unauthedReq)
{
	typedef QVector<HttpUtils::Authentication::Challenge::Ptr> ChallengeVector;
	ChallengeVector authChallenges = HttpUtils::Authentication::getAuthenticationChallenges(unauthedReply);

	if (authChallenges.isEmpty())
	{
		qCWarning( log ) << "Missing authentication challenge in reply"
		                 << detail::pointerToQString( unauthedReply ).toLatin1().data();
		return Q_NULLPTR;
	}

	/* Select the strongest challenge.
	 * If there are multiple challenges with the same strength,
	 * the last one is used according to the order they appear in the HTTP headers.
	 */
	std::stable_sort(authChallenges.begin(), authChallenges.end(), HttpUtils::Authentication::Challenge::StrengthCompare());
	HttpUtils::Authentication::Challenge::Ptr authChallenge = authChallenges.last();

	QAuthenticator authenticator = getAuthenticator(unauthedReply, unauthedReq, authChallenge);
	if (authenticator.user().isNull() && authenticator.password().isNull())
		return Q_NULLPTR;

	QNetworkRequest authedQReq(unauthedReq.qRequest);
	authChallenge->addAuthorization(authedQReq, unauthedReq.operation, unauthedReq.body, authenticator);
	const Request authedReq(unauthedReq.operation, authedQReq, unauthedReq.body);
	QNetworkReply* authedReply = this->handleRequest(authedReq); // POTENTIAL RECURSION
	return authedReply;
}

template<class Base>
QAuthenticator Manager<Base>::getAuthenticator(MockReply* unauthedReply, const Request& unauthedReq,
                                               const HttpUtils::Authentication::Challenge::Ptr& authChallenge)
{
	const QString realm = authChallenge->realm().toLower(); // realm is case-insensitive
	const QUrl authScope = HttpUtils::Authentication::authenticationScopeForUrl(unauthedReply->url());
	const QString authKey = realm + QChar::fromLatin1('\x1C') + authScope.toString(QUrl::FullyEncoded);
	const QNetworkRequest::LoadControl authReuse = static_cast<QNetworkRequest::LoadControl>(
	                                               unauthedReq.qRequest
	                                               .attribute(QNetworkRequest::AuthenticationReuseAttribute,
	                                               static_cast<int>(QNetworkRequest::Automatic)).toInt());

	if (authReuse == QNetworkRequest::Automatic && m_authenticationCache.contains(authKey))
		return m_authenticationCache.value(authKey);
	else
	{
		QAuthenticator authenticator;
		authenticator.setOption(HttpUtils::Authentication::Basic::realmKey(), realm);
		#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
			authenticator.setRealm(realm);
		#endif // Qt >= 5.4.0
		Q_EMIT this->authenticationRequired(unauthedReply, &authenticator);
		if (!authenticator.user().isNull() || !authenticator.password().isNull())
			m_authenticationCache.insert(authKey, authenticator);
		return authenticator;
	}
}


#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)

/*! \internal Implementation details
 */
namespace detail
{
/*! Checks if a redirect would cause a security degradation.
 * \param from The URL from which the request is redirected.
 * \param to The target URL of the redirect.
 * \return \c true if a redirect from \p from to \p to degrades protocol security (for example, HTTPS to HTTP).
 */
inline bool secureToUnsecureRedirect( const QUrl& from, const QUrl& to )
{
	return from.scheme().toLower() == HttpUtils::httpsScheme() && to.scheme().toLower() == HttpUtils::httpScheme();
}

/*! Checks if two URLs refer to the same origin.
 *
 * \param left One QUrl to compare.
 * \param right The other QUrl to compare.
 * \return \c true if \p left and \p right refer to the same origin.
 */
inline bool isSameOrigin( const QUrl& left, const QUrl& right )
{
	return left.scheme() == right.scheme()
	    && left.host()   == right.host()
	    && left.port()   == right.port();
}
}

template<class Base>
QNetworkReply* Manager<Base>::followRedirect(MockReply* prevReply, const Request& prevReq)
{
	using namespace detail;

	const QUrl prevTarget = prevReq.qRequest.url();
	const QUrl nextTarget = prevTarget.resolved( prevReply->locationHeader() );
	const QString nextTargetScheme = nextTarget.scheme().toLower();
	const QVariant statusCodeAttr = prevReply->attribute(QNetworkRequest::HttpStatusCodeAttribute);

	if (!nextTarget.isValid() || (nextTargetScheme != HttpUtils::httpScheme()
	    && nextTargetScheme != HttpUtils::httpsScheme()))
	{
		prevReply->setError(QNetworkReply::ProtocolUnknownError);
		prevReply->setAttribute(QNetworkRequest::RedirectionTargetAttribute, QVariant());
		return Q_NULLPTR;
	}

	#if ( QT_VERSION >= QT_VERSION_CHECK( 5,9,0 ) )
		const QVariant redirectPolicyAttr = prevReq.qRequest.attribute( QNetworkRequest::RedirectPolicyAttribute );
		if ( redirectPolicyAttr.isValid() )
		{
			QNetworkRequest::RedirectPolicy redirectPolicy
				= static_cast<QNetworkRequest::RedirectPolicy>( redirectPolicyAttr.toInt() );
			if ( !applyRedirectPolicy( redirectPolicy, prevReply, prevReq.qRequest, nextTarget ) )
				return Q_NULLPTR;
		}
		else
	#endif // Qt >= 5.9.0
	{
		#if ( QT_VERSION < QT_VERSION_CHECK( 6,0,0 ) && QT_DEPRECATED_SINCE( 5,15 ) )
			QVariant followRedirectsAttr = prevReq.qRequest.attribute( QNetworkRequest::FollowRedirectsAttribute );
			if ( followRedirectsAttr.isValid() )
			{
				if ( !followRedirectsAttr.toBool() )
					return Q_NULLPTR;

				if ( detail::secureToUnsecureRedirect( prevTarget, nextTarget ) )
				{
					prevReply->setError( QNetworkReply::InsecureRedirectError );
					return Q_NULLPTR;
				}
			}
			else
		#endif // Qt < 6.0.0
		{
			#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 ) )
				if ( !applyRedirectPolicy( this->redirectPolicy(), prevReply, prevReq.qRequest, nextTarget ) )
					return Q_NULLPTR;
			#else // Qt < 5.9.0
				// Following the redirect is not requested
				return Q_NULLPTR;
			#endif // Qt >= 5.9.0
		}
	}


	if (prevReq.qRequest.maximumRedirectsAllowed() <= 0)
	{
		prevReply->setError(QNetworkReply::TooManyRedirectsError);
		return Q_NULLPTR;
	}

	QNetworkAccessManager::Operation nextOperation;
	QByteArray nextReqBody;
	if (   prevReq.operation == QNetworkAccessManager::GetOperation
	    || prevReq.operation == QNetworkAccessManager::HeadOperation)
		nextOperation = prevReq.operation;
	else if (m_behaviorFlags.testFlag(Behavior_RedirectWithGet))
		// Qt up to 5.9.3 always redirects with a GET
		nextOperation = QNetworkAccessManager::GetOperation;
	else
	{
		nextOperation = prevReq.operation;
		nextReqBody = prevReq.body;

		switch (static_cast<HttpStatus::Code>(statusCodeAttr.toInt()))
		{
		case HttpStatus::TemporaryRedirect: // 307
		case HttpStatus::PermanentRedirect: // 308
			break;
		case HttpStatus::MovedPermanently:  // 301
		case HttpStatus::Found:             // 302
			if ( !m_behaviorFlags.testFlag( Behavior_IgnoreSafeRedirectMethods )
				&& usesSafeRedirectRequestMethod( prevReq ) )
			{
				break;
			}
			// Fall through
		case HttpStatus::SeeOther:          // 303
		default:
			nextOperation = QNetworkAccessManager::GetOperation;
			nextReqBody.clear();
			break;
		}

	}

	QNetworkRequest nextQReq(prevReq.qRequest);
	nextQReq.setUrl(nextTarget);
	nextQReq.setMaximumRedirectsAllowed(prevReq.qRequest.maximumRedirectsAllowed() - 1);
	if (nextOperation != QNetworkAccessManager::CustomOperation)
		nextQReq.setAttribute(QNetworkRequest::CustomVerbAttribute, QVariant());

	Request nextReq(nextOperation, nextQReq, nextReqBody);
	QNetworkReply* redirectReply = this->handleRequest(nextReq); // POTENTIAL RECURSION

	RequestList followedRedirects = redirectReply->property(followedRedirectsPropertyName()).value<RequestList>();
	followedRedirects.prepend(nextReq);
	redirectReply->setProperty(followedRedirectsPropertyName(), QVariant::fromValue(followedRedirects));

	MockReply* mockedReply = ::qobject_cast<MockReply*>(redirectReply);
	if (mockedReply)
		mockedReply->setUrl(nextQReq.url());

	return redirectReply;
}


#endif // Qt >= 5.6.0


#if QT_VERSION >= QT_VERSION_CHECK( 5, 9, 0 )

template< class Base >
bool Manager< Base >::applyRedirectPolicy( QNetworkRequest::RedirectPolicy policy, MockReply* prevReply,
                                         const QNetworkRequest& prevRequest, const QUrl& redirectTarget )
{
	const QUrl prevTarget = prevRequest.url();
	switch ( policy )
	{
	case QNetworkRequest::ManualRedirectPolicy:
		return false;
	case QNetworkRequest::NoLessSafeRedirectPolicy:
		if ( detail::secureToUnsecureRedirect( prevTarget, redirectTarget ) )
		{
			prevReply->setError( QNetworkReply::InsecureRedirectError );
			return false;
		}
		break;
	case QNetworkRequest::SameOriginRedirectPolicy:
		if ( !detail::isSameOrigin( prevTarget, redirectTarget ) )
		{
			prevReply->setError( QNetworkReply::InsecureRedirectError );
			return false;
		}
		break;
	case QNetworkRequest::UserVerifiedRedirectPolicy:
		// TODO: QNetworkRequest::UserVerifiedRedirectPolicy
		/* Does that even make sense? We would probably need to make the limitation that the
		 * QNetworkReply::redirectAllowed() signal must be emitted synchronously inside the slot handling
		 * the QNetworkReply::redirected() signal.
		 * Or we would need to return a proxy QNetworkReply from the Manager which is then "filled" and "finished" with
		 * either a MockReply or a real QNetworkReply after the redirection(s).
		 */
		qCWarning( log ) << "User verified redirection policy is not supported at the moment";
		prevReply->setError( QNetworkReply::InsecureRedirectError );
		return false;
		break;
	// LCOV_EXCL_START
	default:
		qCWarning( log ) << "Unknown redirect policy:" << policy;
		prevReply->setError( QNetworkReply::InsecureRedirectError );
		return false;
	// LCOV_EXCL_STOP
	}

	return true;
}

#endif // Qt >= 5.9.0


} // namespace MockNetworkAccess

Q_DECLARE_METATYPE( MockNetworkAccess::MockReplyBuilder )
Q_DECLARE_METATYPE( MockNetworkAccess::HttpStatus::Code )
Q_DECLARE_OPERATORS_FOR_FLAGS( MockNetworkAccess::BehaviorFlags )
Q_DECLARE_METATYPE( MockNetworkAccess::RequestList )


#endif /* MOCKNETWORKACCESSMANAGER_HPP */
