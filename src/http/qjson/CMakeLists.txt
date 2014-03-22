# add_custom_command (OUTPUT ${qjson_SOURCE_DIR}/lib/json_parser.cc
#                    PRE_BUILD
#                    COMMAND bison -t -o json_parser.cc -d json_parser.yy
#                    DEPENDS json_parser.yy
#                    WORKING_DIRECTORY ${qjson_SOURCE_DIR}/lib/
#                    )

# To regenerate json_scanner.cc use:
# flex json_scanner.yy

set(qjson_MOC_HDRS
  parserrunnable.h
  serializerrunnable.h
)

IF (NOT Qt5Core_FOUND)
  qt4_wrap_cpp(qjson_MOC_SRCS ${qjson_MOC_HDRS})
ENDIF()

set (qjson_SRCS parser.cpp qobjecthelper.cpp json_scanner.cpp json_parser.cc parserrunnable.cpp serializer.cpp serializerrunnable.cpp)
set (qjson_HEADERS parser.h parserrunnable.h qobjecthelper.h serializer.h serializerrunnable.h qjson_export.h)

# Required to use the intree copy of FlexLexer.h
INCLUDE_DIRECTORIES(.)

add_gcc_compiler_cxxflags("-fexceptions")
remove_definitions(-DQT_NO_KEYWORDS)
add_library (qjson STATIC ${qjson_SRCS} ${qjson_MOC_SRCS} ${qjson_HEADERS})
