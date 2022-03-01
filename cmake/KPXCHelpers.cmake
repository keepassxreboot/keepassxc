function(kpxc_run_macdeployqt_on_installed_helper_binary app_name installed_binary_relative_path)
    # Running macdeployqt on a POST_BUILD copied binary is not useful for CPack, because
    # the copied binary will be overriden by the corresponding install(TARGETS) command of the same
    # binary.
    # Thus, we run macdeployqt using install(CODE) on the already installed binary.
    # The precondition is that install(TARGETS) has to be called before this function is called.
    set(escaped_prefix "\${CMAKE_INSTALL_PREFIX}")
    set(app_bundle_name "${app_name}.app")
    set(app_bundle_path "${escaped_prefix}/${app_bundle_name}")
    set(installed_binary_path "${escaped_prefix}/${installed_binary_relative_path}")

    if(CMAKE_VERSION VERSION_GREATER "3.14")
        set(command_echo "COMMAND_ECHO STDOUT")
    else()
        set(command_echo "")
    endif()

    install(CODE
    "
execute_process(
    COMMAND
        ${MACDEPLOYQT_EXE}
        ${app_bundle_path}
        -executable=${installed_binary_path} -no-plugins 2> /dev/null
    ${command_echo}
    RESULT_VARIABLE exit_code
)
if(NOT exit_code EQUAL 0)
    message(FATAL_ERROR
        \"Running macdeployqt on ${installed_binary_path} failed with exit code \${exit_code}.\")
endif()
")
endfunction()
