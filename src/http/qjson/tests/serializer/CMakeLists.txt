##### Probably don't want to edit below this line #####

SET( QT_USE_QTTEST TRUE )

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

SET( UNIT_TESTS
  testserializer
)

# Build the tests
FOREACH(test ${UNIT_TESTS})
  MESSAGE(STATUS "Building ${test}")
  IF (NOT Qt5Core_FOUND)
    QT4_WRAP_CPP(MOC_SOURCE ${test}.cpp)
  ENDIF()
  ADD_EXECUTABLE(
    ${test}
    ${test}.cpp
  )

  ADD_FILE_DEPENDENCIES(${test}.cpp ${MOC_SOURCE})
  TARGET_LINK_LIBRARIES(
    ${test}
    ${QT_LIBRARIES}
    ${TEST_LIBRARIES}
    qjson
  )
  if (QJSON_TEST_OUTPUT STREQUAL "xml")
    # produce XML output
    add_test( ${test} ${test} -xml -o ${test}.tml )
  else (QJSON_TEST_OUTPUT STREQUAL "xml")
    add_test( ${test} ${test} )
  endif (QJSON_TEST_OUTPUT STREQUAL "xml")
ENDFOREACH()
