/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#include "TestFdoSecrets.h"

#include "core/EntrySearcher.h"
#include "core/Group.h"
#include "crypto/Random.h"
#include "fdosecrets/objects/Collection.h"
#include "fdosecrets/objects/SessionCipher.h"

#include <QTest>

QTEST_GUILESS_MAIN(TestFdoSecrets)

void TestFdoSecrets::testDhIetf1024Sha256Aes128CbcPkcs7()
{
    FdoSecrets::DhIetf1024Sha256Aes128CbcPkcs7 cipher(randomGen()->randomArray(128));
    QVERIFY(cipher.isValid());
}

void TestFdoSecrets::testCrazyAttributeKey()
{
    using FdoSecrets::Collection;
    using FdoSecrets::Item;

    const QScopedPointer<Group> root(new Group());
    const QScopedPointer<Entry> e1(new Entry());
    e1->setGroup(root.data());

    const QString key = "_a:bc&-+'-e%12df_d";
    const QString value = "value";
    e1->attributes()->set(key, value);

    // search for custom entries
    const auto term = Collection::attributeToTerm(key, value);
    const auto res = EntrySearcher().search({term}, root.data());
    QCOMPARE(res.count(), 1);
}

void TestFdoSecrets::testSpecialCharsInAttributeValue()
{
    using FdoSecrets::Collection;
    using FdoSecrets::Item;

    const QScopedPointer<Group> root(new Group());
    QScopedPointer<Entry> e1(new Entry());
    e1->setGroup(root.data());

    e1->setTitle("titleA");
    e1->attributes()->set("testAttribute", "OAuth::[test.name@gmail.com]");

    QScopedPointer<Entry> e2(new Entry());
    e2->setGroup(root.data());
    e2->setTitle("titleB");
    e2->attributes()->set("testAttribute", "Abc:*+.-");

    // search for custom entries via programmatic API
    {
        const auto term = Collection::attributeToTerm("testAttribute", "OAuth::[test.name@gmail.com]");
        const auto res = EntrySearcher().search({term}, root.data());
        QCOMPARE(res.count(), 1);
        QCOMPARE(res[0]->title(), QStringLiteral("titleA"));
    }
    {
        const auto term = Collection::attributeToTerm("testAttribute", "Abc:*+.-");
        const auto res = EntrySearcher().search({term}, root.data());
        QCOMPARE(res.count(), 1);
        QCOMPARE(res[0]->title(), QStringLiteral("titleB"));
    }
    {
        const auto term = Collection::attributeToTerm("testAttribute", "v|");
        const auto res = EntrySearcher().search({term}, root.data());
        QCOMPARE(res.count(), 0);
    }
}

void TestFdoSecrets::testDBusPathParse()
{
    using FdoSecrets::DBusMgr;
    using PathType = FdoSecrets::DBusMgr::PathType;

    auto parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets"));
    QCOMPARE(parsed.type, PathType::Service);

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets/collection/xxx"));
    QCOMPARE(parsed.type, PathType::Collection);
    QCOMPARE(parsed.id, QStringLiteral("xxx"));

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets/collection/xxx/yyy"));
    QCOMPARE(parsed.type, PathType::Item);
    QCOMPARE(parsed.id, QStringLiteral("yyy"));
    QCOMPARE(parsed.parentId, QStringLiteral("xxx"));

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets/aliases/xxx"));
    QCOMPARE(parsed.type, PathType::Aliases);
    QCOMPARE(parsed.id, QStringLiteral("xxx"));

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets/session/xxx"));
    QCOMPARE(parsed.type, PathType::Session);
    QCOMPARE(parsed.id, QStringLiteral("xxx"));

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/secrets/prompt/xxx"));
    QCOMPARE(parsed.type, PathType::Prompt);
    QCOMPARE(parsed.id, QStringLiteral("xxx"));

    parsed = DBusMgr::parsePath(QStringLiteral("/org/freedesktop/other/prompt/xxx"));
    QCOMPARE(parsed.type, PathType::Unknown);

    parsed = DBusMgr::parsePath(QStringLiteral("/org"));
    QCOMPARE(parsed.type, PathType::Unknown);
}
