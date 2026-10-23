include_guard(GLOBAL)

include("${CURRENT_HOST_INSTALLED_DIR}/share/vcpkg-cmake/vcpkg-port-config.cmake")
include("${CURRENT_HOST_INSTALLED_DIR}/share/vcpkg-cmake-config/vcpkg-port-config.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/qt_install_copyright.cmake")

if(NOT DEFINED QT6_DIRECTORY_PREFIX)
    set(QT6_DIRECTORY_PREFIX "Qt6/")
endif()

if(VCPKG_TARGET_IS_ANDROID)
    # ANDROID_HOME: canonical SDK environment variable
    # ANDROID_SDK_ROOT: legacy qtbase triplet variable
    if(NOT ANDROID_SDK_ROOT)
        if("$ENV{ANDROID_HOME}" STREQUAL "")
            message(FATAL_ERROR "${PORT} requires environment variable ANDROID_HOME to be set.")
        endif()
        set(ANDROID_SDK_ROOT "$ENV{ANDROID_HOME}")
    endif()
endif()

function(qt_download_submodule_impl)
    cmake_parse_arguments(PARSE_ARGV 0 "_qarg" "" "SUBMODULE" "PATCHES")

    if("${_qarg_SUBMODULE}" IN_LIST QT_FROM_QT_GIT)
        # qtinterfaceframework is not available in the release, so we fall back to a `git clone`.
        vcpkg_from_git(
            OUT_SOURCE_PATH SOURCE_PATH
            URL "${${_qarg_SUBMODULE}_URL}"
            REF "${${_qarg_SUBMODULE}_REF}"
            PATCHES ${_qarg_PATCHES}
        )
        if(PORT STREQUAL "qttools") # Keep this for beta & rc's
            vcpkg_from_git(
                OUT_SOURCE_PATH SOURCE_PATH_QLITEHTML
                URL https://code.qt.io/playground/qlitehtml.git
                REF "${${PORT}_qlitehtml_REF}"
                FETCH_REF master
                HEAD_REF master
            )
            # port 'litehtml' is not in vcpkg!
            vcpkg_from_github(
                OUT_SOURCE_PATH SOURCE_PATH_LITEHTML
                REPO litehtml/litehtml
                REF "${${PORT}_litehtml_REF}"
                SHA512 "${${PORT}_litehtml_HASH}"
                HEAD_REF master
            )
            file(COPY "${SOURCE_PATH_QLITEHTML}/" DESTINATION "${SOURCE_PATH}/src/assistant/qlitehtml")
            file(COPY "${SOURCE_PATH_LITEHTML}/" DESTINATION "${SOURCE_PATH}/src/assistant/qlitehtml/src/3rdparty/litehtml")
        elseif(PORT STREQUAL "qtwebengine")
            vcpkg_from_git(
                OUT_SOURCE_PATH SOURCE_PATH_WEBENGINE
                URL https://code.qt.io/qt/qtwebengine-chromium.git
                REF "${${PORT}_chromium_REF}"
            )
            if(NOT EXISTS "${SOURCE_PATH}/src/3rdparty/chromium")
                file(RENAME "${SOURCE_PATH_WEBENGINE}/chromium" "${SOURCE_PATH}/src/3rdparty/chromium")
            endif()
            if(NOT EXISTS "${SOURCE_PATH}/src/3rdparty/gn")
                file(RENAME "${SOURCE_PATH_WEBENGINE}/gn" "${SOURCE_PATH}/src/3rdparty/gn")
            endif()
        endif()
    else()
        if(VCPKG_USE_HEAD_VERSION)
            set(sha512 SKIP_SHA512)
        elseif(NOT DEFINED "${_qarg_SUBMODULE}_HASH")
            message(FATAL_ERROR "No information for ${_qarg_SUBMODULE} -- add it to QT_PORTS and run qtbase in QT_UPDATE_VERSION mode first")
        else()
            set(sha512 SHA512 "${${_qarg_SUBMODULE}_HASH}")
        endif()

        vcpkg_download_distfile(archive
            URLS ${${_qarg_SUBMODULE}_URL}
            FILENAME ${${_qarg_SUBMODULE}_FILENAME}
            ${sha512}
        )
        vcpkg_extract_source_archive(
            SOURCE_PATH
            ARCHIVE "${archive}"
            PATCHES ${_qarg_PATCHES}
        )
    endif()
    set(SOURCE_PATH "${SOURCE_PATH}" PARENT_SCOPE)
endfunction()

