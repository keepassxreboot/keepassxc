cmake_minimum_required(VERSION 3.8)
project(argon2 C)

set(PROJECT_VERSION 20190702)

if(MSVC)
  add_compile_options(/W3)
endif()

include_directories(include src)

set(SRC
  src/argon2.c
  src/core.c
  src/encoding.c
  src/ref.c
  src/thread.c
  src/opt.c
  src/blake2/blake2b.c
)

set(HEADERS
  include/argon2.h
)

add_library(argon2 ${SRC})

install(
  TARGETS argon2
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

if(NOT DISABLE_INSTALL_HEADERS)
  install(FILES ${HEADERS} DESTINATION include)
endif()
