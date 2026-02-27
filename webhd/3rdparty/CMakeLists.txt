add_subdirectory(HDDLL)

# --- MbedTLS (TLS backend for IXWebSocket on Windows) ---
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(MBEDTLS_AS_SUBPROJECT ON)
add_subdirectory(mbedtls)

# Tell IXWebSocket that MbedTLS is already available so it skips find_package.
set(MBEDTLS_FOUND TRUE CACHE BOOL "" FORCE)
set(MBEDTLS_VERSION_GREATER_THAN_3 TRUE CACHE BOOL "" FORCE)
set(MBEDTLS_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/include" CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto CACHE STRING "" FORCE)
# Variables expected by FindMbedTLS.cmake (prevents find_library from running)
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)

# --- IXWebSocket (with TLS enabled) ---
set(USE_TLS ON CACHE BOOL "" FORCE)
set(USE_ZLIB OFF CACHE BOOL "" FORCE)
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(IXWebSocket)
# IXWebSocket skips this define when MBEDTLS_FOUND is already set, so add it
# ourselves. Without it, the MbedTLS 3.x API signatures won't match.
target_compile_definitions(ixwebsocket PRIVATE IXWEBSOCKET_USE_MBED_TLS_MIN_VERSION_3)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
add_subdirectory(nlohmann_json)
