# Running macdeployqt on a POST_BUILD copied binaries is pointless when using CPack because
# the copied binaries will be overridden by the corresponding install(TARGETS) commands.
# That's why we run macdeployqt using install(CODE) on the already installed binaries.
# The precondition is that all install(TARGETS) calls have to be called before this function is
# called.
# macdeloyqt is called only once, but it is given all executables that should be processed.
function(kpxc_run_macdeployqt_at_install_time)
    set(NO_VALUE_OPTIONS)
    set(SINGLE_VALUE_OPTIONS
        APP_NAME
    )
    set(MULTI_VALUE_OPTIONS
        EXTRA_BINARIES
    )
    cmake_parse_arguments(PARSE_ARGV 0 ARG
        "${NO_VALUE_OPTIONS}" "${SINGLE_VALUE_OPTIONS}" "${MULTI_VALUE_OPTIONS}"
    )

    set(ESCAPED_PREFIX "\${CMAKE_INSTALL_PREFIX}")
    set(APP_BUNDLE_NAME "${ARG_APP_NAME}.app")
    set(APP_BUNDLE_PATH "${ESCAPED_PREFIX}/${APP_BUNDLE_NAME}")

    # Collect extra binaries and plugins that should be handled by macdpeloyqt.
    set(EXTRA_BINARIES "")
    foreach(EXTRA_BINARY ${ARG_EXTRA_BINARIES})
        set(INSTALLED_BINARY_PATH "${ESCAPED_PREFIX}/${EXTRA_BINARY}")
        list(APPEND EXTRA_BINARIES "-executable=${INSTALLED_BINARY_PATH}")
    endforeach()

    list(JOIN EXTRA_BINARIES " " EXTRA_BINARIES_STR)

    if(CMAKE_VERSION VERSION_GREATER "3.14")
        set(COMMAND_ECHO "COMMAND_ECHO STDOUT")
    else()
        set(COMMAND_ECHO "")
    endif()

    set(COMMAND_ARGS
        ${MACDEPLOYQT_EXE}
        ${APP_BUNDLE_PATH}

        # Adjusts dependency rpaths of extra binaries
        ${EXTRA_BINARIES_STR}

        # Silences warnings on subsequent re-installations
        -always-overwrite
    )

    install(CODE
    "
execute_process(
    COMMAND ${COMMAND_ARGS}
    ${COMMAND_ECHO}
    RESULT_VARIABLE EXIT_CODE
)
if(NOT EXIT_CODE EQUAL 0)
    message(FATAL_ERROR
        \"Running ${COMMAND_ARGS} failed with exit code \${EXIT_CODE}.\")
endif()
")
endfunction()
