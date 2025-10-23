# Install script for directory: /home/runner/work/keepassxc/keepassxc/share/translations

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/keepassxc/translations" TYPE FILE FILES
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ar.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_bg.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ca.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_cs.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_da.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_de.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_el.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_en_GB.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_en_US.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_es.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_et.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_fi.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_fil.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_fr.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_fr_CA.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_he.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_hr.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_hu.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_id.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_it.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ja.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_km.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ko.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_lt.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_my.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_nb.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_nl.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_pl.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_pt_BR.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_pt_PT.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ro.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_ru.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_si.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_sk.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_sl.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_sq.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_sr.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_sv.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_th.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_tr.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_uk.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_zh_CN.qm"
    "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_zh_TW.qm"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/keepassxc/translations" TYPE FILE RENAME "keepassxc_en.qm" FILES "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/keepassxc_en_US.qm")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/runner/work/keepassxc/keepassxc/_codeql_build_dir/share/translations/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
