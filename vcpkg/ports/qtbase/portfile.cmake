# Reminder for myself and everybody else:
# Qt cross module dependency information within the Qt respository is wrong and/or incomplete.
# Always check the toplevel CMakeLists.txt for the find_package call and search for linkage against the Qt:: targets
# Often enough certain (bigger) dependencies are only used to build examples and/or tests.
# As such getting the correct dependency information relevant for vcpkg requires a manual search/check
set(QT_IS_LATEST ON)

## All above goes into the qt_port_hashes in the future
include("${CMAKE_CURRENT_LIST_DIR}/cmake/qt_install_submodule.cmake")

set(${PORT}_PATCHES
        allow_outside_prefix.patch
        config_install.patch
        fix_cmake_build.patch
        harfbuzz.patch
        fix_egl.patch
        fix_egl_2.patch
        installed_dir.patch
        GLIB2-static.patch # alternative is to force pkg-config
        clang-cl_source_location.patch
        clang-cl_QGADGET_fix.diff
        fix-host-aliasing.patch
        fix_deploy_windows.patch
        windeployqt-webengine-debug-resources.patch
        fix-link-lib-discovery.patch
        macdeployqt-symlinks.patch
        moltenvk.patch
        fix-ioring-32bit.patch
        fix-liburing-config-test.patch
        fix-libresolv-test.patch
        use_inotify_on_freebsd.patch
        silence-winrtbase-coroutine-warnings.diff
        md4c.diff
        QTBUG-147449.patch # https://github.com/qt/qtbase/commit/65020b43cdd2c923c42ac4bc894c967ac90a0477
)

if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    list(APPEND ${PORT}_PATCHES env.patch)
endif()

if("shared-mime-info" IN_LIST FEATURES)
    list(APPEND ${PORT}_PATCHES use-shared-mime-info.patch)
endif()

list(APPEND ${PORT}_PATCHES
        dont_force_cmakecache_latest.patch
    )

if(VCPKG_TARGET_IS_WINDOWS AND NOT "doubleconversion" IN_LIST FEATURES)
    message(FATAL_ERROR "${PORT} requires feature doubleconversion on windows!" )
endif()

# Feature like glib depend on pkg-config
vcpkg_find_acquire_program(PKGCONFIG)
set(ENV{PKG_CONFIG} "${PKGCONFIG}")

if(VCPKG_TARGET_IS_LINUX)
    message(WARNING "qtbase currently requires packages from the system package manager. "
    "They can be installed on Ubuntu systems via sudo apt-get install "
    "'^libxcb.*-dev' libx11-xcb-dev libglu1-mesa-dev libxrender-dev libxi-dev libxkbcommon-dev "
    "libxkbcommon-x11-dev libegl1-mesa-dev.")
endif()

# Features can be found via searching for qt_feature in all configure.cmake files in the source:
# The files also contain information about the Platform for which it is searched
# Always use FEATURE_<feature> in vcpkg_cmake_configure
# (using QT_FEATURE_X overrides Qts condition check for the feature.)
# Theoretically there is a feature for every widget to enable/disable it but that is way to much for vcpkg

set(input_vars doubleconversion freetype harfbuzz libb2 jpeg md4c png sql-sqlite)
set(INPUT_OPTIONS "")
foreach(_input IN LISTS input_vars)
    if(_input MATCHES "(png|jpeg|md4c)" )
        list(APPEND INPUT_OPTIONS -DINPUT_lib${_input}:STRING=)
    elseif(_input MATCHES "(sql-sqlite)") # Not yet used by the cmake build
    else()
        list(APPEND INPUT_OPTIONS -DINPUT_${_input}:STRING=)
    endif()
    if("${_input}" IN_LIST FEATURES)
        string(APPEND INPUT_OPTIONS system)
    elseif(_input STREQUAL "libb2" AND NOT VCPKG_TARGET_IS_WINDOWS)
        string(APPEND INPUT_OPTIONS system)
    elseif(_input STREQUAL "md4c") # required by qtdeclarative
        string(APPEND INPUT_OPTIONS system)
    else()
        string(APPEND INPUT_OPTIONS no)
    endif()
endforeach()

