### Steps to update the qt6 ports
## 1. Change QT_VERSION below to the new version
## 2. Set QT_UPDATE_VERSION to 1
## 3. Add any new Qt modules to QT_PORTS
## 4. Run a build of `qtbase`
## 5. Fix any intermediate failures by adding the module into QT_FROM_GITHUB, QT_FROM_GITHUB_BRANCH, or QT_FROM_QT_GIT as appropriate
## 6. The build should fail with "Done downloading version and emitting hashes." This will have changed out the vcpkg.json versions of the qt ports and rewritten qt_port_data.cmake
## 7. Set QT_UPDATE_VERSION back to 0

set(QT_VERSION 6.11.2)

set(QT_DEV_BRANCH 0)

set(QT_UPDATE_VERSION 0)

### Setting up the git tag.

set(QT_PORTS qt
             qtbase
             qttools
             qtdeclarative
             qtsvg
             qt5compat
             qtshadertools
             qtquicktimeline
             qtquick3d
             qttranslations
             qtwayland
             qtdoc
             qtcoap
             qtopcua
             qtimageformats
             qtmqtt
             qtnetworkauth
             qt3d)
if(QT_VERSION VERSION_GREATER_EQUAL 6.1)
    list(APPEND QT_PORTS
             ## New in 6.1
             qtactiveqt
             qtdatavis3d
             qtdeviceutilities
             qtlottie
             qtscxml
             qtvirtualkeyboard
             qtcharts)
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.2)
    list(APPEND QT_PORTS
             ## New in 6.2
             qtconnectivity
             qtpositioning
             qtlocation # back in 6.5 as tech preview
             qtmultimedia
             qtremoteobjects
             qtsensors
             qtserialbus
             qtserialport
             qtwebchannel
             qtwebengine
             qtwebsockets
             qtwebview)
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.2.2)
    list(APPEND QT_PORTS
             ## New in 6.2.2
             qtinterfaceframework
             qtapplicationmanager)
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.3.0)
    list(APPEND QT_PORTS
             ## New in 6.3.0
             qtlanguageserver)
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.4.0)
    list(APPEND QT_PORTS
             ## New in 6.4.0
             qthttpserver
             qtquick3dphysics
             qtspeech)
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.5.0)
    list(APPEND QT_PORTS
             ## New in 6.5.0
             qtgrpc
             qtquickeffectmaker
             )
endif()
if(QT_VERSION VERSION_GREATER_EQUAL 6.6.0)
    list(APPEND QT_PORTS
             ## New in 6.6.0
             qtgraphs
             #qtvncserver # only commercial
             #qtinsighttracker
             )
endif()
#qtinsighttracker
#qtvncserver
#qtgraphs

# 1. By default, modules come from the official release
# 2. These modules are mirrored to github and have tags matching the release
set(QT_FROM_GITHUB qtcoap qtopcua qtmqtt qtapplicationmanager qtinterfaceframework)
# 3. These modules are mirrored to github and have branches matching the release
set(QT_FROM_GITHUB_BRANCH qtdeviceutilities)
# 4. These modules are not mirrored to github and not part of the release
set(QT_FROM_QT_GIT "")
# For beta releases uncomment the next two lines and comment the lines with QT_FROM_GITHUB, QT_FROM_GITHUB_BRANCH, QT_FROM_QT_GIT
#set(QT_FROM_QT_GIT ${QT_PORTS})
#list(POP_FRONT QT_FROM_QT_GIT)

