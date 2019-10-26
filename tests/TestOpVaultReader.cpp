/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "TestOpVaultReader.h"

#include "config-keepassx-tests.h"
#include "core/Database.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "crypto/Crypto.h"
#include "format/OpVaultReader.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QTest>
#include <QUuid>

QTEST_GUILESS_MAIN(TestOpVaultReader)

QPair<QString, QString>* split1PTextExportKV(QByteArray& line)
{
    const auto eq = line.indexOf('=');
    if (-1 == eq) {
        qWarning() << "Bogus key=value pair: <<" << line << ">>";
        return nullptr;
    }
    auto k = QString::fromUtf8(line.mid(0, eq));
    const auto start = eq + 1;
    auto v = QString::fromUtf8(line.mid(start), (line.size() - 1) - start);
    return new QPair<QString, QString>(k, v);
}

QJsonArray* read1PasswordTextExport(QFile& f)
{
    auto result = new QJsonArray;
    auto current = new QJsonObject;

    if (!f.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open your text export file for reading");
        return nullptr;
    }

    while (!f.atEnd()) {
        auto line = f.readLine(1024);

        if (line.size() == 1 and line[0] == '\n') {
            if (!current->isEmpty()) {
                result->append(*current);
            }
            current = new QJsonObject;
            continue;
        }
        const auto kv = split1PTextExportKV(line);
        if (kv == nullptr) {
            break;
        }
        QString k = kv->first;

        const auto multiLine1 = line.indexOf("=\"\"");
        const auto multiLine2 = line.indexOf("=\"");
        const auto isML1 = -1 != multiLine1;
        const auto isML2 = -1 != multiLine2;
        if (isML1 or isML2) {
            QStringList lines;
            const int skipEQ = isML1 ? (multiLine1 + 3) : (multiLine2 + 2);
            lines.append(QString::fromUtf8(line.mid(skipEQ)));
            while (!f.atEnd()) {
                line = f.readLine(1024);
                const auto endMarker = line.indexOf(isML1 ? "\"\"\n" : "\"\n");
                if (-1 != endMarker) {
                    line[endMarker] = '\n';
                    lines.append(QString::fromUtf8(line.mid(0, endMarker)));
                    break;
                } else {
                    lines.append(QString::fromUtf8(line));
                }
            }
            auto v = lines.join("");
            (*current)[k] = v;
        } else {
            (*current)[k] = kv->second;
        }
        delete kv;
    }
    if (!current->isEmpty()) {
        result->append(*current);
    }
    f.close();

    return result;
}

void TestOpVaultReader::initTestCase()
{
    QVERIFY(Crypto::init());

    // https://cache.agilebits.com/security-kb/freddy-2013-12-04.tar.gz
    m_opVaultPath = QString("%1/%2").arg(KEEPASSX_TEST_DATA_DIR, "/freddy-2013-12-04.opvault");
    m_opVaultTextExportPath = QString(m_opVaultPath).replace(".opvault", ".opvault.txt");

    m_password = "freddy";

    QFile testData(m_opVaultTextExportPath);
    QJsonArray* data = read1PasswordTextExport(testData);
    QVERIFY(data);
    QCOMPARE(data->size(), 27);
    delete data;

    m_categoryMap.insert("001", "Login");
    m_categoryMap.insert("002", "Credit Card");
    m_categoryMap.insert("003", "Secure Note");
    m_categoryMap.insert("004", "Identity");
    m_categoryMap.insert("005", "Password");
    m_categoryMap.insert("099", "Tombstone");
    m_categoryMap.insert("100", "Software License");
    m_categoryMap.insert("101", "Bank Account");
    m_categoryMap.insert("102", "Database");
    m_categoryMap.insert("103", "Driver License");
    m_categoryMap.insert("104", "Outdoor License");
    m_categoryMap.insert("105", "Membership");
    m_categoryMap.insert("106", "Passport");
    m_categoryMap.insert("107", "Rewards");
    m_categoryMap.insert("108", "SSN");
    m_categoryMap.insert("109", "Router");
    m_categoryMap.insert("110", "Server");
    m_categoryMap.insert("111", "Email");
}