# General features:

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
FEATURES
    "appstore-compliant"  FEATURE_appstore_compliant
    "zstd"                FEATURE_zstd
    "framework"           FEATURE_framework
    "concurrent"          FEATURE_concurrent
    "future"              FEATURE_future
    "dbus"                FEATURE_dbus
    "gui"                 FEATURE_gui
    "thread"              FEATURE_thread
    "network"             FEATURE_network
    "sql"                 FEATURE_sql
    "widgets"             FEATURE_widgets
    "windeployqt"         FEATURE_windeployqt
    #"xml"                 FEATURE_xml  # Required to build moc
    "testlib"             FEATURE_testlib
    "zstd"                CMAKE_REQUIRE_FIND_PACKAGE_zstd
    ${require_features}
INVERTED_FEATURES
    "zstd"              CMAKE_DISABLE_FIND_PACKAGE_ZSTD
    "dbus"              CMAKE_DISABLE_FIND_PACKAGE_WrapDBus1
    )

list(APPEND FEATURE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Libudev:BOOL=ON)
list(APPEND FEATURE_OPTIONS -DFEATURE_xml:BOOL=ON)

if("dbus" IN_LIST FEATURES AND VCPKG_TARGET_IS_LINUX)
  list(APPEND FEATURE_OPTIONS -DINPUT_dbus=linked)
elseif("dbus" IN_LIST FEATURES)
  list(APPEND FEATURE_OPTIONS -DINPUT_dbus=runtime)
else()
  list(APPEND FEATURE_OPTIONS -DINPUT_dbus=no)
endif()

if(VCPKG_QT_NAMESPACE)
    list(APPEND FEATURE_OPTIONS "-DQT_NAMESPACE:STRING=${VCPKG_QT_NAMESPACE}")
endif()

# Corelib features:
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_CORE_OPTIONS
FEATURES
    "doubleconversion"    FEATURE_doubleconversion
    "glib"                FEATURE_glib
    "icu"                 FEATURE_icu
    "pcre2"               FEATURE_pcre2
    "async-io"            FEATURE_async_io
    #"icu"                 CMAKE_REQUIRE_FIND_PACKAGE_ICU
    #"glib"                CMAKE_REQUIRE_FIND_PACKAGE_GLIB2
INVERTED_FEATURES
    #"doubleconversion"      CMAKE_DISABLE_FIND_PACKAGE_WrapDoubleConversion # Required
    #"pcre2"                 CMAKE_DISABLE_FIND_PACKAGE_WrapSystemPCRE2 # Bug in qt cannot be deactivated
    "icu"                  CMAKE_DISABLE_FIND_PACKAGE_ICU
    "glib"                 CMAKE_DISABLE_FIND_PACKAGE_GLIB2
    )

if(VCPKG_TARGET_IS_LINUX)
    vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OS_CORE_OPTIONS
        FEATURES
            "ioring" FEATURE_liburing
    )
elseif(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OS_CORE_OPTIONS
        FEATURES
            "ioring" FEATURE_windows_ioring
    )
endif()

list(APPEND FEATURE_CORE_OPTIONS ${FEATURE_OS_CORE_OPTIONS})

list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_LTTngUST:BOOL=ON)
list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_PPS:BOOL=ON)
list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Slog2:BOOL=ON)
list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Libsystemd:BOOL=ON)
list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_WrapBacktrace:BOOL=ON)
list(APPEND FEATURE_CORE_OPTIONS -DFEATURE_pkg_config:BOOL=ON)
#list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_WrapAtomic:BOOL=ON) # Cannot be disabled on x64 platforms
#list(APPEND FEATURE_CORE_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_WrapRt:BOOL=ON) # Cannot be disabled on osx

# Network features:
 vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_NET_OPTIONS
 FEATURES
    "openssl"             FEATURE_openssl
    "brotli"              FEATURE_brotli
    "securetransport"     FEATURE_securetransport
    "dnslookup"           FEATURE_dnslookup
    #"brotli"              CMAKE_REQUIRE_FIND_PACKAGE_WrapBrotli
    #"openssl"             CMAKE_REQUIRE_FIND_PACKAGE_WrapOpenSSL
 INVERTED_FEATURES
    "brotli"              CMAKE_DISABLE_FIND_PACKAGE_WrapBrotli
    "openssl"             CMAKE_DISABLE_FIND_PACKAGE_WrapOpenSSL
    "dnslookup"           CMAKE_DISABLE_FIND_PACKAGE_WrapResolve
    )

if("openssl" IN_LIST FEATURES)
    list(APPEND FEATURE_NET_OPTIONS -DINPUT_openssl=linked)
else()
    list(APPEND FEATURE_NET_OPTIONS -DINPUT_openssl=no)
