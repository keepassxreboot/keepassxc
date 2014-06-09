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

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src)

add_definitions(-DQT_TEST_LIB)

set(KEEPASSX_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
configure_file(config-keepassx-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx-tests.h)

macro(parse_arguments prefix arg_names option_names)
  set(DEFAULT_ARGS)
  foreach(arg_name ${arg_names})
    set(${prefix}_${arg_name})
  endforeach(arg_name)
  foreach(option ${option_names})
    set(${prefix}_${option} FALSE)
  endforeach(option)

  set(current_arg_name DEFAULT_ARGS)
  set(current_arg_list)
  foreach(arg ${ARGN})
    set(larg_names ${arg_names})
    list(FIND larg_names "${arg}" is_arg_name)
    if(is_arg_name GREATER -1)
      set(${prefix}_${current_arg_name} ${current_arg_list})
      set(current_arg_name ${arg})
      set(current_arg_list)
    else()
      set(loption_names ${option_names})
      list(FIND loption_names "${arg}" is_option)
      if(is_option GREATER -1)
        set(${prefix}_${arg} TRUE)
      else(is_option GREATER -1)
        set(current_arg_list ${current_arg_list} ${arg})
      endif()
    endif()
  endforeach(arg)
  set(${prefix}_${current_arg_name} ${current_arg_list})
endmacro(parse_arguments)

macro(add_unit_test)
  parse_arguments(TEST "NAME;SOURCES;MOCS;LIBS" "" ${ARGN})
  set(_test_NAME ${TEST_NAME})
  set(_srcList ${TEST_SOURCES})
  qt4_wrap_cpp(_srcList ${TEST_MOCS})
  add_executable(${_test_NAME} ${_srcList})
  target_link_libraries(${_test_NAME} ${TEST_LIBS})

  if(NOT TEST_OUTPUT)
    set(TEST_OUTPUT plaintext)
  endif(NOT TEST_OUTPUT)
  set(TEST_OUTPUT ${TEST_OUTPUT} CACHE STRING "The output to generate when running the QTest unit tests")

  get_target_property(loc ${_test_NAME} LOCATION)

  if(KDE4_TEST_OUTPUT STREQUAL "xml")
    add_test(${_test_NAME} ${loc} -xml -o ${_test_NAME}.tml)
  else(KDE4_TEST_OUTPUT STREQUAL "xml")
    add_test(${_test_NAME} ${loc})
  endif(KDE4_TEST_OUTPUT STREQUAL "xml")

  if(NOT MSVC_IDE)   #not needed for the ide
    # if the tests are EXCLUDE_FROM_ALL, add a target "buildtests" to build all tests
    if(NOT WITH_TESTS)
      get_directory_property(_buildtestsAdded BUILDTESTS_ADDED)
      if(NOT _buildtestsAdded)
        add_custom_target(buildtests)
        set_directory_properties(PROPERTIES BUILDTESTS_ADDED TRUE)
      endif()
      add_dependencies(buildtests ${_test_NAME})
    endif()
  endif()
endmacro(add_unit_test)


set(TEST_LIBRARIES
    keepassx_core
    ${QT_QTCORE_LIBRARY}
    ${QT_QTGUI_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${GCRYPT_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if(UNIX AND NOT APPLE)
  set(TEST_LIBRARIES ${TEST_LIBRARIES} ${QT_QTDBUS_LIBRARY})
endif()

set(modeltest_SOURCRS modeltest.cpp)
qt4_wrap_cpp(modeltest_SOURCRS modeltest.h)
add_library(modeltest STATIC ${modeltest_SOURCRS})

add_unit_test(NAME testgroup SOURCES TestGroup.cpp MOCS TestGroup.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2xmlreader SOURCES TestKeePass2XmlReader.cpp MOCS TestKeePass2XmlReader.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeys SOURCES TestKeys.cpp MOCS TestKeys.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2reader SOURCES TestKeePass2Reader.cpp MOCS TestKeePass2Reader.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2writer SOURCES TestKeePass2Writer.cpp MOCS TestKeePass2Writer.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testgroupmodel SOURCES TestGroupModel.cpp MOCS TestGroupModel.h
              LIBS modeltest ${TEST_LIBRARIES})

add_unit_test(NAME testentrymodel SOURCES TestEntryModel.cpp MOCS TestEntryModel.h
              LIBS modeltest ${TEST_LIBRARIES})

add_unit_test(NAME testcryptohash SOURCES TestCryptoHash.cpp MOCS TestCryptoHash.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testsymmetriccipher SOURCES TestSymmetricCipher.cpp MOCS TestSymmetricCipher.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testhashedblockstream SOURCES TestHashedBlockStream.cpp MOCS TestHashedBlockStream.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass2randomstream SOURCES TestKeePass2RandomStream.cpp MOCS TestKeePass2RandomStream.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testmodified SOURCES TestModified.cpp MOCS TestModified.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testdeletedobjects SOURCES TestDeletedObjects.cpp MOCS TestDeletedObjects.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testkeepass1reader SOURCES TestKeePass1Reader.cpp MOCS TestKeePass1Reader.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testqsavefile SOURCES TestQSaveFile.cpp MOCS TestQSaveFile.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testwildcardmatcher SOURCES TestWildcardMatcher.cpp MOCS TestWildcardMatcher.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testautotype SOURCES TestAutoType.cpp MOCS TestAutoType.h
              LIBS ${TEST_LIBRARIES})
set_target_properties(testautotype PROPERTIES ENABLE_EXPORTS ON)

add_unit_test(NAME testentry SOURCES TestEntry.cpp MOCS TestEntry.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testqcommandlineparser SOURCES TestQCommandLineParser.cpp MOCS TestQCommandLineParser.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testrandom SOURCES TestRandom.cpp MOCS TestRandom.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testentrysearcher SOURCES TestEntrySearcher.cpp MOCS TestEntrySearcher.h
              LIBS ${TEST_LIBRARIES})

add_unit_test(NAME testexporter SOURCES TestExporter.cpp MOCS TestExporter.h
              LIBS ${TEST_LIBRARIES})

if(WITH_GUI_TESTS)
  add_subdirectory(gui)
endif(WITH_GUI_TESTS)
