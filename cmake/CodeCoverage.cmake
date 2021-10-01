# Copyright (c) 2012 - 2017, Lars Bilke
# Copyright (c) 2021 KeePassXC Team
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(CMakeParseArguments)

# Check prereqs
find_program(GCOV_PATH gcov)
find_program(LLVM_COV_PATH llvm-cov)
find_program(LLVM_PROFDATA_PATH llvm-profdata)
find_program(XCRUN_PATH xcrun)
find_program(GENHTML_PATH NAMES genhtml genhtml.perl genhtml.bat)
find_program(GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test)

set(COVERAGE_COMPILER_FLAGS "-g -O0" CACHE INTERNAL "")
if(CMAKE_COMPILER_IS_GNUCXX)
    set(COVERAGE_COMPILER_FLAGS "${COVERAGE_COMPILER_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
elseif(CMAKE_COMPILER_IS_CLANGXX)
    set(COVERAGE_COMPILER_FLAGS "${COVERAGE_COMPILER_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
endif()

set(CMAKE_COVERAGE_FORMAT
    "html" "xml"
    CACHE STRING "Coverage report output format.")
set_property(CACHE CMAKE_COVERAGE_FORMAT PROPERTY STRINGS "html" "txt")

set(CMAKE_CXX_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C++ compiler during coverage builds.")
set(CMAKE_C_FLAGS_COVERAGE
    ${COVERAGE_COMPILER_FLAGS}
    CACHE STRING "Flags used by the C compiler during coverage builds.")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds.")
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used by the shared libraries linker during coverage builds.")
mark_as_advanced(
    CMAKE_COVERAGE_FORMAT
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE)

if(NOT CMAKE_BUILD_TYPE_LOWER STREQUAL "debug")
    message(WARNING "Code coverage results with an optimised (non-Debug) build may be misleading")
endif() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"

if(CMAKE_COMPILER_IS_GNUCXX)
    if(NOT GCOV_PATH)
        message(FATAL_ERROR "gcov not found! Aborting...")
    endif() # NOT GCOV_PATH
    link_libraries(gcov)
endif()


# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE_GCOVR(
#     NAME ctest_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES executable_target         # Dependencies to build first
# )
function(SETUP_TARGET_FOR_COVERAGE_GCOVR)

    set(options NONE)
    set(oneValueArgs NAME SOURCES_ROOT)
    set(multiValueArgs EXECUTABLE EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT GCOVR_PATH)
        message(FATAL_ERROR "gcovr not found! Aborting...")
    endif() # NOT GCOVR_PATH

    # Combine excludes to several -e arguments
    set(GCOVR_EXCLUDES "")
    foreach(EXCLUDE ${COVERAGE_EXCLUDES})
        list(APPEND GCOVR_EXCLUDES "-e")
        list(APPEND GCOVR_EXCLUDES "${EXCLUDE}")
    endforeach()

    add_custom_target(${Coverage_NAME}
        # Run tests
        COMMAND ctest -C $<CONFIG> $ENV{ARGS} $$ARGS

        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        DEPENDS ${Coverage_DEPENDENCIES}
    )

    if("html" IN_LIST CMAKE_COVERAGE_FORMAT)
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            # Create folder
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/${Coverage_NAME}-html

            # Running gcovr HTML
            COMMAND ${GCOVR_PATH} --html --html-details
                -r ${Coverage_SOURCES_ROOT} ${GCOVR_EXCLUDES}
                --object-directory=${PROJECT_BINARY_DIR}
                --exclude-unreachable-branches --exclude-throw-branches
                -o ${Coverage_NAME}-html/index.html
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Running gcovr to produce HTML code coverage report ${Coverage_NAME}-html."
        )
    endif()

    if("xml" IN_LIST CMAKE_COVERAGE_FORMAT)
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            # Running gcovr TXT
            COMMAND ${GCOVR_PATH} --xml
                -r ${Coverage_SOURCES_ROOT} ${GCOVR_EXCLUDES}
                --object-directory=${PROJECT_BINARY_DIR}
                --exclude-unreachable-branches --exclude-throw-branches
                -o ${Coverage_NAME}.xml
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Running gcovr to produce XML code coverage report ${Coverage_NAME}.xml."
        )
    endif()

    if("txt" IN_LIST CMAKE_COVERAGE_FORMAT)
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            # Running gcovr TXT
            COMMAND ${GCOVR_PATH}
                -r ${Coverage_SOURCES_ROOT} ${GCOVR_EXCLUDES}
                --object-directory=${PROJECT_BINARY_DIR}
                --exclude-unreachable-branches --exclude-throw-branches
                -o ${Coverage_NAME}.txt
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Running gcovr to produce TXT code coverage report ${Coverage_NAME}.txt."
        )
    endif()

