/*
*  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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
#include "catch2/catch_all.hpp"
#include "crypto/Crypto.h"

#include <QCoreApplication>

int main(int argc, char** argv)
{
    Crypto::init();
    QCoreApplication app(argc, argv);

    // Add your common setup for all unit tests here...

    int result = Catch::Session().run(argc, argv);

    // Add your clean-up here...

    return result;
}