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

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    SET(larg_names ${arg_names})
    LIST(FIND larg_names "${arg}" is_arg_name)
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})
      LIST(FIND loption_names "${arg}" is_option)
      IF (is_option GREATER -1)
             SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
             SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

macro (ADD_UNIT_TEST)
    parse_arguments( TEST "NAME;SOURCES;MOCS;LIBS" "" ${ARGN} )
    set(_test_NAME ${TEST_NAME})
    set(_srcList ${TEST_SOURCES})
    qt4_wrap_cpp( _srcList ${TEST_MOCS} )
    add_executable( ${_test_NAME} ${_srcList} )
    target_link_libraries( ${_test_NAME} ${TEST_LIBS} )

    if(NOT TEST_OUTPUT)
        set(TEST_OUTPUT plaintext)
    endif(NOT TEST_OUTPUT)
    set(TEST_OUTPUT ${TEST_OUTPUT} CACHE STRING "The output to generate when running the QTest unit tests")

    get_target_property( loc ${_test_NAME} LOCATION )

    if (KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test( ${_test_NAME} ${loc} -xml -o ${_test_NAME}.tml)
    else (KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test( ${_test_NAME} ${loc} )
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


set(TEST_LIBRARIES
    keepassx_core
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${GCRYPT_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

set(modeltest_SOURCRS modeltest.cpp)
qt4_wrap_cpp( modeltest_SOURCRS modeltest.h )
add_library( modeltest STATIC ${modeltest_SOURCRS} )

add_unit_test(NAME testgroup SOURCES TestGroup.cpp MOCS TestGroup.h LIBS ${TEST_LIBRARIES} )

add_unit_test(NAME testkeepass2xmlreader SOURCES TestKeePass2XmlReader.cpp MOCS TestKeePass2XmlReader.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeys SOURCES TestKeys.cpp MOCS TestKeys.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2reader SOURCES TestKeePass2Reader.cpp MOCS TestKeePass2Reader.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2writer SOURCES TestKeePass2Writer.cpp MOCS TestKeePass2Writer.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testgroupmodel SOURCES TestGroupModel.cpp MOCS TestGroupModel.h LIBS ${TEST_LIBRARIES} modeltest)

add_unit_test(NAME testentrymodel SOURCES TestEntryModel.cpp MOCS TestEntryModel.h LIBS ${TEST_LIBRARIES} modeltest)

add_unit_test(NAME testcryptohash SOURCES TestCryptoHash.cpp MOCS TestCryptoHash.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testsymmetriccipher SOURCES TestSymmetricCipher.cpp MOCS TestSymmetricCipher.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testhashedblockstream SOURCES TestHashedBlockStream.cpp MOCS TestHashedBlockStream.h LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2randomstream SOURCES TestKeePass2RandomStream.cpp MOCS TestKeePass2RandomStream.h LIBS ${TEST_LIBRARIES})
