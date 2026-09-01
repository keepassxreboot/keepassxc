/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#ifndef KEEPASSXC_HTTPSTATUS_H
#define KEEPASSXC_HTTPSTATUS_H

// Shared HTTP status code constants for the remote sync providers and their
// login/transport helpers, so the same named values are used everywhere
// instead of bare integer literals.
namespace HttpStatus
{
    constexpr int Ok = 200;
    constexpr int Created = 201;
    constexpr int NoContent = 204;
    constexpr int MultiStatus = 207;
    constexpr int BadRequest = 400;
    constexpr int Unauthorized = 401;
    constexpr int Forbidden = 403;
    constexpr int NotFound = 404;
    constexpr int Conflict = 409;
    constexpr int Gone = 410;
    constexpr int PreconditionFailed = 412;
    constexpr int Locked = 423;
    constexpr int TooManyRequests = 429;
    constexpr int InsufficientStorage = 507;

    // 3xx redirect range (300-399).
    constexpr bool isRedirect(int status)
    {
        return status >= 300 && status < 400;
    }

    // 5xx server error range (500-599).
    constexpr bool isServerError(int status)
    {
        return status >= 500 && status < 600;
    }
} // namespace HttpStatus

#endif // KEEPASSXC_HTTPSTATUS_H
