#  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)

set( KEEPASSX_TEST_TREE ${CMAKE_SOURCE_DIR}/tests )
configure_file( config-keepassx-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx-tests.h )

macro (ADD_UNIT_TEST _test_NAME)
    set(_srcList ${ARGN})
    set(_targetName ${_test_NAME})
    if( ${ARGV1} STREQUAL "TESTNAME" )
        set(_targetName ${ARGV2})
        list(REMOVE_AT _srcList 0 1)
    endif( ${ARGV1} STREQUAL "TESTNAME" )

    set(_nogui)
    list(GET ${_srcList} 0 first_PARAM)
    if( ${first_PARAM} STREQUAL "NOGUI" )
        set(_nogui "NOGUI")
    endif( ${first_PARAM} STREQUAL "NOGUI" )

    automoc4_add_executable( ${_test_NAME} ${_srcList} )

    if(NOT TEST_OUTPUT)
        set(TEST_OUTPUT plaintext)
    endif(NOT TEST_OUTPUT)
    set(TEST_OUTPUT ${TEST_OUTPUT} CACHE STRING "The output to generate when running the QTest unit tests")

    get_target_property( loc ${_test_NAME} LOCATION )

    if (KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test( ${_targetName} ${loc} -xml -o ${_targetName}.tml)
    else (KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test( ${_targetName} ${loc} )
    endif (KDE4_TEST_OUTPUT STREQUAL "xml")

    if (NOT MSVC_IDE)   #not needed for the ide
        # if the tests are EXCLUDE_FROM_ALL, add a target "buildtests" to build all tests
        if (NOT WITH_TESTS)
           get_directory_property(_buildtestsAdded BUILDTESTS_ADDED)
           if(NOT _buildtestsAdded)
              add_custom_target(buildtests)
              set_directory_properties(PROPERTIES BUILDTESTS_ADDED TRUE)
           endif(NOT _buildtestsAdded)
           add_dependencies(buildtests ${_test_NAME})
        endif (NOT WITH_TESTS)
    endif (NOT MSVC_IDE)

endmacro (ADD_UNIT_TEST)


# TODO automocify?

add_unit_test( testgroup TestGroup.cpp )
target_link_libraries( testgroup keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testkeepass2xmlreader TestKeePass2XmlReader.cpp )
target_link_libraries( testkeepass2xmlreader keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testkeepass2reader TestKeePass2Reader.cpp )
target_link_libraries( testkeepass2reader keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testgroupmodel TestGroupModel.cpp modeltest.cpp )
target_link_libraries( testgroupmodel keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testentrymodel TestEntryModel.cpp modeltest.cpp )
target_link_libraries( testentrymodel keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testcryptohash TestCryptoHash.cpp )
target_link_libraries( testcryptohash keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES} )

add_unit_test( testsymmetriccipher TestSymmetricCipher.cpp )
target_link_libraries( testsymmetriccipher keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES}  )

add_unit_test( testhashedblockstream TestHashedBlockStream.cpp )
target_link_libraries( testhashedblockstream keepassx_core ${QT_QTCORE_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTTEST_LIBRARY} ${LIBGCRYPT_LIBS} ${ZLIB_LIBRARIES}  )
