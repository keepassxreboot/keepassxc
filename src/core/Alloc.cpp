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

#include <QtGlobal>
#include <cstdint>
#include <sodium.h>
#ifdef Q_OS_MACOS
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

/**
 * Custom sized delete operator which securely zeroes out allocated
 * memory before freeing it (requires C++14 sized deallocation support).
 */
void operator delete(void* ptr, std::size_t size) noexcept
{
    if (nullptr == ptr) {
        return;
    }

#ifdef __cpp_sized_deallocation
    sodium_memzero(ptr, size);
#else
    Q_UNUSED(size);
#endif

    std::free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept
{
    ::operator delete(ptr, size);
}

/**
 * Custom delete operator which securely zeroes out
 * allocated memory before freeing it.
 */
void operator delete(void* ptr) noexcept
{
    if (nullptr == ptr) {
        return;
    }

#if defined(Q_OS_WIN)
    ::operator delete(ptr, _msize(ptr));
#elif defined(Q_OS_MACOS)
    ::operator delete(ptr, malloc_size(ptr));
#elif defined(Q_OS_UNIX)
    ::operator delete(ptr, malloc_usable_size(ptr));
#else
    // whatever OS this is, give up and simply free stuff
    std::free(ptr);
#endif
}

void operator delete[](void* ptr) noexcept
{
    ::operator delete(ptr);
}

/**
 * Custom insecure delete operator that does not zero out memory before
 * freeing a buffer. Can be used for better performance.
 */
void operator delete(void* ptr, bool) noexcept
{
    std::free(ptr);
}

void operator delete[](void* ptr, bool) noexcept
{
    ::operator delete(ptr, false);
}
