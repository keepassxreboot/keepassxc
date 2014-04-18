
set(qhttpserver_MOC_HDRS
  qhttpserver.h
  qhttpresponse.h
  qhttprequest.h
  qhttpconnection.h
)

IF (NOT Qt5Core_FOUND)
  qt4_wrap_cpp(qhttpserver_MOC_SRCS ${qhttpserver_MOC_HDRS})
ENDIF()

set (qhttpserver_SRCS qhttpconnection.cpp  qhttprequest.cpp  qhttpresponse.cpp  qhttpserver.cpp
                      http-parser/http_parser.c http-parser/url_parser.c)
set (qhttpserver_HEADERS qhttpconnection.h  qhttprequest.h  qhttpresponse.h  qhttpserver.h
                         http-parser/http_parser.h)

INCLUDE_DIRECTORIES(http-parser)

add_library (qhttpserver STATIC ${qhttpserver_SRCS} ${qhttpserver_MOC_SRCS} ${qhttpserver_HEADERS})