function(qt_download_submodule)
    cmake_parse_arguments(PARSE_ARGV 0 "_qarg" "" "" "PATCHES")

    qt_download_submodule_impl(SUBMODULE "${PORT}" PATCHES ${_qarg_PATCHES})

    set(SOURCE_PATH "${SOURCE_PATH}" PARENT_SCOPE)
endfunction()


function(qt_cmake_configure)
    cmake_parse_arguments(PARSE_ARGV 0 "_qarg" "DISABLE_NINJA;DISABLE_PARALLEL_CONFIGURE"
                      ""
                      "TOOL_NAMES;OPTIONS;OPTIONS_DEBUG;OPTIONS_RELEASE;OPTIONS_MAYBE_UNUSED")

    vcpkg_find_acquire_program(PERL) # Perl is probably required by all qt ports for syncqt
    get_filename_component(PERL_PATH ${PERL} DIRECTORY)
    vcpkg_add_to_path(${PERL_PATH})
    if(NOT PORT STREQUAL "qtwebengine" OR QT_IS_LATEST) # qtwebengine requires python2; since 6.3 python3
        vcpkg_find_acquire_program(PYTHON3) # Python is required by some qt ports
        get_filename_component(PYTHON3_PATH ${PYTHON3} DIRECTORY)
        vcpkg_add_to_path(${PYTHON3_PATH})
    endif()

    if(NOT PORT MATCHES "^qtbase")
        list(APPEND _qarg_OPTIONS "-DQT_SYNCQT:PATH=${CURRENT_HOST_INSTALLED_DIR}/tools/Qt6/bin/syncqt.pl")
    endif()
    set(PERL_OPTION "-DHOST_PERL:PATH=${PERL}")

    set(ninja_option "")
    if(_qarg_DISABLE_NINJA)
        set(ninja_option WINDOWS_USE_MSBUILD)
    endif()

    set(disable_parallel "")
    if(_qarg_DISABLE_PARALLEL_CONFIGURE)
        set(disable_parallel DISABLE_PARALLEL_CONFIGURE)
    endif()

    if(VCPKG_CROSSCOMPILING)
        list(APPEND _qarg_OPTIONS "-DQT_HOST_PATH=${CURRENT_HOST_INSTALLED_DIR}")
        list(APPEND _qarg_OPTIONS "-DQT_HOST_PATH_CMAKE_DIR:PATH=${CURRENT_HOST_INSTALLED_DIR}/share")
    endif()

    # Disable warning for CMAKE_(REQUIRE|DISABLE)_FIND_PACKAGE_<packagename>
    string(REGEX MATCHALL "CMAKE_DISABLE_FIND_PACKAGE_[^:=]+" disabled_find_package "${_qarg_OPTIONS}")
    list(APPEND _qarg_OPTIONS_MAYBE_UNUSED ${disabled_find_package})

    string(REGEX MATCHALL "CMAKE_REQUIRE_FIND_PACKAGE_[^:=]+(:BOOL)?=OFF" require_find_package "${_qarg_OPTIONS}")
    list(TRANSFORM require_find_package REPLACE "(:BOOL)?=OFF" "")
    list(APPEND _qarg_OPTIONS_MAYBE_UNUSED ${require_find_package})

    # Disable unused warnings for disabled features. Qt might decide to not emit the feature variables if other features are deactivated.
    string(REGEX MATCHALL "(QT_)?FEATURE_[^:=]+(:BOOL)?=OFF" disabled_features "${_qarg_OPTIONS}")
    list(TRANSFORM disabled_features REPLACE "(:BOOL)?=OFF" "")
    list(APPEND _qarg_OPTIONS_MAYBE_UNUSED ${disabled_features})

    list(APPEND _qarg_OPTIONS "-DQT_NO_FORCE_SET_CMAKE_BUILD_TYPE:BOOL=ON")

    if(VCPKG_TARGET_IS_ANDROID)
        list(APPEND _qarg_OPTIONS "-DANDROID_SDK_ROOT=${ANDROID_SDK_ROOT}")
    endif()

    if(NOT PORT MATCHES "qtbase")
        list(APPEND _qarg_OPTIONS "-DQT_MKSPECS_DIR:PATH=${CURRENT_HOST_INSTALLED_DIR}/share/Qt6/mkspecs")
    endif()

    if(NOT DEFINED VCPKG_OSX_DEPLOYMENT_TARGET)
        list(APPEND _qarg_OPTIONS "-DCMAKE_OSX_DEPLOYMENT_TARGET=14")
    endif()

    vcpkg_cmake_configure(
        SOURCE_PATH "${SOURCE_PATH}"
        ${ninja_option}
        ${disable_parallel}
        OPTIONS
            -DQT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK=ON
            -DQT_NO_FORCE_SET_CMAKE_BUILD_TYPE:BOOL=ON
            -DQT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS:BOOL=ON # We don't want Qt to mess with users toolchain settings.
            -DCMAKE_FIND_PACKAGE_TARGETS_GLOBAL=ON # Because Qt doesn't correctly scope find_package calls.
            #-DQT_HOST_PATH=<somepath> # For crosscompiling
            #-DQT_PLATFORM_DEFINITION_DIR=mkspecs/win32-msvc
            #-DQT_QMAKE_TARGET_MKSPEC=win32-msvc
            #-DQT_USE_CCACHE
            -DQT_BUILD_EXAMPLES:BOOL=OFF
            -DQT_BUILD_TESTS:BOOL=OFF
            -DQT_BUILD_BENCHMARKS:BOOL=OFF
            ${PERL_OPTION}
            -DINSTALL_BINDIR:STRING=bin
            -DINSTALL_LIBEXECDIR:STRING=bin
            -DINSTALL_PLUGINSDIR:STRING=${qt_plugindir}
            -DINSTALL_QMLDIR:STRING=${qt_qmldir}
            ${_qarg_OPTIONS}
            "-DQT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX:STRING=${CURRENT_INSTALLED_DIR}"
        OPTIONS_RELEASE
            ${_qarg_OPTIONS_RELEASE}
            -DINSTALL_DOCDIR:STRING=doc/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_INCLUDEDIR:STRING=include/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_DESCRIPTIONSDIR:STRING=share/Qt6/modules
            -DINSTALL_MKSPECSDIR:STRING=share/Qt6/mkspecs
            -DINSTALL_TRANSLATIONSDIR:STRING=translations/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_CMAKEDIR:STRING=share
        OPTIONS_DEBUG
            # -DFEATURE_debug:BOOL=ON only needed by qtbase and auto detected?
            -DINSTALL_DOCDIR:STRING=../doc/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_INCLUDEDIR:STRING=../include/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_TRANSLATIONSDIR:STRING=../translations/${QT6_DIRECTORY_PREFIX}
            -DINSTALL_DESCRIPTIONSDIR:STRING=../share/Qt6/modules
            -DINSTALL_MKSPECSDIR:STRING=../share/Qt6/mkspecs
            -DINSTALL_CMAKEDIR:STRING=share
            ${_qarg_OPTIONS_DEBUG}
        MAYBE_UNUSED_VARIABLES
            INSTALL_BINDIR
            INSTALL_DOCDIR
            INSTALL_LIBEXECDIR
            INSTALL_QMLDIR  # No qml files
            INSTALL_TRANSLATIONSDIR # No translations
            INSTALL_PLUGINSDIR # No plugins
            INSTALL_DESCRIPTIONSDIR
            INSTALL_INCLUDEDIR
            HOST_PERL
            QT_SYNCQT
            QT_NO_FORCE_SET_CMAKE_BUILD_TYPE
            QT_FORCE_WARN_APPLE_SDK_AND_XCODE_CHECK
            QT_TOOLCHAIN_RELOCATABLE_INSTALL_PREFIX
            ${_qarg_OPTIONS_MAYBE_UNUSED}
            INPUT_bundled_xcb_xinput
            INPUT_freetype
            INPUT_harfbuzz
            INPUT_libjpeg
            INPUT_libmd4c
            INPUT_libpng
            INPUT_opengl
            INPUT_openssl
            INPUT_xcb
            INPUT_xkbcommon
    )
    foreach(suffix IN ITEMS dbg rel)
        if(EXISTS "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-${suffix}/config.summary")
            file(COPY_FILE
                "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-${suffix}/config.summary"
                "${CURRENT_BUILDTREES_DIR}/config.summary-${TARGET_TRIPLET}-${suffix}.log"
            )
        endif()
    endforeach()
