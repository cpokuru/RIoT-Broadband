cmake_minimum_required (VERSION 2.8.11)
set(CMAKE_VERBOSE_MAKEFILE ON)

project (RIoT-Broadband)

find_path(RTMESSAGE_API_HEADER NAMES rtMessage.h REQUIRED HINTS ${RBUS_INSTALL_DIR}/usr/include PATH_SUFFIXES rtmessage)

set(RTMESSAGE_API_HEADERS "")
if (NOT ${RTMESSAGE_API_HEADER} STREQUAL "RTMESSAGE_API_HEADER-NOTFOUND")
        message("Found rtMessage.h ${RTMESSAGE_API_HEADER}")
        set(RTMESSAGE_API_HEADERS ${RTMESSAGE_API_HEADER})

endif()

set(RTMESSAGE_LIBRARIES "")

find_library(RTMESSAGE_LIBRARY REQUIRED HINTS ${RBUS_INSTALL_DIR}/usr/lib NAMES rtMessage)
if (NOT ${RTMESSAGE_LIBRARY} STREQUAL "RTMESSAGE_LIBRARY-NOTFOUND")
        set(RTMESSAGE_LIBRARIES ${RTMESSAGE_LIBRARY})
endif()
add_subdirectory(riotInterface)