endif()

if ("dnslookup" IN_LIST FEATURES AND NOT VCPKG_TARGET_IS_ANDROID AND NOT VCPKG_TARGET_IS_WINDOWS)
    list(APPEND FEATURE_NET_OPTIONS -DFEATURE_libresolv:BOOL=ON)
endif()

list(APPEND FEATURE_NET_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Libproxy:BOOL=ON)
list(APPEND FEATURE_NET_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_GSSAPI:BOOL=ON)

# Gui features:
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_GUI_OPTIONS
    FEATURES
    "freetype"            FEATURE_freetype # required on windows
    "harfbuzz"            FEATURE_harfbuzz
    "fontconfig"          FEATURE_fontconfig # NOT WINDOWS
    "jpeg"                FEATURE_jpeg
    "png"                 FEATURE_png
    "opengl"              FEATURE_opengl
    "sessionmanager"      FEATURE_sessionmanager
    "xlib"                FEATURE_xlib
    "xkb"                 FEATURE_xkbcommon
    "xcb"                 FEATURE_xcb
    "xcb-sm"              FEATURE_xcb_sm
    "xcb-xlib"            FEATURE_xcb_xlib
    "xkbcommon-x11"       FEATURE_xkbcommon_x11
    "xrender"             FEATURE_xrender # requires FEATURE_xcb_native_painting; otherwise disabled.
    "xrender"             FEATURE_xcb_native_painting # experimental
    "gles2"               FEATURE_opengles2
    "gles3"               FEATURE_opengles3
    #Cannot be required since Qt will look in CONFIG mode first but is controlled via CMAKE_DISABLE_FIND_PACKAGE_Vulkan below
    #"vulkan"              CMAKE_REQUIRE_FIND_PACKAGE_WrapVulkanHeaders
    "egl"                 FEATURE_egl
    #"fontconfig"          CMAKE_REQUIRE_FIND_PACKAGE_Fontconfig
    #"harfbuzz"            CMAKE_REQUIRE_FIND_PACKAGE_WrapSystemHarfbuzz
    #"jpeg"                CMAKE_REQUIRE_FIND_PACKAGE_JPEG
    #"png"                 CMAKE_REQUIRE_FIND_PACKAGE_PNG
    "wayland"             FEATURE_wayland
    #"xlib"                CMAKE_REQUIRE_FIND_PACKAGE_X11
    #"xkb"                 CMAKE_REQUIRE_FIND_PACKAGE_XKB
    #"xcb"                 CMAKE_REQUIRE_FIND_PACKAGE_XCB
    #"xcb-xlib"            CMAKE_REQUIRE_FIND_PACKAGE_X11_XCB
    #"xkbcommon-x11"       CMAKE_REQUIRE_FIND_PACKAGE_XKB_COMMON_X11
    #"xrender"             CMAKE_REQUIRE_FIND_PACKAGE_XRender
    INVERTED_FEATURES
    "vulkan"              CMAKE_DISABLE_FIND_PACKAGE_Vulkan
    "opengl"              CMAKE_DISABLE_FIND_PACKAGE_WrapOpenGL
    "egl"                 CMAKE_DISABLE_FIND_PACKAGE_EGL
    "gles2"               CMAKE_DISABLE_FIND_PACKAGE_GLESv2
    "gles3"               CMAKE_DISABLE_FIND_PACKAGE_GLESv3
    "fontconfig"          CMAKE_DISABLE_FIND_PACKAGE_Fontconfig
    #"freetype"            CMAKE_DISABLE_FIND_PACKAGE_WrapSystemFreetype # Bug in qt cannot be deactivated
    "harfbuzz"            CMAKE_DISABLE_FIND_PACKAGE_WrapSystemHarfbuzz
    "jpeg"                CMAKE_DISABLE_FIND_PACKAGE_JPEG
    #"png"                 CMAKE_DISABLE_FIND_PACKAGE_PNG # Unable to disable if Freetype requires it
    "wayland"             CMAKE_DISABLE_FIND_PACKAGE_Wayland
    "xlib"                CMAKE_DISABLE_FIND_PACKAGE_X11
    "xkb"                 CMAKE_DISABLE_FIND_PACKAGE_XKB
    "xcb"                 CMAKE_DISABLE_FIND_PACKAGE_XCB
    "xcb-xlib"            CMAKE_DISABLE_FIND_PACKAGE_X11_XCB
    "xkbcommon-x11"       CMAKE_DISABLE_FIND_PACKAGE_XKB_COMMON_X11
    "xrender"             CMAKE_DISABLE_FIND_PACKAGE_XRender
    # There are more X features but I am unsure how to safely disable them! Most of them seem to be found automaticall with find_package(X11)
     )

