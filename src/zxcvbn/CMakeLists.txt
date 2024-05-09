add_library(zxcvbn STATIC zxcvbn.c)
# Disable error-level shadow issues
if(CC_HAS_Wshadow_compatible_local)
    set_property(SOURCE zxcvbn.c APPEND PROPERTY COMPILE_OPTIONS "-Wno-shadow-compatible-local")
endif()
if(CC_HAS_Wshadow_local)
    set_property(SOURCE zxcvbn.c APPEND PROPERTY COMPILE_OPTIONS "-Wno-shadow-local")
endif()
target_include_directories(zxcvbn PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
