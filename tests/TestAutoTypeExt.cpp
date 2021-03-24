/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "TestAutoTypeExt.h"
#include "TestGlobal.h"

#include <QPluginLoader>

#include "autotype/AutoType.h"
#include "autotype/AutoTypeExternalPlugin.h"
#include "autotype/ext-test/AutoTypeExtTestInterface.h"
#include "core/Config.h"
#include "core/Resources.h"
#include "crypto/Crypto.h"
#include "gui/osutils/OSUtils.h"

QTEST_GUILESS_MAIN(TestAutoTypeExt)

void TestAutoTypeExt::initTestCase()
{
    QVERIFY(Crypto::init());
    Config::createTempFileInstance();
    config()->set(Config::AutoTypeDelay, 1);
    config()->set(Config::Security_AutoTypeAsk, false);
    AutoType::createTestInstance();

    QPluginLoader loader(resources()->pluginPath("keepassxc-autotype-ext-test"));
    loader.setLoadHints(QLibrary::ResolveAllSymbolsHint);
    QVERIFY(loader.instance());

    m_platform = qobject_cast<AutoTypeExternalInterface*>(loader.instance());
    QVERIFY(m_platform);

    m_test = qobject_cast<AutoTypeExtTestInterface*>(loader.instance());
    QVERIFY(m_test);

    m_autoType = AutoType::instance();
}

void TestAutoTypeExt::init()
{
    config()->set(Config::AutoTypeEntryTitleMatch, false);
    m_test->clearActions();

    m_db = QSharedPointer<Database>::create();
    m_dbList.clear();
    m_dbList.append(m_db);
    m_group = new Group();
    m_db->setRootGroup(m_group);

    AutoTypeAssociations::Association association;

    m_entry1 = new Entry();
    m_entry1->setGroup(m_group);
    m_entry1->setUsername("myuser");
    m_entry1->setPassword("mypass");
    association.window = "custom window";
    association.sequence = "{username}association{password}";
    m_entry1->autoTypeAssociations()->add(association);

    m_defaultTestMap = new AutoTypeTargetMap();
    m_defaultTestMap->append(QSharedPointer<AutoTypeTarget>::create("id1", "description1"));
    m_defaultTestMap->append(QSharedPointer<AutoTypeTarget>::create("id2", "description2"));
    m_defaultTestMap->append(QSharedPointer<AutoTypeTarget>::create("id3", "description3"));
}

void TestAutoTypeExt::cleanup()
{
}

void TestAutoTypeExt::testExtAutoType()
{
    m_test->setTargetSelectionRequired(true);
    m_autoType->performAutoTypeOnExternalPlugin(m_entry1, "ext-test", m_defaultTestMap->get("id2"));

    QCOMPARE(m_test->getActionCount(), 0);
    QCOMPARE(m_test->getActionCount("id1"), 0);
    QCOMPARE(m_test->getActionCount("id2"), 14);
    QCOMPARE(m_test->getActionCount("id3"), 0);
    QCOMPARE(m_test->getActionChars(), "");
    QCOMPARE(m_test->getActionChars("id1"), "");
    QCOMPARE(m_test->getActionChars("id2"),
             QString("myuser%1mypass%2").arg(m_test->keyToString(Qt::Key_Tab)).arg(m_test->keyToString(Qt::Key_Enter)));
    QCOMPARE(m_test->getActionChars("id3"), "");
}

void TestAutoTypeExt::testExtAutoTypeNoTargetSelection()
{
    m_test->setTargetSelectionRequired(false);
    m_autoType->performAutoTypeOnExternalPlugin(m_entry1, "ext-test", QSharedPointer<AutoTypeTarget>());

    QCOMPARE(m_test->getActionCount(), 14);
    QCOMPARE(m_test->getActionCount("id1"), 0);
    QCOMPARE(m_test->getActionCount("id2"), 0);
    QCOMPARE(m_test->getActionCount("id3"), 0);
    QCOMPARE(m_test->getActionChars(),
             QString("myuser%1mypass%2").arg(m_test->keyToString(Qt::Key_Tab)).arg(m_test->keyToString(Qt::Key_Enter)));
    QCOMPARE(m_test->getActionChars("id1"), "");
    QCOMPARE(m_test->getActionChars("id2"), "");
    QCOMPARE(m_test->getActionChars("id3"), "");
}