if("gles2" IN_LIST FEATURES)
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_opengl='es2')
    list(APPEND FEATURE_GUI_OPTIONS -DFEATURE_opengl_desktop=OFF)
endif()

if(NOT "opengl" IN_LIST FEATURES AND NOT "gles2" IN_LIST FEATURES)
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_opengl='no')
    list(APPEND FEATURE_GUI_OPTIONS -DFEATURE_opengl_desktop=OFF)
    list(APPEND FEATURE_GUI_OPTIONS -DFEATURE_opengl_dynamic=OFF)
endif()

if("xcb" IN_LIST FEATURES)
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_xcb=yes)
else()
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_xcb=no)
endif()
if("xkb" IN_LIST FEATURES)
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_xkbcommon=yes)
else()
    list(APPEND FEATURE_GUI_OPTIONS -DINPUT_xkbcommon=no)
endif()

# Disable OpenGL ES 3.1 and 3.2
list(APPEND FEATURE_GUI_OPTIONS -DFEATURE_opengles31:BOOL=OFF)
list(APPEND FEATURE_GUI_OPTIONS -DFEATURE_opengles32:BOOL=OFF)

list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_ATSPI2:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_DirectFB:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Libdrm:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_gbm:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Libinput:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Mtdev:BOOL=ON)
list(APPEND FEATURE_GUI_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_Tslib:BOOL=ON)
# sql-drivers features:

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_SQLDRIVERS_OPTIONS
    FEATURES
    "sql-sqlite"          FEATURE_system_sqlite
    "sql-odbc"            FEATURE_sql_odbc
    "sql-mysql"           FEATURE_sql_mysql
    "sql-oci"             FEATURE_sql_oci
    #"sql-psql"            CMAKE_REQUIRE_FIND_PACKAGE_PostgreSQL
    #"sql-sqlite"          CMAKE_REQUIRE_FIND_PACKAGE_SQLite3
    INVERTED_FEATURES
    "sql-psql"            CMAKE_DISABLE_FIND_PACKAGE_PostgreSQL
    "sql-sqlite"          CMAKE_DISABLE_FIND_PACKAGE_SQLite3
    "sql-odbc"            CMAKE_DISABLE_FIND_PACKAGE_ODBC
    "sql-mysql"           CMAKE_DISABLE_FIND_PACKAGE_MySQL
    "sql-oci"             CMAKE_DISABLE_FIND_PACKAGE_Oracle
    )

set(DB_LIST DB2 Interbase Mimer)
foreach(_db IN LISTS DB_LIST)
    list(APPEND FEATURE_SQLDRIVERS_OPTIONS -DCMAKE_DISABLE_FIND_PACKAGE_${_db}:BOOL=ON)
endforeach()

# printsupport features:
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_PRINTSUPPORT_OPTIONS
  FEATURES
  "cups" FEATURE_cups
  INVERTED_FEATURES
  "cups" CMAKE_DISABLE_FIND_PACKAGE_Cups
)


vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_WIDGETS_OPTIONS
    FEATURES
    "gtk3"              FEATURE_gtk3
    INVERTED_FEATURES
    "gtk3"              CMAKE_DISABLE_FIND_PACKAGE_GTK3
)

set(TOOL_NAMES
        androiddeployqt
        androidtestrunner
        cmake_automoc_parser
        moc
        qdbuscpp2xml
        qdbusxml2cpp
        qlalr
        qmake
        qmake6
        qvkgen
        rcc
        tracegen
        uic
        qtpaths
        qtpaths6
        windeployqt
        windeployqt6
        macdeployqt
        macdeployqt6
        androiddeployqt6
        syncqt
        tracepointgen
        qtwaylandscanner
        wasmdeployqt
        wasmdeployqt6
    )

