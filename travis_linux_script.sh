#!/usr/bin/env bash
#
# KeePassXC Travis Linux test script
# Copyright (C) 2017 KeePassXC team <https://keepassxc.org/>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 or (at your option)
# version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake -DCMAKE_BUILD_TYPE=${CONFIG} -DWITH_GUI_TESTS=ON -DWITH_ASAN=ON -DWITH_XC_HTTP=ON -DWITH_XC_AUTOTYPE=ON -DWITH_XC_YUBIKEY=ON $CMAKE_ARGS /keepassxc/src && \
    make -j2 && \
    make test ARGS+="-E testgui --output-on-failure" && \
    xvfb-run -a --server-args="-screen 0 800x600x24" make test ARGS+="-R testgui --output-on-failure"
