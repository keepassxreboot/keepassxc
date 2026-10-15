# The install commands in this subdirectory will be executed after all the install commands in the
# current scope are ran. It is required for correct functioning of macdeployqt.

if(APPLE AND WITH_APP_BUNDLE)
    install(CODE "file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}/${PLUGIN_INSTALL_DIR}/keepassxc-autofill.appex\")")

    # Run macdeloyqt on the main app and any extra binaries and plugins as specified by the
    # _MACDEPLOYQT_EXTRA_BINARIES global property.
    # All install(TARGETS) calls should have already been called.
    get_property(MACDEPLOYQT_EXTRA_BINARIES GLOBAL PROPERTY _MACDEPLOYQT_EXTRA_BINARIES)
    kpxc_run_macdeployqt_at_install_time(
        APP_NAME "${PROGNAME}"
        EXTRA_BINARIES ${MACDEPLOYQT_EXTRA_BINARIES}
    )

    install(DIRECTORY "$<TARGET_BUNDLE_DIR:keepassxc-autofill>"
            DESTINATION "${PLUGIN_INSTALL_DIR}"
            USE_SOURCE_PERMISSIONS)
endif()