qt_install_submodule(PATCHES    ${${PORT}_PATCHES}
                     TOOL_NAMES ${TOOL_NAMES}
                     CONFIGURE_OPTIONS
                        ${FEATURE_OPTIONS}
                        ${FEATURE_CORE_OPTIONS}
                        ${FEATURE_NET_OPTIONS}
                        ${FEATURE_GUI_OPTIONS}
                        ${FEATURE_SQLDRIVERS_OPTIONS}
                        ${FEATURE_PRINTSUPPORT_OPTIONS}
                        ${FEATURE_WIDGETS_OPTIONS}
                        ${INPUT_OPTIONS}
                        -DQT_USE_BUNDLED_BundledFreetype:BOOL=FALSE
                        -DQT_USE_BUNDLED_BundledHarfbuzz:BOOL=FALSE
                        -DQT_USE_BUNDLED_BundledLibpng:BOOL=FALSE
                        -DQT_USE_BUNDLED_BundledPcre2:BOOL=FALSE
                        -DINPUT_bundled_xcb_xinput:STRING=no
                        -DFEATURE_force_debug_info:BOOL=ON
                        -DFEATURE_relocatable:BOOL=ON
                        -DQT_AUTODETECT_ANDROID:BOOL=ON # Use vcpkg toolchain as is
                        -DQT_NO_XCODE_MIN_VERSION_CHECK:BOOL=ON # The cmd line tools are missing xcodebuild
                        -DQT_FIND_APPLE_SYSTEM_FRAMEWORKS_MODE:STRING=ONLY
                     CONFIGURE_OPTIONS_RELEASE
                     CONFIGURE_OPTIONS_DEBUG
                        -DFEATURE_debug:BOOL=ON
                     CONFIGURE_OPTIONS_MAYBE_UNUSED
                        FEATURE_appstore_compliant # only used for android/ios
                        QT_AUTODETECT_ANDROID
                        QT_NO_XCODE_MIN_VERSION_CHECK
                        QT_FIND_APPLE_SYSTEM_FRAMEWORKS_MODE
                    )

