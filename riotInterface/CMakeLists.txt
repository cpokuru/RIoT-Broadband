set (RIOT_DAEMON RIoTInterface)

add_executable(${RIOT_DAEMON} RIoTInterface.cpp)
set_source_files_properties(RIoTInterface.cpp PROPERTIES COMPILE_FLAGS "-fpermissive")

target_include_directories(${RIOT_DAEMON} PRIVATE  ${RTMESSAGE_API_HEADERS})
target_link_libraries(${RIOT_DAEMON} PRIVATE  ${RTMESSAGE_LIBRARIES} pthread)

install(TARGETS ${RIOT_DAEMON}  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDR})
