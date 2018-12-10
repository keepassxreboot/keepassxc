/*
 *  Copyright (C) 2018 Sami Vänttinen <sami.vanttinen@protonmail.com>
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

#include "TestBrowser.h"
#include "TestGlobal.h"
#include "crypto/Crypto.h"
#include "sodium/crypto_box.h"
#include "browser/BrowserSettings.h"
#include <QString>

QTEST_GUILESS_MAIN(TestBrowser)

const QString PUBLICKEY = "UIIPObeoya1G8g1M5omgyoPR/j1mR1HlYHu0wHCgMhA=";
const QString SECRETKEY = "B8ei4ZjQJkWzZU2SK/tBsrYRwp+6ztEMf5GFQV+i0yI=";
const QString SERVERPUBLICKEY = "lKnbLhrVCOqzEjuNoUz1xj9EZlz8xeO4miZBvLrUPVQ=";
const QString SERVERSECRETKEY = "tbPQcghxfOgbmsnEqG2qMIj1W2+nh+lOJcNsHncaz1Q=";
const QString NONCE = "zBKdvTjL5bgWaKMCTut/8soM/uoMrFoZ";
const QString CLIENTID = "testClient";

void TestBrowser::initTestCase()
{
    QVERIFY(Crypto::init());
    m_browserService.reset(new BrowserService(nullptr));
    m_browserAction.reset(new BrowserAction(*m_browserService.data()));
}

void TestBrowser::cleanupTestCase()
{

}

/**
 * Tests for BrowserAction
 */

void TestBrowser::testChangePublicKeys()
{
    QJsonObject json;
    json["action"] = "change-public-keys";
    json["publicKey"] = PUBLICKEY;
    json["nonce"] = NONCE;

    auto response = m_browserAction->handleAction(json);
    QCOMPARE(response["action"], "change-public-keys");
    QCOMPARE(response["publicKey"].toString() == PUBLICKEY, false);
    QCOMPARE(response["success"], "true");
}

void TestBrowser::testEncryptMessage()
{
    QJsonObject message;
    message["action"] = "test-action";

    m_browserAction->setKeys(SERVERPUBLICKEY, SERVERSECRETKEY, PUBLICKEY);
    auto encrypted = m_browserAction->encryptMessage(message, NONCE);

    QCOMPARE(encrypted, "+zjtntnk4rGWSl/Ph7Vqip/swvgeupk4lNgHEm2OO3ujNr0OMz6eQtGwjtsj+/rP");
}

void TestBrowser::testDecryptMessage()
{
    QString message = "+zjtntnk4rGWSl/Ph7Vqip/swvgeupk4lNgHEm2OO3ujNr0OMz6eQtGwjtsj+/rP";
    m_browserAction->setKeys(SERVERPUBLICKEY, SERVERSECRETKEY, PUBLICKEY);
    auto decrypted = m_browserAction->decryptMessage(message, NONCE);

    QCOMPARE(decrypted["action"], "test-action");
}

void TestBrowser::testGetBase64FromKey()
{
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    
    for (unsigned int i = 0; i < crypto_box_PUBLICKEYBYTES; ++i) {
        pk[i] = i;
    }

    auto response = m_browserAction->getBase64FromKey(pk, crypto_box_PUBLICKEYBYTES);
    QCOMPARE(response, "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=");
}

void TestBrowser::testIncrementNonce()
{
    auto result = m_browserAction->incrementNonce(NONCE);
    QCOMPARE(result, "zRKdvTjL5bgWaKMCTut/8soM/uoMrFoZ");
}

/**
 * Tests for BrowserService
 */
void TestBrowser::testBaseDomain()
{
    QString url1 = "https://another.example.co.uk";
    QString url2 = "https://www.example.com";
    QString url3 = "http://test.net";
    QString url4 = "http://so.many.subdomains.co.jp";

    QString res1 = m_browserService->baseDomain(url1);
    QString res2 = m_browserService->baseDomain(url2);
    QString res3 = m_browserService->baseDomain(url3);
    QString res4 = m_browserService->baseDomain(url4);

    QCOMPARE(res1, "example.co.uk");
    QCOMPARE(res2, "example.com");
    QCOMPARE(res3, "test.net");
    QCOMPARE(res4, "subdomains.co.jp");
}

