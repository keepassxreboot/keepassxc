PROJECT(qjson)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII" )

# Force cmake 2.8.8 in order to have a decent support of Qt5
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.8)
CMAKE_POLICY(SET CMP0003 NEW)

# Do not link against qtmain on Windows
cmake_policy(SET CMP0020 OLD)

set(CMAKE_INSTALL_NAME_DIR ${LIB_INSTALL_DIR})

IF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")
  ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")

# Ability to disable verbose debug output
IF(QJSON_VERBOSE_DEBUG_OUTPUT)
  ADD_DEFINITIONS("-DQJSON_VERBOSE_DEBUG_OUTPUT")
endif(QJSON_VERBOSE_DEBUG_OUTPUT)

# On Windows debug library should have 'd' postfix.
IF (WIN32)
  SET(CMAKE_DEBUG_POSTFIX "d")
ENDIF (WIN32)

OPTION(OSX_FRAMEWORK "Build a Mac OS X Framework")
SET(FRAMEWORK_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/Library/Frameworks"
    CACHE PATH "Where to place qjson.framework if OSX_FRAMEWORK is selected")

# Don't use absolute path in qjson-targets-*.cmake
# (This will have no effect with CMake < 2.8)
SET(QT_USE_IMPORTED_TARGETS TRUE)

option(QT4_BUILD "Force building with Qt4 even if Qt5 is found")
IF (NOT QT4_BUILD)
  FIND_PACKAGE( Qt5Core QUIET )
ENDIF()

IF (Qt5Core_FOUND)
  MESSAGE ("Qt5 found")

  INCLUDE_DIRECTORIES(${Qt5Core_INCLUDE_DIRS})
  ADD_DEFINITIONS(${Qt5Core_DEFINITIONS})

  # Tell CMake to run moc when necessary:
  set(CMAKE_AUTOMOC ON)
  # As moc files are generated in the binary dir, tell CMake
  # to always look for includes there:
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
ELSE()
  MESSAGE ("Qt5 not found, searching for Qt4")
  # Find Qt4
  FIND_PACKAGE( Qt4 REQUIRED )

  # Include the cmake file needed to use qt4
  INCLUDE( ${QT_USE_FILE} )

ENDIF()

IF (NOT WIN32)
  SET( QT_DONT_USE_QTGUI TRUE )
ENDIF()


#add extra search paths for libraries and includes
SET (LIB_SUFFIX "" CACHE STRING "Define suffix of directory name (32/64)" )
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}" CACHE STRING "Directory where lib will install")
SET (INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The directory the headers are installed in")
SET (CMAKECONFIG_INSTALL_DIR "${LIB_INSTALL_DIR}/cmake/${CMAKE_PROJECT_NAME}" CACHE PATH "Directory where to install QJSONConfig.cmake")

set(QJSON_LIB_MAJOR_VERSION "0")
set(QJSON_LIB_MINOR_VERSION "8")
set(QJSON_LIB_PATCH_VERSION "1")

set(QJSON_LIB_VERSION_STRING "${QJSON_LIB_MAJOR_VERSION}.${QJSON_LIB_MINOR_VERSION}.${QJSON_LIB_PATCH_VERSION}")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib" )

# pkg-config
IF (NOT WIN32)
  CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/QJson.pc.in
                  ${CMAKE_CURRENT_BINARY_DIR}/QJson.pc
                  @ONLY)
  INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/QJson.pc
           DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)
ENDIF (NOT WIN32)

# Subdirs
ADD_SUBDIRECTORY(src)
IF (KDE4_BUILD_TESTS OR QJSON_BUILD_TESTS)
  enable_testing()
  ADD_SUBDIRECTORY(tests)
ENDIF (KDE4_BUILD_TESTS OR QJSON_BUILD_TESTS)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

INSTALL(EXPORT qjson-export DESTINATION ${CMAKECONFIG_INSTALL_DIR} FILE QJSONTargets.cmake)

# figure out the relative path from the installed Config.cmake file to the install prefix (which may be at
# runtime different from the chosen CMAKE_INSTALL_PREFIX if under Windows the package was installed anywhere)
# This relative path will be configured into the QJSONConfig.cmake
file(RELATIVE_PATH relInstallDir ${CMAKE_INSTALL_PREFIX}/${CMAKECONFIG_INSTALL_DIR} ${CMAKE_INSTALL_PREFIX} )

# cmake-modules
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/QJSONConfig.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/QJSONConfig.cmake
               @ONLY)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/QJSONConfigVersion.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/QJSONConfigVersion.cmake
               @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/QJSONConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/QJSONConfigVersion.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}")

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
