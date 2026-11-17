add_library(zxcvbn STATIC zxcvbn.c)
# Disable error-level shadow issues
if(CC_HAS_Wshadow_compatible_local)
    set_property(SOURCE zxcvbn.c APPEND PROPERTY COMPILE_OPTIONS "-Wno-shadow-compatible-local")
endif()
if(CC_HAS_Wshadow_local)
    set_property(SOURCE zxcvbn.c APPEND PROPERTY COMPILE_OPTIONS "-Wno-shadow-local")
endif()
target_include_directories(zxcvbn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Based on zxcvbn-c 2.3 plus upstream changes through f7037ff (2017-11-21),
# with KeePassXC encoding, build and warning fixes.
set_target_properties(zxcvbn PROPERTIES
    VERSION "2.3+keepassxc"
    SBOM_SOURCE_PATH src/thirdparty/zxcvbn/zxcvbn.c)
