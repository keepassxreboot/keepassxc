/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 */

#ifndef KEEPASSXC_TESTPERSISTENTQUICKUNLOCK_H
#define KEEPASSXC_TESTPERSISTENTQUICKUNLOCK_H

#include <QObject>

class TestPersistentQuickUnlock : public QObject
{
    Q_OBJECT

private slots:
    void testPersistAndReload();
    void testLegacyPasswordRecord();
    void testDatabaseBinding();
    void testTamperedPayload();
    void testResetAndCredentialChange();
    void testCompositeKeyVariants();
    void testUnsupportedKey();
    void testCanceledAuthentication();
    void testDisabledByDefault();
};

#endif // KEEPASSXC_TESTPERSISTENTQUICKUNLOCK_H
