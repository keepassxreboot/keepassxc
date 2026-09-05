include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(autotype_SOURCES
        AutoType.cpp
        AutoTypeAction.cpp
        AutoTypeMatchModel.cpp
        AutoTypeMatchView.cpp
        AutoTypeSelectDialog.cpp
        PickcharsDialog.cpp
        WindowSelectComboBox.cpp
        test/AutoTypeTest.cpp)

# Platform specific auto-type implementations
if(UNIX AND NOT APPLE AND NOT HAIKU)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Xkbcommon REQUIRED IMPORTED_TARGET xkbcommon)

    list(APPEND autotype_SOURCES
            wayland/AutoTypeWayland.cpp)

    if(WITH_X11)
        find_package(X11 REQUIRED COMPONENTS Xi XTest)
        if(PRINT_SUMMARY)
            add_feature_info(libXi X11_Xi_FOUND "The X11 Xi Protocol library is required for auto-type")
            add_feature_info(libXtst X11_XTest_FOUND "The X11 XTEST Protocol library is required for auto-type")
            add_feature_info(Qt6X11Extras Qt6X11Extras_FOUND "The Qt6X11Extras library is required for auto-type")
        endif()

        list(APPEND autotype_SOURCES
                xcb/AutoTypeXCB.cpp)
    endif()
elseif(APPLE)
    list(APPEND autotype_SOURCES
            mac/AutoTypeMac.cpp)
elseif(WIN32)
    list(APPEND autotype_SOURCES
            windows/AutoTypeWindows.cpp)
    if(KPXC_FEATURE_UIACCESS_HELPER)
        list(APPEND autotype_SOURCES
                windows/UiAccessInjector.cpp)
    endif()
endif()

add_library(autotype STATIC ${autotype_SOURCES})
target_link_libraries(autotype Qt6::Core Qt6::Widgets)

if(UNIX AND NOT APPLE AND NOT HAIKU)
    target_link_libraries(autotype Qt6::DBus PkgConfig::Xkbcommon)
    if(WITH_X11)
        target_link_libraries(autotype ${X11_X11_LIB} ${X11_Xi_LIB} ${X11_XTest_LIB})
    endif()
elseif(APPLE)
    target_link_libraries(autotype "-framework Foundation" "-framework AppKit" "-framework Carbon" "-framework ScreenCaptureKit")
elseif(WIN32)
    # shell32: ShellExecuteEx, the only way to start a uiAccess binary.
    target_link_libraries(autotype shell32 advapi32)
    if(MINGW)
        # FOLDERID_* GUIDs used by UiAccessInjector live in libuuid on MinGW.
        target_link_libraries(autotype uuid)
    endif()
endif()
