#  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/../src)

set(TARGET_NAME tests)

find_package(Catch2 3 REQUIRED)

set(KEEPASSX_TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
configure_file(config-keepassx-tests.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-keepassx-tests.h)

# Macros are copy-pasted from the original tests/CMakeLists.txt as is.
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
    parse_arguments(TEST "NAME;SOURCES;LIBS;LAUNCHER" "" ${ARGN})
    set(_test_NAME ${TEST_NAME})
    set(_test_LAUNCHER ${TEST_LAUNCHER})
    set(_srcList ${TEST_SOURCES})
    add_executable(${_test_NAME} ${_srcList})
    target_link_libraries(${_test_NAME} ${TEST_LIBS})

    if(NOT TEST_OUTPUT)
        set(TEST_OUTPUT plaintext)
    endif(NOT TEST_OUTPUT)
    set(TEST_OUTPUT ${TEST_OUTPUT} CACHE STRING "The output to generate when running the unit tests")

    if(KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test(${_test_NAME} ${_test_LAUNCHER} ${_test_NAME} -xml -o ${_test_NAME}.tml)
    else(KDE4_TEST_OUTPUT STREQUAL "xml")
        add_test(${_test_NAME} ${_test_LAUNCHER} ${_test_NAME})
    endif(KDE4_TEST_OUTPUT STREQUAL "xml")

    set_tests_properties(${_test_NAME} PROPERTIES ENVIRONMENT "LANG=en_US.UTF-8")

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

set(TEST_LIBRARIES keepassx_core)

set(TEST_SUPPORT_SOURCES main.cpp)
add_library(TEST_SUPPORT STATIC ${TEST_SUPPORT_SOURCES})
target_link_libraries(TEST_SUPPORT Qt5::Core Catch2::Catch2)

# Add unit tests here.
# Naming convention: word-cased separated by dash. Avoid naming with "test", it is obvious.
add_unit_test(NAME passphrase-generator
        SOURCES core/TestPassphraseGenerator.cpp
        LIBS TEST_SUPPORT ${TEST_LIBRARIES})
