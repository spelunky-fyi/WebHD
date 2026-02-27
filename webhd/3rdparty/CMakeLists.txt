add_subdirectory(HDDLL)

set(USE_TLS OFF CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(IXWebSocket)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(nlohmann_json)
