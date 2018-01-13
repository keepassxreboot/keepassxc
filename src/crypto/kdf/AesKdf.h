/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AESKDF_H
#define KEEPASSX_AESKDF_H

#include "Kdf.h"

class AesKdf: public Kdf
{
public:
    AesKdf();
    explicit AesKdf(bool legacyKdbx3);

    bool processParameters(const QVariantMap& p) override;
    QVariantMap writeParameters() override;
    bool transform(const QByteArray& raw, QByteArray& result) const override;
    QSharedPointer<Kdf> clone() const override;

protected:
    int benchmarkImpl(int msec) const override;

private:
    static bool transformKeyRaw(const QByteArray& key,
                                const QByteArray& seed,
                                int rounds,
                                QByteArray* result) Q_REQUIRED_RESULT;
};

#endif // KEEPASSX_AESKDF_H
