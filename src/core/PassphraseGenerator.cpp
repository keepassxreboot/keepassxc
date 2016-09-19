/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "PassphraseGenerator.h"

#include "crypto/Random.h"

PassphraseGenerator::PassphraseGenerator()
    : m_length(0)
{
}

void PassphraseGenerator::setLength(int length)
{
    m_length = length;
}

QString PassphraseGenerator::generatePassphrase() const
{
    Q_ASSERT(isValid());

    QString passphrase;
    for (int i = 0; i < m_length; i ++) {
      //int word = randomGen()->randomUInt(7776);
      passphrase.append("foobar");

      if(i < m_length - 1) {
        passphrase.append(" ");
      }
    }

    return passphrase;
}

bool PassphraseGenerator::isValid() const
{
    if (m_length == 0) {
        return false;
    }

    return true;
}
