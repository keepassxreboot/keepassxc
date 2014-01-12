/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "Crypto.h"

#include <QMutex>

#include <gcrypt.h>

bool Crypto::m_initalized(false);

#if !defined(GCRYPT_VERSION_NUMBER) || (GCRYPT_VERSION_NUMBER < 0x010600)
static int gcry_qt_mutex_init(void** p_sys)
{
    *p_sys = new QMutex();
    return 0;
}

static int gcry_qt_mutex_destroy(void** p_sys)
{
    delete reinterpret_cast<QMutex*>(*p_sys);
    return 0;
}

static int gcry_qt_mutex_lock(void** p_sys)
{
    reinterpret_cast<QMutex*>(*p_sys)->lock();
    return 0;
}

static int gcry_qt_mutex_unlock(void** p_sys)
{
    reinterpret_cast<QMutex*>(*p_sys)->unlock();
    return 0;
}

static const struct gcry_thread_cbs gcry_threads_qt =
{
    GCRY_THREAD_OPTION_USER,
    0,
    gcry_qt_mutex_init,
    gcry_qt_mutex_destroy,
    gcry_qt_mutex_lock,
    gcry_qt_mutex_unlock,
    0, 0, 0, 0, 0, 0, 0, 0
};
#endif

Crypto::Crypto()
{
}

void Crypto::init()
{
    if (m_initalized) {
        qWarning("Crypto::init: already initalized");
        return;
    }

    // libgcrypt >= 1.6 doesn't allow custom thread callbacks anymore.
#if !defined(GCRYPT_VERSION_NUMBER) || (GCRYPT_VERSION_NUMBER < 0x010600)
    gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_qt);
#endif
    gcry_check_version(0);
    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    m_initalized = true;
}

bool Crypto::initalized()
{
    return m_initalized;
}

bool Crypto::selfTest()
{
    return (gcry_control(GCRYCTL_SELFTEST) == 0);
}
