/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#include "KeeShare.h"
#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/ssh/OpenSSHKey.h"
#include "keeshare/ShareObserver.h"
#include "keeshare/Signature.h"

#include <QMessageBox>
#include <QPainter>
#include <QPushButton>

namespace
{
    static const QString KeeShare_Reference("KeeShare/Reference");
    static const QString KeeShare_Own("KeeShare/Settings.own");
    static const QString KeeShare_Foreign("KeeShare/Settings.foreign");
    static const QString KeeShare_Active("KeeShare/Settings.active");
}

KeeShare* KeeShare::m_instance = nullptr;

KeeShare* KeeShare::instance()
{
    if (!m_instance) {
        qFatal("Race condition: instance wanted before it was initialized, this is a bug.");
    }

    return m_instance;
}

void KeeShare::init(QObject* parent)
{
    Q_ASSERT(!m_instance);
    m_instance = new KeeShare(parent);
}

KeeShareSettings::Own KeeShare::own()
{
    return KeeShareSettings::Own::deserialize(config()->get(KeeShare_Own).toString());
}

KeeShareSettings::Active KeeShare::active()
{
    return KeeShareSettings::Active::deserialize(config()->get(KeeShare_Active).toString());
}

KeeShareSettings::Foreign KeeShare::foreign()
{
    return KeeShareSettings::Foreign::deserialize(config()->get(KeeShare_Foreign).toString());
}

void KeeShare::setForeign(const KeeShareSettings::Foreign& foreign)
{
    config()->set(KeeShare_Foreign, KeeShareSettings::Foreign::serialize(foreign));
}

void KeeShare::setActive(const KeeShareSettings::Active& active)
{
    config()->set(KeeShare_Active, KeeShareSettings::Active::serialize(active));
}

void KeeShare::setOwn(const KeeShareSettings::Own& own)
{
    config()->set(KeeShare_Own, KeeShareSettings::Own::serialize(own));
}

bool KeeShare::isShared(const Group* group)
{
    return group->customData()->contains(KeeShare_Reference);
}

KeeShareSettings::Reference KeeShare::referenceOf(const Group* group)
{
    static const KeeShareSettings::Reference s_emptyReference;
    const CustomData* customData = group->customData();
    if (!customData->contains(KeeShare_Reference)) {
        return s_emptyReference;
    }
    const auto encoded = customData->value(KeeShare_Reference);
    const auto serialized = QString::fromUtf8(QByteArray::fromBase64(encoded.toLatin1()));
    KeeShareSettings::Reference reference = KeeShareSettings::Reference::deserialize(serialized);
    if (reference.isNull()) {
        qWarning("Invalid sharing reference detected - sharing disabled");
        return s_emptyReference;
    }
    return reference;
}

void KeeShare::setReferenceTo(Group* group, const KeeShareSettings::Reference& reference)
{
    CustomData* customData = group->customData();
    if (reference.isNull()) {
        customData->remove(KeeShare_Reference);
        return;
    }
    const auto serialized = KeeShareSettings::Reference::serialize(reference);
    const auto encoded = serialized.toUtf8().toBase64();
    customData->set(KeeShare_Reference, encoded);
}

QPixmap KeeShare::indicatorBadge(const Group* group, QPixmap pixmap)
{
    if (!isShared(group)) {
        return pixmap;
    }
    const auto reference = KeeShare::referenceOf(group);
    const auto active = KeeShare::active();
    const bool enabled = (reference.isImporting() && active.in) || (reference.isExporting() && active.out);
    const QPixmap badge = enabled ? databaseIcons()->iconPixmap(DatabaseIcons::SharedIconIndex)
                                  : databaseIcons()->iconPixmap(DatabaseIcons::UnsharedIconIndex);
    QImage canvas = pixmap.toImage();
    const QRectF target(canvas.width() * 0.4, canvas.height() * 0.4, canvas.width() * 0.6, canvas.height() * 0.6);
    QPainter painter(&canvas);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(target, badge, badge.rect());
    pixmap.convertFromImage(canvas);
    return pixmap;
}

QString KeeShare::referenceTypeLabel(const KeeShareSettings::Reference& reference)
{
    switch (reference.type) {
    case KeeShareSettings::Inactive:
        return tr("Disabled share");
    case KeeShareSettings::ImportFrom:
        return tr("Import from");
    case KeeShareSettings::ExportTo:
        return tr("Export to");
    case KeeShareSettings::SynchronizeWith:
        return tr("Synchronize with");
    }
    return "";
}

QString KeeShare::indicatorSuffix(const Group* group, const QString& text)
{
    // we not adjust the display name for now - it's just an alternative to the icon
    Q_UNUSED(group);
    return text;
}

void KeeShare::connectDatabase(Database* newDb, Database* oldDb)
{
    if (oldDb && m_observersByDatabase.contains(oldDb)) {
        QPointer<ShareObserver> observer = m_observersByDatabase.take(oldDb);
        if (observer) {
            delete observer;
        }
    }

    if (newDb && !m_observersByDatabase.contains(newDb)) {
        QPointer<ShareObserver> observer(new ShareObserver(newDb, newDb));
        m_observersByDatabase[newDb] = observer;
        connect(observer.data(),
                SIGNAL(sharingMessage(QString, MessageWidget::MessageType)),
                this,
                SLOT(emitSharingMessage(QString, MessageWidget::MessageType)));
    }
}

void KeeShare::handleDatabaseOpened(Database* db)
{
    QPointer<ShareObserver> observer = m_observersByDatabase.value(db);
    if (observer) {
        observer->handleDatabaseOpened();
    }
}

void KeeShare::handleDatabaseSaved(Database* db)
{
    QPointer<ShareObserver> observer = m_observersByDatabase.value(db);
    if (observer) {
        observer->handleDatabaseSaved();
    }
}

void KeeShare::emitSharingMessage(const QString& message, KMessageWidget::MessageType type)
{
    QObject* observer = sender();
    Database* db = m_databasesByObserver.value(observer);
    if (db) {
        emit sharingMessage(db, message, type);
    }
}

void KeeShare::handleDatabaseDeleted(QObject* db)
{
    auto observer = m_observersByDatabase.take(db);
    if (observer) {
        m_databasesByObserver.remove(observer);
    }
}

void KeeShare::handleObserverDeleted(QObject* observer)
{
    auto database = m_databasesByObserver.take(observer);
    if (database) {
        m_observersByDatabase.remove(database);
    }
}

void KeeShare::handleSettingsChanged(const QString& key)
{
    if (key == KeeShare_Active) {
        emit activeChanged();
    }
}

KeeShare::KeeShare(QObject* parent)
    : QObject(parent)
{
    connect(config(), SIGNAL(changed(QString)), this, SLOT(handleSettingsChanged(QString)));
}
