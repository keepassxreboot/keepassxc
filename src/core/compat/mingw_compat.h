/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_MINGW_COMPAT_H
#define KEEPASSXC_MINGW_COMPAT_H

#ifdef __MINGW32__

#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

// MinGW compatibility functions
namespace KeePassXC {
namespace MinGW {

/**
 * Secure string copy for MinGW
 * @param dest Destination buffer
 * @param destsz Destination buffer size
 * @param src Source string
 * @param count Maximum characters to copy
 * @return 0 on success, non-zero on failure
 */
errno_t secure_strncpy(char* dest, size_t destsz, const char* src, size_t count);

/**
 * Secure string concatenation for MinGW
 * @param dest Destination buffer
 * @param destsz Destination buffer size
 * @param src Source string
 * @return 0 on success, non-zero on failure
 */
errno_t secure_strncat(char* dest, size_t destsz, const char* src);

/**
 * Secure sprintf for MinGW
 * @param buffer Output buffer
 * @param sizeOfBuffer Buffer size
 * @param format Format string
 * @param ... Arguments
 * @return Number of characters written, or negative on error
 */
int secure_sprintf(char* buffer, size_t sizeOfBuffer, const char* format, ...);

/**
 * Convert wide string to UTF-8 for MinGW
 * @param wstr Wide string input
 * @return UTF-8 string (caller must free with delete[])
 */
char* wide_to_utf8(const wchar_t* wstr);

/**
 * Convert UTF-8 to wide string for MinGW
 * @param str UTF-8 string input
 * @return Wide string (caller must free with delete[])
 */
wchar_t* utf8_to_wide(const char* str);

/**
 * Get temporary directory path for MinGW
 * @param buffer Buffer to store path
 * @param size Buffer size
 * @return Length of path string, or 0 on failure
 */
DWORD get_temp_path(DWORD size, char* buffer);

/**
 * Check if MinGW secure API functions are available
 * @return true if secure API is available
 */
bool has_secure_api();

} // namespace MinGW
} // namespace KeePassXC

// MinGW specific macros
#define MINGW_SECURE_STRNCPY(dest, destsz, src, count) \
    KeePassXC::MinGW::secure_strncpy(dest, destsz, src, count)

#define MINGW_SECURE_STRNCAT(dest, destsz, src) \
    KeePassXC::MinGW::secure_strncat(dest, destsz, src)

#define MINGW_SECURE_SPRINTF(buffer, size, format, ...) \
    KeePassXC::MinGW::secure_sprintf(buffer, size, format, __VA_ARGS__)

// Provide missing function declarations for older MinGW versions
#ifndef _MINGW_SECURE_API_H
#define _MINGW_SECURE_API_H

#ifdef __cplusplus
extern "C" {
#endif

// These functions are provided by modern MinGW but may be missing in older versions
#ifndef _INC_STDLIB
errno_t strcpy_s(char* dest, size_t destsz, const char* src);
errno_t strcat_s(char* dest, size_t destsz, const char* src);
errno_t strncpy_s(char* dest, size_t destsz, const char* src, size_t count);
errno_t strncat_s(char* dest, size_t destsz, const char* src, size_t count);
#endif

#ifndef _INC_STDIO
int sprintf_s(char* buffer, size_t sizeOfBuffer, const char* format, ...);
int snprintf_s(char* buffer, size_t sizeOfBuffer, size_t count, const char* format, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif // _MINGW_SECURE_API_H

#endif // __MINGW32__

#endif // KEEPASSXC_MINGW_COMPAT_H