void TestOpVaultReader::testReadIntoDatabase()
{
    QDir opVaultDir(m_opVaultPath);

    auto reader = new OpVaultReader();
    auto db = reader->readDatabase(opVaultDir, m_password);
    QVERIFY2(!reader->hasError(), qPrintable(reader->errorString()));
    QVERIFY(db);
    QVERIFY(!db->children().isEmpty());

    Group* rootGroup = db->rootGroup();
    QVERIFY(rootGroup);

    QFile testDataFile(m_opVaultTextExportPath);
    auto testData = read1PasswordTextExport(testDataFile);
    QVERIFY(testData);

    QMap<QUuid, QJsonObject> objectsByUuid;
    QMap<QString, QList<QJsonObject>> objectsByCategory;
    for (QJsonArray::const_iterator it = testData->constBegin(); it != testData->constEnd(); ++it) {
        QJsonObject value = (*it).toObject();
        auto cat = value["category"].toString();
        QVERIFY2(m_categoryMap.contains(cat), qPrintable(QString("BOGUS, unmapped category \"%1\"").arg(cat)));

        auto catName = m_categoryMap[cat];
        if (!objectsByCategory.contains(catName)) {
            QList<QJsonObject> theList;
            objectsByCategory[catName] = theList;
        }
        objectsByCategory[catName].append(value);

        QUuid u = Tools::hexToUuid(value["uuid"].toString());
        objectsByUuid[u] = value;
    }
    delete testData;
    QCOMPARE(objectsByUuid.size(), 27);

    for (QUuid u : objectsByUuid.keys()) {
        QJsonObject o = objectsByUuid[u];
        const auto e = db->rootGroup()->findEntryByUuid(u);
        QVERIFY2(e, qPrintable(QString("Expected to find UUID %1").arg(u.toString())));

        auto jsonTitle = o["title"].toString();
        QCOMPARE(jsonTitle, e->title());
    }

    for (QString& catName : m_categoryMap.values()) {
        const auto g = rootGroup->findChildByName(catName);
        QVERIFY2(g, qPrintable(QString("Expected to find Group(%1)").arg(catName)));
        for (QJsonObject testEntry : objectsByCategory[catName]) {
            auto uuidStr = testEntry["uuid"].toString();
            auto jsonTitle = testEntry["title"].toString();

            QUuid u = Tools::hexToUuid(uuidStr);
            const auto entry = g->findEntryByUuid(u);
            QVERIFY2(entry, qPrintable(QString("Expected to find Group(%1).entry(%2)").arg(catName).arg(uuidStr)));
            QCOMPARE(entry->title(), jsonTitle);
        }
    }
}

void TestOpVaultReader::testKeyDerivation()
{
    OpVaultReader reader;
    QDir opVaultDir(m_opVaultPath);

    // yes, the reader checks this too, but in our case best to fail early
    QVERIFY(opVaultDir.exists());
    QVERIFY(opVaultDir.isReadable());

    QDir defDir = QDir(opVaultDir);
    defDir.cd("default");
    QFile profileJs(defDir.absoluteFilePath("profile.js"));
    QVERIFY(profileJs.exists());

    auto profileObj = reader.readAndAssertJsonFile(profileJs, "var profile=", ";");

    QByteArray salt = QByteArray::fromBase64(profileObj["salt"].toString().toUtf8());
    unsigned long iter = profileObj["iterations"].toInt();
    const auto derived = reader.deriveKeysFromPassPhrase(salt, m_password, iter);
    QVERIFY(derived);
    QVERIFY(!derived->error);

    QByteArray encHex = derived->encrypt.toHex();
    QByteArray hmacHex = derived->hmac.toHex();
    delete derived;

    QCOMPARE(QString::fromUtf8(encHex),
             QStringLiteral("63b075de858949559d4faa9d348bf10bdaa0e567ad943d7803f2291c9342aaaa"));
    QCOMPARE(QString::fromUtf8(hmacHex),
             QStringLiteral("ff3ab426ce55bf097b252b3f2df1c4ba4312a6960180844d7a625bc0ab40c35e"));
}

void TestOpVaultReader::testBandEntry1()
{
    auto reader = new OpVaultReader();
    QByteArray json(R"({"hello": "world"})");
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QJsonObject data;
    QByteArray entryKey;
    QByteArray entryHmacKey;
    QVERIFY(!reader->decryptBandEntry(doc.object(), data, entryKey, entryHmacKey));
}