endfunction() # SETUP_TARGET_FOR_COVERAGE_GCOVR

# Defines a target for running and collection code coverage information
# Builds dependencies, runs the given executable and outputs reports.
# NOTE! The executable should always have a ZERO as exit code otherwise
# the coverage generation will not complete.
#
# SETUP_TARGET_FOR_COVERAGE_LLVM(
#     NAME ctest_coverage                    # New target name
#     EXECUTABLE ctest -j ${PROCESSOR_COUNT} # Executable in PROJECT_BINARY_DIR
#     DEPENDENCIES executable_target         # Dependencies to build first
# )
function(SETUP_TARGET_FOR_COVERAGE_LLVM)

    set(options NONE)
    set(oneValueArgs NAME SOURCES_ROOT PROF_FILE)
    set(multiValueArgs EXECUTABLE BINARY EXECUTABLE_ARGS DEPENDENCIES)
    cmake_parse_arguments(Coverage "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(XCRUN_PATH)
        set(LLVM_COV_PATH ${XCRUN_PATH} llvm-cov)
        set(LLVM_PROFDATA_PATH ${XCRUN_PATH} llvm-profdata)
    else()
        if(NOT LLVM_COV_PATH)
            message(FATAL_ERROR "llvm-cov not found! Aborting...")
        endif() # NOT LLVM_COV_PATH
        if(NOT LLVM_PROFDATA_PATH)
            message(FATAL_ERROR "llvm-profdata not found! Aborting...")
        endif() # NOT LLVM_PROFDATA_PATH
    endif() # XCRUN_PATH

    set(LLVM_PROFILE_DIR ${PROJECT_BINARY_DIR}/llvm_profile)
    file(REMOVE_RECURSE ${LLVM_PROFILE_DIR})

    set(COV_EXCLUDES "")
    foreach(EXCLUDE ${COVERAGE_EXCLUDES})
        list(APPEND COV_EXCLUDES "-ignore-filename-regex=${EXCLUDE}")
    endforeach()

    list(GET Coverage_BINARY 0 COV_BINARY)
    if(Coverage_BINARY)
        list(REMOVE_AT Coverage_BINARY 0)
        foreach(BIN ${Coverage_BINARY})
            list(APPEND COV_BINARY -object ${BIN})
        endforeach()
    endif()

    add_custom_target(${Coverage_NAME}
        COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${LLVM_PROFILE_DIR}/profile-%p.profraw ctest -C $<CONFIG> $$ARGS

        COMMAND ${LLVM_PROFDATA_PATH} merge -sparse ${LLVM_PROFILE_DIR}/* -o coverage.profdata
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${Coverage_DEPENDENCIES})

    if("html" IN_LIST CMAKE_COVERAGE_FORMAT)
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ${LLVM_COV_PATH} show -instr-profile=coverage.profdata ${COV_BINARY}
                --format=html --output-dir=${Coverage_NAME}-html ${COV_EXCLUDES} ${Coverage_SOURCES_ROOT}
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Running llvm-cov to produce HTML code coverage report ${Coverage_NAME}-html")
    endif()

    if("xml" IN_LIST CMAKE_COVERAGE_FORMAT)
        message(WARNING "XML coverage report format not supported for llvm-cov")
    endif()

    if("txt" IN_LIST CMAKE_COVERAGE_FORMAT)
        add_custom_command(TARGET ${Coverage_NAME} POST_BUILD
            COMMAND ${LLVM_COV_PATH} show -instr-profile=coverage.profdata ${COV_BINARY}
                --format=text ${COV_EXCLUDES} ${Coverage_SOURCES_ROOT} > ${Coverage_NAME}.txt

            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            COMMENT "Running llvm-cov to produce TXT code coverage report ${Coverage_NAME}.txt.")
    endif()

endfunction() # SETUP_TARGET_FOR_COVERAGE_LLVM


function(APPEND_COVERAGE_COMPILER_FLAGS)
    message(STATUS "Appending code coverage compiler flags: ${COVERAGE_COMPILER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COVERAGE_COMPILER_FLAGS}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COVERAGE_COMPILER_FLAGS}" PARENT_SCOPE)
endfunction() # APPEND_COVERAGE_COMPILER_FLAGS
