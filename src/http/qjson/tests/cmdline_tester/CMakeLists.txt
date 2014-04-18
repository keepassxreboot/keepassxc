##### Probably don't want to edit below this line #####

IF (WIN32 AND Qt5Core_FOUND)
  FIND_PACKAGE( Qt5Widgets REQUIRED )

  INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS})
  ADD_DEFINITIONS(${Qt5Widgets_DEFINITIONS})
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
ENDIF()

IF (NOT Qt5Core_FOUND)
  # Use it
  INCLUDE( ${QT_USE_FILE} )
ENDIF()

INCLUDE(AddFileDependencies)

# Include the library include directories, and the current build directory (moc)
INCLUDE_DIRECTORIES(
  ../../include
  ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_EXECUTABLE(
  cmdline_tester
  cmdline_tester.cpp
  cmdlineparser.cpp
)

TARGET_LINK_LIBRARIES(
  cmdline_tester
  ${QT_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  qjson
)