void TestBrowser::testSortPriority()
{
    QString host = "github.com";
    QString submitUrl = "https://github.com/session";
    QString baseSubmitUrl = "https://github.com";

    Entry* entry1 = new Entry();
    Entry* entry2 = new Entry();
    Entry* entry3 = new Entry();
    Entry* entry4 = new Entry();
    Entry* entry5 = new Entry();
    Entry* entry6 = new Entry();
    Entry* entry7 = new Entry();
    Entry* entry8 = new Entry();
    Entry* entry9 = new Entry();
    Entry* entry10 = new Entry();

    entry1->setUrl("https://github.com/login");
    entry2->setUrl("https://github.com/login");
    entry3->setUrl("https://github.com/");
    entry4->setUrl("github.com/login");
    entry5->setUrl("http://github.com");
    entry6->setUrl("http://github.com/login");
    entry7->setUrl("github.com");
    entry8->setUrl("github.com/login");
    entry9->setUrl("https://github");
    entry10->setUrl("github.com");

    // The extension uses the submitUrl as default for comparison
    auto res1 = m_browserService->sortPriority(entry1, host, "https://github.com/login", baseSubmitUrl);
    auto res2 = m_browserService->sortPriority(entry2, host, submitUrl, baseSubmitUrl);
    auto res3 = m_browserService->sortPriority(entry3, host, submitUrl, baseSubmitUrl);
    auto res4 = m_browserService->sortPriority(entry4, host, submitUrl, baseSubmitUrl);
    auto res5 = m_browserService->sortPriority(entry5, host, submitUrl, baseSubmitUrl);
    auto res6 = m_browserService->sortPriority(entry6, host, submitUrl, baseSubmitUrl);
    auto res7 = m_browserService->sortPriority(entry7, host, submitUrl, baseSubmitUrl);
    auto res8 = m_browserService->sortPriority(entry8, host, submitUrl, baseSubmitUrl);
    auto res9 = m_browserService->sortPriority(entry9, host, submitUrl, baseSubmitUrl);
    auto res10 = m_browserService->sortPriority(entry10, host, submitUrl, baseSubmitUrl);

    QCOMPARE(res1, 100);
    QCOMPARE(res2, 40);
    QCOMPARE(res3, 90);
    QCOMPARE(res4, 0);
    QCOMPARE(res5, 0);
    QCOMPARE(res6, 0);
    QCOMPARE(res7, 0);
    QCOMPARE(res8, 0);
    QCOMPARE(res9, 90);
    QCOMPARE(res10, 0);
}

void TestBrowser::testSearchEntries()
{
    QSharedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    QList<QString> urls;
    urls.push_back("https://github.com/login_page");
    urls.push_back("https://github.com/login");
    urls.push_back("https://github.com/");
    urls.push_back("github.com/login");
    urls.push_back("http://github.com");
    urls.push_back("http://github.com/login");
    urls.push_back("github.com");
    urls.push_back("github.com/login");
    urls.push_back("https://github");
    urls.push_back("github.com");

    for (int i = 0; i < urls.length(); ++i) {
        Entry* entry = new Entry();
        entry->setGroup(root);
        entry->beginUpdate();
        entry->setUrl(urls[i]);
        entry->setUsername(QString("User %1").arg(i));
        entry->endUpdate();
    }

    browserSettings()->setMatchUrlScheme(false);
    auto result = m_browserService->searchEntries(db, "github.com", "https://github.com");    // db, hostname, url
    QCOMPARE(result.length(), 7);
    QCOMPARE(result[0]->url(), "https://github.com/login_page");
    QCOMPARE(result[1]->url(), "https://github.com/login");
    QCOMPARE(result[2]->url(), "https://github.com/");
    QCOMPARE(result[3]->url(), "http://github.com");
    QCOMPARE(result[4]->url(), "http://github.com/login");
    QCOMPARE(result[5]->url(), "github.com");
    QCOMPARE(result[6]->url(), "github.com");

    // With matching there should be only 5 results
    browserSettings()->setMatchUrlScheme(true);
    result = m_browserService->searchEntries(db, "github.com", "https://github.com");    // db, hostname, url
    QCOMPARE(result.length(), 5);
    QCOMPARE(result[0]->url(), "https://github.com/login_page");
    QCOMPARE(result[1]->url(), "https://github.com/login");
    QCOMPARE(result[2]->url(), "https://github.com/");
    QCOMPARE(result[3]->url(), "github.com");   // These should be ignored.. or kept as wildcard?
    QCOMPARE(result[4]->url(), "github.com");
}

