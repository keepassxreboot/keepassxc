/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "TestKeyScrubbing.h"

#include "keys/CompositeKey.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

#include <QTest>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <malloc.h>

/*
 * Replaces the glibc allocation functions for this executable (including the Qt libraries it loads)
 * and counts the buffers that still contain key material when they are handed back with free().
 */
extern "C" void* __libc_malloc(size_t size);
extern "C" void __libc_free(void* ptr);
extern "C" void* __libc_realloc(void* ptr, size_t size);

namespace
{
    constexpr int SecretSize = 32;
    unsigned char g_secrets[2][SecretSize];

    thread_local bool t_watching = false;
    thread_local int t_dirtyFrees = 0;
    thread_local int t_dirtyMoves = 0;

    bool containsSecret(void* ptr)
    {
        const auto size = malloc_usable_size(ptr);
        return std::any_of(std::begin(g_secrets), std::end(g_secrets), [&](const unsigned char* secret) {
            return memmem(ptr, size, secret, SecretSize) != nullptr;
        });
    }

    // Watches the current thread for the lifetime of the object
    class ReleaseWatcher
    {
    public:
        ReleaseWatcher()
        {
            t_dirtyFrees = 0;
            t_dirtyMoves = 0;
            t_watching = true;
        }
        ~ReleaseWatcher()
        {
            t_watching = false;
        }
        void stop()
        {
            t_watching = false;
        }
    };

    QByteArray secret(int index)
    {
        return {reinterpret_cast<const char*>(g_secrets[index]), SecretSize};
    }

    void initSecrets()
    {
        memset(g_secrets[0], 0xA7, SecretSize);
        memset(g_secrets[1], 0xB9, SecretSize);
    }

    // Keep the compiler from eliding allocations and writes
    void escape(void* ptr)
    {
        asm volatile("" : : "r"(ptr) : "memory");
    }

    void addKeys(CompositeKey& key)
    {
        auto passwordKey = QSharedPointer<PasswordKey>::create();
        passwordKey->setRawKey(secret(0));
        key.addKey(passwordKey);

        auto fileKey = QSharedPointer<FileKey>::create();
        fileKey->setRawKey(secret(1));
        key.addKey(fileKey);
    }
} // namespace

extern "C" void* malloc(size_t size)
{
    auto ptr = __libc_malloc(size);
    if (t_watching && ptr) {
        // Remove leftovers of earlier allocations, they must not be mistaken for new leaks
        memset(ptr, 0, malloc_usable_size(ptr));
    }
    return ptr;
}

extern "C" void free(void* ptr)
{
    if (t_watching && ptr && containsSecret(ptr)) {
        ++t_dirtyFrees;
    }
    __libc_free(ptr);
}

extern "C" void* realloc(void* ptr, size_t size)
{
    if (!t_watching || !ptr || size == 0) {
        return __libc_realloc(ptr, size);
    }
    // Always move the buffer, so that the result does not depend on the heap layout
    auto newPtr = malloc(size);
    if (newPtr) {
        memcpy(newPtr, ptr, std::min(size, malloc_usable_size(ptr)));
        if (containsSecret(ptr)) {
            ++t_dirtyMoves;
        }
        __libc_free(ptr);
    }
    return newPtr;
}

QTEST_GUILESS_MAIN(TestKeyScrubbing)

void TestKeyScrubbing::testDetectorFindsUnwipedFree()
{
    initSecrets();
    ReleaseWatcher watcher;
    auto ptr = malloc(SecretSize);
    memcpy(ptr, g_secrets[0], SecretSize);
    escape(ptr);
    free(ptr);
    watcher.stop();
    QCOMPARE(t_dirtyFrees, 1);
}

void TestKeyScrubbing::testOperatorDeleteIsScrubbed()
{
    // The operator delete override in core/Alloc.cpp wipes the memory first
    initSecrets();
    ReleaseWatcher watcher;
    auto ptr = new char[SecretSize];
    memcpy(ptr, g_secrets[0], SecretSize);
    escape(ptr);
    delete[] ptr;
    watcher.stop();
    QCOMPARE(t_dirtyFrees, 0);
}

void TestKeyScrubbing::testQByteArrayIsNotScrubbed()
{
    // QByteArray releases its buffer with free(), the operator delete override does not apply.
    // If this ever fails, wiping QByteArrays explicitly is no longer necessary.
    initSecrets();
    ReleaseWatcher watcher;
    {
        auto data = secret(0);
        escape(data.data());
    }
    watcher.stop();
    QCOMPARE(t_dirtyFrees, 1);
}

void TestKeyScrubbing::testSerialize()
{
    initSecrets();
    CompositeKey key;
    addKeys(key);

    ReleaseWatcher watcher;
    auto blob = key.serialize();
    watcher.stop();

    QVERIFY(blob.contains(secret(0)));
    QVERIFY(blob.contains(secret(1)));
    // Copies left behind when a growing QByteArray is reallocated are only reported
    qInfo("Unwiped buffers: %d freed, %d left behind by realloc", t_dirtyFrees, t_dirtyMoves);
    QCOMPARE(t_dirtyFrees, 0);
}

void TestKeyScrubbing::testDeserialize()
{
    initSecrets();
    CompositeKey key;
    addKeys(key);
    const auto blob = key.serialize();
    CompositeKey restored;

    ReleaseWatcher watcher;
    restored.setRawKey(blob);
    watcher.stop();

    QCOMPARE(restored.getKey(PasswordKey::UUID)->rawKey(), secret(0));
    QCOMPARE(restored.getKey(FileKey::UUID)->rawKey(), secret(1));
    qInfo("Unwiped buffers: %d freed, %d left behind by realloc", t_dirtyFrees, t_dirtyMoves);
    QCOMPARE(t_dirtyFrees, 0);
}
