##### Probably don't want to edit below this line #####

SET( QT_USE_QTTEST TRUE )

INCLUDE(AddFileDependencies)

# Include the library include directories, and the current build directory (moc)
INCLUDE_DIRECTORIES(
  ../../include
  ${CMAKE_CURRENT_BINARY_DIR}
)

SET( UNIT_TESTS
  parsingbenchmark
  qlocalevsstrtod_l
)

# Build the tests
FOREACH(test ${UNIT_TESTS})
  MESSAGE(STATUS "Building ${test}")
  ADD_EXECUTABLE(
    ${test}
    ${test}.cpp
  )

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