# Install CMake helper scripts
file(COPY
    "${CMAKE_CURRENT_LIST_DIR}/cmake/"
    DESTINATION
        "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    )

file(CONFIGURE OUTPUT "${CURRENT_PACKAGES_DIR}/share/${PORT}/port_status.cmake" CONTENT "set(qtbase_with_icu ${FEATURE_icu})\n")

set(other_files qt-cmake
                qt-cmake-create
                qt-cmake-private
                qt-cmake-standalone-test
                qt-configure-module
                qt-internal-configure-tests
                qt-cmake-create
                qt-internal-configure-examples
                qt-internal-configure-tests
                qmake
                qmake6
                qtpaths
                qtpaths6
)

if(CMAKE_HOST_WIN32)
    set(script_suffix ".bat")
else()
    set(script_suffix "")
endif()
list(TRANSFORM other_files APPEND "${script_suffix}")

list(APPEND other_files
                android_cmakelist_patcher.sh
                android_emulator_launcher.sh
                ensure_pro_file.cmake
                qt-android-runner.py
                qt-cmake-private-install.cmake
                qt_cyclonedx_generator.py
                qt-testrunner.py
                qt-wasmtestrunner.py
                sanitizer-testrunner.py
                syncqt.pl
                target_qt.conf
)

foreach(_config debug release)
    if(_config MATCHES "debug")
        set(path_suffix debug/)
    else()
        set(path_suffix)
    endif()
    if(NOT EXISTS "${CURRENT_PACKAGES_DIR}/${path_suffix}bin")
        continue()
    endif()
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/${path_suffix}")
    foreach(other_file IN LISTS other_files)
        if(EXISTS "${CURRENT_PACKAGES_DIR}/${path_suffix}bin/${other_file}")
            set(target_file "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/${path_suffix}${other_file}")
            file(RENAME "${CURRENT_PACKAGES_DIR}/${path_suffix}bin/${other_file}" "${target_file}")
            file(READ "${target_file}" _contents)
            if(_config MATCHES "debug")
                string(REPLACE "..\\share\\" "..\\..\\..\\..\\share\\" _contents "${_contents}")
                string(REPLACE "../share/" "../../../../share/" _contents "${_contents}")
            else()
                string(REPLACE "..\\share\\" "..\\..\\..\\share\\" _contents "${_contents}")
                string(REPLACE "../share/" "../../../share/" _contents "${_contents}")
            endif()
            string(REGEX REPLACE "set cmake_path=[^\n]+\n" "set cmake_path=cmake\n" _contents "${_contents}")
            string(REGEX REPLACE "original_cmake_path=[^\n]+\n" "original_cmake_path=does-not-exist\n" _contents "${_contents}")
            file(WRITE "${target_file}" "${_contents}")
        endif()
    endforeach()
endforeach()

# Fixup qt.toolchain.cmake
set(qttoolchain "${CURRENT_PACKAGES_DIR}/share/Qt6/qt.toolchain.cmake")
file(READ "${qttoolchain}" toolchain_contents)
string(REGEX REPLACE "set\\\(__qt_initially_configured_toolchain_file [^\\\n]+\\\n" "" toolchain_contents "${toolchain_contents}")
string(REGEX REPLACE "set\\\(__qt_chainload_toolchain_file [^\\\n]+\\\n" "set(__qt_chainload_toolchain_file \"\${VCPKG_CHAINLOAD_TOOLCHAIN_FILE}\")\n" toolchain_contents "${toolchain_contents}")
string(REGEX REPLACE "set\\\(VCPKG_CHAINLOAD_TOOLCHAIN_FILE [^\\\n]+\\\n" "" toolchain_contents "${toolchain_contents}")
string(REGEX REPLACE "set\\\(__qt_initial_c_compiler [^\\\n]+\\\n" "" toolchain_contents "${toolchain_contents}")
string(REGEX REPLACE "set\\\(__qt_initial_cxx_compiler [^\\\n]+\\\n" "" toolchain_contents "${toolchain_contents}")
string(REPLACE "${CURRENT_HOST_INSTALLED_DIR}" "\${vcpkg_installed_dir}/${HOST_TRIPLET}" toolchain_contents "${toolchain_contents}")
file(WRITE "${qttoolchain}" "${toolchain_contents}")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static" OR NOT VCPKG_TARGET_IS_WINDOWS)
    if(VCPKG_CROSSCOMPILING)
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin/qmake" "${CURRENT_PACKAGES_DIR}/debug/bin/qmake") # qmake has been moved so this is the qmake helper script
    endif()
    file(GLOB_RECURSE _bin_files "${CURRENT_PACKAGES_DIR}/bin/*")
    if(NOT _bin_files) # Only clean if empty otherwise let vcpkg throw and error.
        file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin/" "${CURRENT_PACKAGES_DIR}/debug/bin/")
    else()
        message(STATUS "Files in '/bin':${_bin_files}")
    endif()
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/Qt6/QtBuildInternals")

if(NOT VCPKG_TARGET_IS_OSX)
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/Qt6/macos")
endif()
if(NOT VCPKG_TARGET_IS_IOS)
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/Qt6/ios")
endif()
if(NOT VCPKG_TARGET_IS_WINDOWS)
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/Qt6/windows")
endif()

file(RELATIVE_PATH installed_to_host "${CURRENT_INSTALLED_DIR}" "${CURRENT_HOST_INSTALLED_DIR}")
file(RELATIVE_PATH host_to_installed "${CURRENT_HOST_INSTALLED_DIR}" "${CURRENT_INSTALLED_DIR}")
if(installed_to_host)
    string(APPEND installed_to_host "/")
    string(APPEND host_to_installed "/")
endif()
set(_file "${CMAKE_CURRENT_LIST_DIR}/qt.conf.in")
set(REL_PATH "")
set(REL_HOST_TO_DATA "\${CURRENT_INSTALLED_DIR}/")
set(LIBEXEC_DEBUG_SUFFIX "")
configure_file("${_file}" "${CURRENT_PACKAGES_DIR}/tools/Qt6/qt_release.conf" @ONLY) # For vcpkg-qmake
set(BACKUP_CURRENT_INSTALLED_DIR "${CURRENT_INSTALLED_DIR}")
set(BACKUP_CURRENT_HOST_INSTALLED_DIR "${CURRENT_HOST_INSTALLED_DIR}")
set(CURRENT_INSTALLED_DIR "./../../../")
set(CURRENT_HOST_INSTALLED_DIR "${CURRENT_INSTALLED_DIR}${installed_to_host}")

## Configure installed qt.conf
set(REL_HOST_TO_DATA "${host_to_installed}")
configure_file("${_file}" "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qt.conf")
set(REL_PATH debug/)
if(VCPKG_TARGET_IS_WINDOWS)
    set(LIBEXEC_DEBUG_SUFFIX "/debug")
endif()
configure_file("${_file}" "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qt.debug.conf")

set(CURRENT_INSTALLED_DIR "${BACKUP_CURRENT_INSTALLED_DIR}")
set(CURRENT_HOST_INSTALLED_DIR "${BACKUP_CURRENT_HOST_INSTALLED_DIR}")
set(REL_HOST_TO_DATA "\${CURRENT_INSTALLED_DIR}/")
configure_file("${_file}" "${CURRENT_PACKAGES_DIR}/tools/Qt6/qt_debug.conf" @ONLY) # For vcpkg-qmake

# target_qt_conf exists iff CMAKE_CROSSCOMPILING
# cf. qt_generate_qmake_and_qtpaths_wrapper_for_target in <src>/cmake/QtQmakeHelpers.cmake
set(target_qt_conf "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/target_qt.conf")
if(EXISTS "${target_qt_conf}")
    file(READ "${target_qt_conf}" qt_conf_contents)
    string(REGEX REPLACE "Prefix=[^\n]+" "Prefix=./../../../" qt_conf_contents ${qt_conf_contents})
    string(REGEX REPLACE "HostData=[^\n]+" "HostData=./../${TARGET_TRIPLET}/share/Qt6" qt_conf_contents ${qt_conf_contents})
    string(REGEX REPLACE "HostPrefix=[^\n]+" "HostPrefix=./../../../../${_HOST_TRIPLET}" qt_conf_contents ${qt_conf_contents})
    file(WRITE "${target_qt_conf}" "${qt_conf_contents}")

    if(script_suffix STREQUAL "")
        foreach(name IN ITEMS qmake qmake6 qtpaths qtpaths6)
            vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/${name}" "${CURRENT_HOST_INSTALLED_DIR}/" "\"\$script_dir_path\"/../../../../${HOST_TRIPLET}/")
        endforeach()
    endif()

    if(NOT VCPKG_BUILD_TYPE)
      set(target_qt_conf_debug "${CURRENT_PACKAGES_DIR}/tools/Qt6/target_qt_debug.conf")
      configure_file("${target_qt_conf}" "${target_qt_conf_debug}" COPYONLY)
      file(READ "${target_qt_conf_debug}" qt_conf_contents)
      string(REGEX REPLACE "=(bin|lib|Qt6/plugins|Qt6/qml)" "=debug/\\1" qt_conf_contents ${qt_conf_contents})
      file(WRITE "${target_qt_conf_debug}" "${qt_conf_contents}")

      configure_file("${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qmake${script_suffix}" "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qmake.debug${script_suffix}" COPYONLY)
      vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qmake.debug${script_suffix}" "target_qt.conf" "target_qt_debug.conf")
    endif()