endfunction()

function(qt_fix_prl_files)
    file(TO_CMAKE_PATH "${CURRENT_PACKAGES_DIR}/lib" package_dir)
    file(TO_CMAKE_PATH "${package_dir}/lib" lib_path)
    file(TO_CMAKE_PATH "${package_dir}/include/Qt6" include_path)
    file(TO_CMAKE_PATH "${CURRENT_INSTALLED_DIR}" install_prefix)
    file(GLOB_RECURSE prl_files "${CURRENT_PACKAGES_DIR}/*.prl" "${CURRENT_PACKAGES_DIR}/*.pri")
    foreach(prl_file IN LISTS prl_files)
        file(READ "${prl_file}" _contents)
        string(REPLACE "${lib_path}" "\$\$[QT_INSTALL_LIBS]" _contents "${_contents}")
        string(REPLACE "${include_path}" "\$\$[QT_INSTALL_HEADERS]" _contents "${_contents}")
        string(REPLACE "${install_prefix}" "\$\$[QT_INSTALL_PREFIX]" _contents "${_contents}")
        string(REPLACE "[QT_INSTALL_PREFIX]/lib/objects-Debug" "[QT_INSTALL_LIBS]/objects-Debug" _contents "${_contents}")
        string(REPLACE "[QT_INSTALL_PREFIX]/Qt6/qml" "[QT_INSTALL_QML]" _contents "${_contents}")
        #Note: This only works without an extra if case since QT_INSTALL_PREFIX is the same for debug and release
        file(WRITE "${prl_file}" "${_contents}")
    endforeach()
