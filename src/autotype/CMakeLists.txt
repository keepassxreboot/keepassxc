if(UNIX AND NOT APPLE)
  find_package(X11)
  find_package(Qt5X11Extras)
  add_feature_info(libXtest X11_XTest_FOUND "The X11 XTEST Protocol library is required for auto-type")
  add_feature_info(libXtest Qt5X11Extras_FOUND "The Qt5X11Extras library is required for auto-type")

  if(X11_FOUND AND X11_XTest_FOUND AND Qt5X11Extras_FOUND)
    add_subdirectory(x11)
  endif()
endif()

if(WITH_TESTS)
  add_subdirectory(test)
endif()
