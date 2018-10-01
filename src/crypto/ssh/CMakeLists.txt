if(WITH_XC_CRYPTO_SSH)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

    set(crypto_ssh_SOURCES
        bcrypt_pbkdf.cpp
        blowfish.c
	ASN1Key.cpp
	BinaryStream.cpp
	OpenSSHKey.cpp
    )

    add_library(crypto_ssh STATIC ${crypto_ssh_SOURCES})
    target_link_libraries(crypto_ssh Qt5::Core ${GCRYPT_LIBRARIES})
endif()
