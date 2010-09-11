# - Try to find the GNU Libgcrypt library
# Once done this will define
#
#  LIBGCRYPT_FOUND - system has the Libgcrypt library
#  LIBGCRYPT_LIBS - The libraries needed to use Libgcrypt

# Copyright (c) 2006, Pino Toscano, <toscano.pino@tiscali.it>
# Copyright (c) 2008, Modestas Vainius, <modestas@vainius.eu>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying LICENSE.BSD file.

include(CheckIncludeFiles)

check_include_files(gcrypt.h HAVE_GCRYPT_H)

if (HAVE_GCRYPT_H)
   set(LIBGCRYPT_HEADERS_FOUND TRUE)
endif (HAVE_GCRYPT_H)

if (LIBGCRYPT_HEADERS_FOUND)
   find_library(LIBGCRYPT_LIBS NAMES gcrypt )
endif (LIBGCRYPT_HEADERS_FOUND)

if (LIBGCRYPT_LIBS)
   set(LIBGCRYPT_FOUND TRUE)
   message(STATUS "Libgcrypt found: ${LIBGCRYPT_LIBS}")
elseif (Libgcrypt_FIND_REQUIRED)
   message(FATAL_ERROR "Could not find Libgcrypt")
endif (LIBGCRYPT_LIBS)
