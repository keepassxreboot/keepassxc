if(Q_WS_X11)
  find_package(X11)
  if(PRINT_SUMMARY)
    add_feature_info(libXtest X11_XTest_FOUND "The X11 XTEST Protocol library is required for auto-type")
  endif()

  if(X11_FOUND AND X11_XTest_FOUND)
    add_subdirectory(x11)
  endif()
endif()

if(Q_WS_MAC)
  add_subdirectory(mac)
endif()

if(WITH_TESTS)
  add_subdirectory(test)
endif()
