project(qhttp)

set(qhttp_SOURCES
    qhttpabstracts.cpp
    qhttpserverconnection.cpp
    qhttpserverrequest.cpp
    qhttpserverresponse.cpp
    qhttpserver.cpp
    qhttpclientrequest.cpp
    qhttpclientresponse.cpp
    qhttpclient.cpp
    http-parser/http_parser.c
)

add_library(qhttp STATIC ${qhttp_SOURCES})
target_compile_definitions(qhttp PUBLIC QHTTP_MEMORY_LOG=0 QHTTP_EXPORT)
target_include_directories(qhttp PRIVATE .)
target_link_libraries(qhttp Qt5::Core Qt5::Network)