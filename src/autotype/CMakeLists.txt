if(WITH_XC_AUTOTYPE)
    if(UNIX AND NOT APPLE AND NOT HAIKU)
        find_package(X11)
        find_package(Qt5X11Extras 5.2)
        if(PRINT_SUMMARY)
            add_feature_info(libXi X11_Xi_FOUND "The X11 Xi Protocol library is required for auto-type")
            add_feature_info(libXtst X11_XTest_FOUND "The X11 XTEST Protocol library is required for auto-type")
            add_feature_info(Qt5X11Extras Qt5X11Extras_FOUND "The Qt5X11Extras library is required for auto-type")
        endif()

        # an error will be emitted by find_package above if X11 (or Qt5X11Extras) dev files were not found.
        # X11 possibly is installed and found while X11_Xi and/or X11_XTest are not, so we emit a warning.
        if(NOT X11_Xi_FOUND)
            message(WARNING "The X11 Xi Protocol library is required for auto-type (try libxi-dev package)")
        endif()

        if(NOT X11_XTest_FOUND)
            message(WARNING "The X11 XTEST Protocol library is required for auto-type (try libxtst-dev package)")
        endif()

        if(X11_FOUND AND X11_Xi_FOUND AND X11_XTest_FOUND AND Qt5X11Extras_FOUND)
            add_subdirectory(xcb)
        endif()
    elseif(APPLE)
        add_subdirectory(mac)
    elseif(WIN32)
        add_subdirectory(windows)
    endif()

    if(WITH_TESTS)
        add_subdirectory(test)
    endif()
endif()
