/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TestAutoType.h"

#include <QPluginLoader>
#include <QTest>

#include "tests.h"
#include "core/Config.h"
#include "core/FilePath.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "crypto/Crypto.h"
#include "autotype/AutoType.h"
#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/test/AutoTypeTestInterface.h"
#include "gui/MessageBox.h"

QTEST_GUILESS_MAIN(TestAutoType)

void TestAutoType::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    AutoType::createTestInstance();
    config()->set("AutoTypeEntryTitleMatch", false);
    config()->set("security/autotypeask", false);

    QPluginLoader loader(filePath()->pluginPath("keepassx-autotype-test"));
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    QVERIFY(loader.instance());

    m_platform = qobject_cast<AutoTypePlatformInterface*>(loader.instance());
    QVERIFY(m_platform);

    m_test = qobject_cast<AutoTypeTestInterface*>(loader.instance());
    QVERIFY(m_test);

    m_autoType = AutoType::instance();
}

void TestAutoType::init()
{
    m_test->clearActions();

    m_db = new Database();
    m_dbList.clear();
    m_dbList.append(m_db);
    m_group = new Group();
    m_db->setRootGroup(m_group);

    m_entry1 = new Entry();
    m_entry1->setGroup(m_group);
    m_entry1->setUsername("myuser");
    m_entry1->setPassword("mypass");
    AutoTypeAssociations::Association association;
    association.window = "custom window";
    association.sequence = "{username}association{password}";
    m_entry1->autoTypeAssociations()->add(association);

    m_entry2 = new Entry();
    m_entry2->setGroup(m_group);
    m_entry2->setPassword("myuser");
    m_entry2->setTitle("entry title");
}

void TestAutoType::cleanup()
{
    delete m_db;
}

void TestAutoType::testInternal()
{
    QVERIFY(m_platform->activeWindowTitle().isEmpty());

    m_test->setActiveWindowTitle("Test");
    QCOMPARE(m_platform->activeWindowTitle(), QString("Test"));
}

void TestAutoType::testAutoTypeWithoutSequence()
{
    m_autoType->performAutoType(m_entry1, Q_NULLPTR);

    QCOMPARE(m_test->actionCount(), 14);
    QCOMPARE(m_test->actionChars(),
             QString("myuser%1mypass%2")
             .arg(m_test->keyToString(Qt::Key_Tab))
             .arg(m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testAutoTypeWithSequence()
{
    m_autoType->performAutoType(m_entry1, Q_NULLPTR, "{Username}abc{PaSsWoRd}");

    QCOMPARE(m_test->actionCount(), 15);
    QCOMPARE(m_test->actionChars(),
             QString("%1abc%2")
             .arg(m_entry1->username())
             .arg(m_entry1->password()));
}

void TestAutoType::testGlobalAutoTypeWithNoMatch()
{
    m_test->setActiveWindowTitle("nomatch");
    MessageBox::setNextAnswer(QMessageBox::Ok);
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString());
}

void TestAutoType::testGlobalAutoTypeWithOneMatch()
{
    m_test->setActiveWindowTitle("custom window");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(),
             QString("%1association%2")
             .arg(m_entry1->username())
             .arg(m_entry1->password()));
}

void TestAutoType::testGlobalAutoTypeTitleMatch()
{
    config()->set("AutoTypeEntryTitleMatch", true);

    m_test->setActiveWindowTitle("An Entry Title!");
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(),
             QString("%1%2").arg(m_entry2->password(), m_test->keyToString(Qt::Key_Enter)));
}

void TestAutoType::testGlobalAutoTypeTitleMatchDisabled()
{
    config()->set("AutoTypeEntryTitleMatch", false);

    m_test->setActiveWindowTitle("An Entry Title!");
    MessageBox::setNextAnswer(QMessageBox::Ok);
    m_autoType->performGlobalAutoType(m_dbList);

    QCOMPARE(m_test->actionChars(), QString());

}