endif()

if(VCPKG_TARGET_IS_EMSCRIPTEN)
  vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6Core/Qt6WasmMacros.cmake" "_qt_test_emscripten_version()" "") # this is missing a include(QtPublicWasmToolchainHelpers)
endif()

if(VCPKG_TARGET_IS_WINDOWS)
    set(_DLL_FILES brotlicommon brotlidec bz2 freetype harfbuzz libpng16)
    set(DLLS_TO_COPY "")
    foreach(_file IN LISTS _DLL_FILES)
        if(EXISTS "${CURRENT_INSTALLED_DIR}/bin/${_file}.dll")
            list(APPEND DLLS_TO_COPY "${CURRENT_INSTALLED_DIR}/bin/${_file}.dll")
        endif()
    endforeach()
    file(COPY ${DLLS_TO_COPY} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
endif()

set(hostinfofile "${CURRENT_PACKAGES_DIR}/share/Qt6HostInfo/Qt6HostInfoConfig.cmake")
file(READ "${hostinfofile}" _contents)
string(REPLACE [[set(QT6_HOST_INFO_LIBEXECDIR "bin")]] [[set(QT6_HOST_INFO_LIBEXECDIR "tools/Qt6/bin")]] _contents "${_contents}")
string(REPLACE [[set(QT6_HOST_INFO_BINDIR "bin")]] [[set(QT6_HOST_INFO_BINDIR "tools/Qt6/bin")]] _contents "${_contents}")
file(WRITE "${hostinfofile}" "${_contents}")

if(NOT VCPKG_CROSSCOMPILING OR EXISTS "${CURRENT_PACKAGES_DIR}/share/Qt6CoreTools/Qt6CoreToolsAdditionalTargetInfo.cmake")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6CoreTools/Qt6CoreToolsAdditionalTargetInfo.cmake"
                         "CMAKE_CURRENT_LIST_DIR}/../../bin/syncqt"
                         "CMAKE_CURRENT_LIST_DIR}/../../tools/Qt6/bin/syncqt"
                         IGNORE_UNCHANGED)