function(qt_get_url_filename qt_port out_urls out_filename)
    if("${qt_port}" IN_LIST QT_FROM_GITHUB)
        set(urls "https://github.com/qt/${qt_port}/archive/v${QT_VERSION}.tar.gz")
        set(filename "qt-${qt_port}-v${QT_VERSION}.tar.gz")
    elseif("${qt_port}" IN_LIST QT_FROM_GITHUB_BRANCH)
        set(urls "https://github.com/qt/${qt_port}/archive/${QT_VERSION}.tar.gz")
        set(filename "qt-${qt_port}-${QT_VERSION}.tar.gz")
    else()
        string(SUBSTRING "${QT_VERSION}" 0 4 qt_major_minor)

        if(NOT QT_DEV_BRANCH)
            set(branch_subpath "archive")
        else()
            set(branch_subpath "development_releases")
        endif()

        set(filename "${qt_port}-everywhere-src-${QT_VERSION}.tar.xz")
        set(mirrors
            "https://download.qt.io/"
            "https://mirrors.ocf.berkeley.edu/qt/"
        )
        set(url_subpath "${branch_subpath}/qt/${qt_major_minor}/${QT_VERSION}/submodules/${filename}")
        list(TRANSFORM mirrors APPEND "${url_subpath}" OUTPUT_VARIABLE urls)
    endif()
    set(${out_urls} ${urls} PARENT_SCOPE)
    set(${out_filename} "${filename}" PARENT_SCOPE)
endfunction()

if(QT_UPDATE_VERSION)
    if(NOT PORT STREQUAL "qtbase")
        message(FATAL_ERROR "QT_UPDATE_VERSION must be used from the root 'qtbase' package")
    endif()
    set(VCPKG_USE_HEAD_VERSION 1)
    set(msg "" CACHE INTERNAL "")
    foreach(qt_port IN LISTS QT_PORTS)
        set(port_json "${CMAKE_CURRENT_LIST_DIR}/../../${qt_port}/vcpkg.json")
        file(READ "${port_json}" _control_contents)
        string(REGEX REPLACE "\"version(-(string|semver))?\": [^\n]+\n" "\"version\": \"${QT_VERSION}\",\n" _control_contents "${_control_contents}")
        string(REGEX REPLACE "\"port-version\": [^\n]+\n" "" _control_contents "${_control_contents}")
        file(WRITE "${port_json}" "${_control_contents}")
        configure_file("${port_json}" "${port_json}" @ONLY NEWLINE_STYLE LF)

        set(port_data "")
        if(qt_port STREQUAL "qt")
            continue()
        endif()
        if("${qt_port}" IN_LIST QT_FROM_QT_GIT)
            vcpkg_find_acquire_program(GIT)
            execute_process(
                COMMAND "${GIT}" ls-remote -t "https://code.qt.io/cgit/qt/${qt_port}.git" "v${QT_VERSION}"
                OUTPUT_VARIABLE out
            )
            string(SUBSTRING "${out}" 0 40 tag_sha)
            string(APPEND msg "set(${qt_port}_REF ${tag_sha})\n")
            string(APPEND port_data "set(${qt_port}_REF ${tag_sha})\n")
            string(APPEND port_data "set(${qt_port}_URL \"https://code.qt.io/cgit/qt/${qt_port}.git\")\n")
        else()
            qt_get_url_filename("${qt_port}" urls filename)
            vcpkg_download_distfile(archive
                URLS ${urls}
                FILENAME "${filename}"
                SKIP_SHA512
            )
            file(SHA512 "${archive}" hash)
            string(APPEND msg "set(${qt_port}_HASH \"${hash}\")\n")
            string(APPEND port_data "set(${qt_port}_HASH \"${hash}\")\n")
            string(APPEND port_data "set(${qt_port}_URL \"${urls}\")\n")
            string(APPEND port_data "set(${qt_port}_FILENAME \"${filename}\")\n")
        endif()
        file(WRITE "${CMAKE_CURRENT_LIST_DIR}/../../${qt_port}/port.data.cmake" "${port_data}")
        configure_file("${CMAKE_CURRENT_LIST_DIR}/../../${qt_port}/port.data.cmake" "${CMAKE_CURRENT_LIST_DIR}/../../${qt_port}/port.data.cmake" @ONLY NEWLINE_STYLE LF)
    endforeach()
    message("${msg}")
    file(WRITE "${CMAKE_CURRENT_LIST_DIR}/qt_port_data_new.cmake" "${msg}")
    message(FATAL_ERROR "Done downloading version and emitting hashes.")
endif()

include("${CURRENT_PORT_DIR}/port.data.cmake")