endfunction()

function(qt_fixup_and_cleanup)
        cmake_parse_arguments(PARSE_ARGV 0 "_qarg" ""
                      ""
                      "TOOL_NAMES")
    vcpkg_copy_pdbs()

    ## Handle PRL files
    qt_fix_prl_files()

    ## Handle CMake files.
    set(COMPONENTS)
    file(GLOB COMPONENTS_OR_FILES LIST_DIRECTORIES true "${CURRENT_PACKAGES_DIR}/share/Qt6*")
    list(REMOVE_ITEM COMPONENTS_OR_FILES "${CURRENT_PACKAGES_DIR}/share/Qt6")
    foreach(_glob IN LISTS COMPONENTS_OR_FILES)
        if(IS_DIRECTORY "${_glob}")
            string(REPLACE "${CURRENT_PACKAGES_DIR}/share/Qt6" "" _component "${_glob}")
            debug_message("Adding cmake component: '${_component}'")
            list(APPEND COMPONENTS ${_component})
        endif()
    endforeach()

    foreach(_comp IN LISTS COMPONENTS)
        if(EXISTS "${CURRENT_PACKAGES_DIR}/share/Qt6${_comp}")
            vcpkg_cmake_config_fixup(PACKAGE_NAME "Qt6${_comp}" CONFIG_PATH "share/Qt6${_comp}" TOOLS_PATH "tools/Qt6/bin")
        else()
            message(STATUS "WARNING: Qt component ${_comp} not found/built!")
        endif()
    endforeach()

    #fix debug plugin paths (should probably be fixed in vcpkg_cmake_config_fixup)
    file(GLOB_RECURSE DEBUG_CMAKE_TARGETS "${CURRENT_PACKAGES_DIR}/share/**/*Targets-debug.cmake")
    debug_message("DEBUG_CMAKE_TARGETS:${DEBUG_CMAKE_TARGETS}")
    foreach(_debug_target IN LISTS DEBUG_CMAKE_TARGETS)
        vcpkg_replace_string("${_debug_target}" "{_IMPORT_PREFIX}/${qt_plugindir}" "{_IMPORT_PREFIX}/debug/${qt_plugindir}" IGNORE_UNCHANGED)
        vcpkg_replace_string("${_debug_target}" "{_IMPORT_PREFIX}/${qt_qmldir}" "{_IMPORT_PREFIX}/debug/${qt_qmldir}" IGNORE_UNCHANGED)
    endforeach()

    file(GLOB_RECURSE STATIC_CMAKE_TARGETS "${CURRENT_PACKAGES_DIR}/share/Qt6Qml/QmlPlugins/*.cmake")
    foreach(_plugin_target IN LISTS STATIC_CMAKE_TARGETS)
        # restore a single get_filename_component which was remove by vcpkg_cmake_config_fixup
        vcpkg_replace_string("${_plugin_target}"
                             [[get_filename_component(_IMPORT_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)]]
                             "get_filename_component(_IMPORT_PREFIX \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)\nget_filename_component(_IMPORT_PREFIX \"\${_IMPORT_PREFIX}\" PATH)"
                             IGNORE_UNCHANGED)
    endforeach()

    set(qt_tooldest "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
    set(qt_searchdir "${CURRENT_PACKAGES_DIR}/bin")
    ## Handle Tools
    foreach(_tool IN LISTS _qarg_TOOL_NAMES)
        if(NOT EXISTS "${CURRENT_PACKAGES_DIR}/bin/${_tool}${VCPKG_TARGET_EXECUTABLE_SUFFIX}")
            debug_message("Removed '${_tool}' from copy tools list since it was not found!")
            list(REMOVE_ITEM _qarg_TOOL_NAMES ${_tool})
        endif()
    endforeach()
    if(_qarg_TOOL_NAMES)
        set(tool_names ${_qarg_TOOL_NAMES})
        vcpkg_copy_tools(TOOL_NAMES ${tool_names} SEARCH_DIR "${qt_searchdir}" DESTINATION "${qt_tooldest}" AUTO_CLEAN)
    endif()

    if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
        if(EXISTS "${CURRENT_PACKAGES_DIR}/bin/")
            file(COPY "${CURRENT_PACKAGES_DIR}/bin/" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin")
        endif()
        file(GLOB_RECURSE _installed_dll_files RELATIVE "${CURRENT_INSTALLED_DIR}/tools/Qt6/bin" "${CURRENT_INSTALLED_DIR}/tools/Qt6/bin/*.dll")
        foreach(_dll_to_remove IN LISTS _installed_dll_files)
            file(GLOB_RECURSE _packaged_dll_file "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/${_dll_to_remove}")
            if(EXISTS "${_packaged_dll_file}")
                file(REMOVE "${_packaged_dll_file}")
            endif()
        endforeach()
        file(GLOB_RECURSE _folders LIST_DIRECTORIES true "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/**/")
        file(GLOB_RECURSE _files "${CURRENT_PACKAGES_DIR}/tools/Qt6/bin/**/")
        if(_files)
            list(REMOVE_ITEM _folders ${_files})
        endif()
        foreach(_dir IN LISTS _folders)
            if(NOT "${_remaining_dll_files}" MATCHES "${_dir}")
                file(REMOVE_RECURSE "${_dir}")
            endif()
        endforeach()
    endif()
    file(REMOVE_RECURSE
                        "${CURRENT_PACKAGES_DIR}/debug/share"
                        "${CURRENT_PACKAGES_DIR}/debug/include"
                        )

    if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
        file(GLOB_RECURSE _bin_files "${CURRENT_PACKAGES_DIR}/bin/*")
        if(NOT _bin_files STREQUAL "")
            message(STATUS "Remaining files in bin: '${_bin_files}'")
        else() # Only clean if empty otherwise let vcpkg throw and error.
            file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin/" "${CURRENT_PACKAGES_DIR}/debug/bin/")
        endif()
    endif()

    vcpkg_fixup_pkgconfig()
endfunction()

function(qt_install_submodule)
    cmake_parse_arguments(PARSE_ARGV 0 "_qis" "DISABLE_NINJA"
                          ""
                          "PATCHES;TOOL_NAMES;CONFIGURE_OPTIONS;CONFIGURE_OPTIONS_DEBUG;CONFIGURE_OPTIONS_RELEASE;CONFIGURE_OPTIONS_MAYBE_UNUSED")

    set(qt_plugindir ${QT6_DIRECTORY_PREFIX}plugins)
    set(qt_qmldir ${QT6_DIRECTORY_PREFIX}qml)

    qt_download_submodule(PATCHES ${_qis_PATCHES})

    if(VCPKG_TARGET_IS_ANDROID)
        # Qt only supports dynamic linkage on Android,
        # https://bugreports.qt.io/browse/QTBUG-32618.
        # It requires libc++_shared, cf. <qtbase>/cmake/QtPlatformAndroid.cmake
        # and https://developer.android.com/ndk/guides/cpp-support#sr
        vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)
    endif()

    if(_qis_DISABLE_NINJA)
        set(_opt DISABLE_NINJA)
    endif()
    qt_cmake_configure(${_opt}
                       OPTIONS ${_qis_CONFIGURE_OPTIONS}
                       OPTIONS_DEBUG ${_qis_CONFIGURE_OPTIONS_DEBUG}
                       OPTIONS_RELEASE ${_qis_CONFIGURE_OPTIONS_RELEASE}
                       OPTIONS_MAYBE_UNUSED ${_qis_CONFIGURE_OPTIONS_MAYBE_UNUSED}
                       )

    vcpkg_cmake_install(ADD_BIN_TO_PATH)

    qt_fixup_and_cleanup(TOOL_NAMES ${_qis_TOOL_NAMES})

    qt_install_copyright("${SOURCE_PATH}")
    set(SOURCE_PATH "${SOURCE_PATH}" PARENT_SCOPE)
endfunction()

include("${CMAKE_CURRENT_LIST_DIR}/qt_port_details.cmake")