endif()

set(configfile "${CURRENT_PACKAGES_DIR}/share/Qt6CoreTools/Qt6CoreToolsTargets-debug.cmake")
if(EXISTS "${configfile}")
    file(READ "${configfile}" _contents)
    if(EXISTS "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qmake.exe")
        file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/qmake.debug.bat" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
        string(REPLACE [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/qmake.exe"]] [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/qmake.debug.bat"]] _contents "${_contents}")
    endif()
    if(EXISTS "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/qtpaths.exe")
        file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/qtpaths.debug.bat" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
        string(REPLACE [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/qtpaths.exe"]] [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/qtpaths.debug.bat"]] _contents "${_contents}")
    endif()
    if(EXISTS "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/windeployqt.exe")
        file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/windeployqt.debug.bat" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
        string(REPLACE [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/windeployqt.exe"]] [[ "${_IMPORT_PREFIX}/tools/Qt6/bin/windeployqt.debug.bat"]] _contents "${_contents}")
    endif()
    file(WRITE "${configfile}" "${_contents}")
endif()

if(VCPKG_CROSSCOMPILING)
    set(dep_file "${CURRENT_PACKAGES_DIR}/share/Qt6/Qt6Dependencies.cmake")
    file(READ "${dep_file}" dep_contents)
    string(REPLACE "${CURRENT_HOST_INSTALLED_DIR}" "\${CMAKE_CURRENT_LIST_DIR}/../../../${HOST_TRIPLET}" dep_contents "${dep_contents}")
    string(REPLACE
        "    set(__qt_platform_requires_host_info_package FALSE)"
        "    set(__qt_platform_requires_host_info_package TRUE)"
        dep_contents "${dep_contents}")
    file(WRITE "${dep_file}" "${dep_contents}")
endif()
vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/Qt6/Qt6Config.cmake" "{Qt6HostInfo_DIR}/.." "{Qt6HostInfo_DIR}/../..")

function(remove_original_cmake_path file)
    file(READ "${file}" _contents)
    string(REGEX REPLACE "original_cmake_path=[^\n]*" "original_cmake_path=''" _contents "${_contents}")
    file(WRITE "${file}" "${_contents}")
endfunction()

if(NOT VCPKG_TARGET_IS_WINDOWS AND NOT CMAKE_HOST_WIN32)
    foreach(file "qt-cmake${script_suffix}" "qt-cmake-private${script_suffix}")
        remove_original_cmake_path("${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/${file}")
        if(NOT VCPKG_BUILD_TYPE)
            remove_original_cmake_path("${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/debug/${file}")
        endif()
    endforeach()
endif()

if(VCPKG_TARGET_IS_WINDOWS)
  # dlls owned but not automatically installed by qtbase
  # this is required to avoid ownership troubles in downstream qt modules
  set(qtbase_owned_dlls
        double-conversion.dll
        md4c.dll
        libcrypto-3-${VCPKG_TARGET_ARCHITECTURE}.dll
        libcrypto-3.dll # for x86
        pcre2-16.dll
        z.dll
        zlib1.dll
        zstd.dll
  )
  # Dynamically find ICU DLLs (the version number tracks the ICU dependency, e.g. icudt78.dll for ICU 78)
  file(GLOB _qt_icu_dt_dlls "${CURRENT_INSTALLED_DIR}/bin/icudt*.dll")
  foreach(_icu_dll IN LISTS _qt_icu_dt_dlls)
    get_filename_component(_icu_name "${_icu_dll}" NAME)
    string(REGEX REPLACE "^icudt([0-9]+)\\.dll$" "\\1" _icu_ver "${_icu_name}")
    list(APPEND qtbase_owned_dlls "icudt${_icu_ver}.dll" "icuin${_icu_ver}.dll" "icuuc${_icu_ver}.dll")
  endforeach()
  if("dbus" IN_LIST FEATURES)
    list(APPEND qtbase_owned_dlls dbus-1-3.dll)
  endif()
  list(TRANSFORM qtbase_owned_dlls PREPEND "${CURRENT_INSTALLED_DIR}/bin/")
  foreach(dll IN LISTS qtbase_owned_dlls)
    if(NOT EXISTS "${dll}") # Need to remove non-existant dlls since dependencies could have been build statically
      list(REMOVE_ITEM qtbase_owned_dlls "${dll}")
    endif()
  endforeach()
  file(COPY ${qtbase_owned_dlls} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
endif()