void TestBrowser::testSearchEntriesWithPort()
{
    QSharedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    QList<QString> urls;
    urls.push_back("http://127.0.0.1:443");
    urls.push_back("http://127.0.0.1:80");

    for (int i = 0; i < urls.length(); ++i) {
        Entry* entry = new Entry();
        entry->setGroup(root);
        entry->beginUpdate();
        entry->setUrl(urls[i]);
        entry->setUsername(QString("User %1").arg(i));
        entry->endUpdate();
    }

    auto result = m_browserService->searchEntries(db, "127.0.0.1", "http://127.0.0.1:443");    // db, hostname, url
    QCOMPARE(result.length(), 1);
    QCOMPARE(result[0]->url(), "http://127.0.0.1:443");
}

void TestBrowser::testSortEntries()
{
    QSharedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    QList<QString> urls;
    urls.push_back("https://github.com/login_page");
    urls.push_back("https://github.com/login");
    urls.push_back("https://github.com/");
    urls.push_back("github.com/login");
    urls.push_back("http://github.com");
    urls.push_back("http://github.com/login");
    urls.push_back("github.com");
    urls.push_back("github.com/login");
    urls.push_back("https://github");
    urls.push_back("github.com");

    QList<Entry*> entries;
    for (int i = 0; i < urls.length(); ++i) {
        Entry* entry = new Entry();
        entry->setGroup(root);
        entry->beginUpdate();
        entry->setUrl(urls[i]);
        entry->setUsername(QString("User %1").arg(i));
        entry->endUpdate();
        entries.push_back(entry);
    }

    auto result = m_browserService->sortEntries(entries, "github.com", "https://github.com/session");    // entries, host, submitUrl
    QCOMPARE(result.length(), 2);
    QCOMPARE(result[0]->username(), "User 2");
    QCOMPARE(result[0]->url(), "https://github.com/");
    QCOMPARE(result[1]->username(), "User 8");
    QCOMPARE(result[1]->url(), "https://github");
}

void TestBrowser::testGetDatabaseGroups()
{
    QSharedPointer<Database> db(new Database());
    auto* root = db->rootGroup();

    Group* group1 = new Group();
    group1->setParent(root);
    group1->setName("group1");

    Group* group2 = new Group();
    group2->setParent(root);
    group2->setName("group2");

    Group* group3 = new Group();
    group3->setParent(root);
    group3->setName("group3");

    Group* group2_1 = new Group();
    group2_1->setParent(group2);
    group2_1->setName("group2_1");

    Group* group2_2 = new Group();
    group2_2->setParent(group2);
    group2_2->setName("group2_2");

    Group* group2_1_1 = new Group();
    group2_1_1->setParent(group2_1);
    group2_1_1->setName("group2_1_1");

    auto result = m_browserService->getDatabaseGroups(db);
    QCOMPARE(result.length(), 1);
    
    auto groups = result["groups"].toArray();
    auto first = groups.at(0);
    auto children = first["children"].toArray();
    QCOMPARE(first["name"], "Root");
    QCOMPARE(children.size(), 3);

    auto firstChild = children.at(0);
    auto secondChild = children.at(1);
    auto thirdChild = children.at(2);
    QCOMPARE(firstChild["name"], "group1");
    QCOMPARE(secondChild["name"], "group2");
    QCOMPARE(thirdChild["name"], "group3");

    auto childrenOfSecond = secondChild["children"].toArray();
    auto firstOfCOS = childrenOfSecond.at(0);
    auto secondOfCOS = childrenOfSecond.at(1);
    QCOMPARE(firstOfCOS["name"], "group2_1");
    QCOMPARE(secondOfCOS["name"], "group2_2");

    auto lastChildren = firstOfCOS["children"].toArray();
    auto lastChild = lastChildren.at(0);
    QCOMPARE(lastChild["name"], "group2_1_1");
}
