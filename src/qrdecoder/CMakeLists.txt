#  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 2 or (at your option)
#  version 3 of the License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

if(KPXC_FEATURE_QRDECODER)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    # set(qrdecoder_SOURCES
    #     QrDecoder.cpp
    #     QrDecoder.h
    #     QrTotpSetupDialog.cpp
    #     QrTotpSetupDialog.h
    # )

    file(GLOB qrdecoder_SOURCES CONFIGURE_DEPENDS
        "*.cpp"
        "*.h"
    )

    add_library(qrdecoder STATIC ${qrdecoder_SOURCES})

    find_package(ZXing CONFIG REQUIRED)
    target_link_libraries(qrdecoder Qt6::Core Qt6::Widgets ZXing::ZXing)
endif()
