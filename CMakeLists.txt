cmake_minimum_required(VERSION 3.20)

project(dirb
        VERSION 0.9.1
        DESCRIPTION "Fast Dir Buster Clone"
        LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(UNIX)
  set(CMAKE_CXX_FLAGS "-pthread -Wno-deprecated-declarations")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g3 -ggdb -DDEBUG -pedantic -Wall -Wextra -Wconversion -Wreorder -Wuninitialized -Wtype-limits -Wno-pragmas")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wno-deprecated")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG "/Od /Wall /Zi /wd5045 /wd5039 /wd5027 /wd4191 /wd4365 /wd4668 /wd4355 /wd4800 /wd4774 /wd4606 /wd4626 /wd4625 /wd4061 /wd4514 /wd4820 /D_CRT_SECURE_NO_WARNINGS /D__STDC_WANT_SECURE_LIB__")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /W1 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /D__STDC_WANT_SECURE_LIB__")
endif(UNIX)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-security")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -glldb")
endif()

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT -DPROJECT_NAME="${CMAKE_PROJECT_NAME}" -DPROJECT_VERSION="${CMAKE_PROJECT_VERSION}")

add_subdirectory(3rdparty/cpp-httplib)

set(OPENSSL_ROOT $ENV{OPENSSL_ROOT_DIR})
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libs: ${OPENSSL_LIBRARIES}")

set(CA_BUNDLE_CANDIDATES
  "$ENV{SSL_CERT_FILE}"
  "$ENV{SSL_CERT_DIR}/ca-certificates.crt"
  "/etc/ssl/certs/ca-certificates.crt"
  "/etc/ssl/cert.pem"
  "/etc/pki/tls/certs/ca-bundle.crt"
  "/opt/homebrew/etc/ca-certificates/cert.pem"
  "/usr/local/share/ca-certificates/cacert.pem"
  "${CMAKE_CURRENT_SOURCE_DIR}/cacert.pem"
)

set(DIRB_CA_BUNDLE "")
foreach(candidate IN LISTS CA_BUNDLE_CANDIDATES)
  if(candidate AND EXISTS "${candidate}")
    set(DIRB_CA_BUNDLE "${candidate}")
    break()
  endif()
endforeach()

if(DIRB_CA_BUNDLE)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/certs.cpp"
    COMMAND python3 -c "import pathlib, sys; data = pathlib.Path(sys.argv[1]).read_bytes(); out = pathlib.Path(sys.argv[2]); out.write_text('unsigned char cacert_pem[] = {' + ', '.join(f'0x{b:02x}' for b in data) + '};\\nunsigned int cacert_pem_len = ' + str(len(data)) + ';\\n')" "${DIRB_CA_BUNDLE}" "${CMAKE_CURRENT_BINARY_DIR}/certs.cpp"
    DEPENDS "${DIRB_CA_BUNDLE}"
    VERBATIM)
else()
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/certs.cpp"
    "unsigned char cacert_pem[] = {0};\nunsigned int cacert_pem_len = 0;\n")
endif()

add_custom_target(certs DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/certs.cpp")

add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/default_wordlist.cpp"
  COMMAND python3 -c "import pathlib, sys; data = pathlib.Path(sys.argv[1]).read_bytes(); array = ', '.join(f'0x{b:02x}' for b in data); out = pathlib.Path(sys.argv[2]); out.write_bytes(b'#include <cstddef>\\nextern const unsigned char dirb_default_wordlist[] = {' + array.encode('ascii') + b'};\\nextern const std::size_t dirb_default_wordlist_size = ' + str(len(data)).encode('ascii') + b';\\n')" "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/common.txt" "${CMAKE_CURRENT_BINARY_DIR}/default_wordlist.cpp"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/wordlists/common.txt"
  VERBATIM)
add_custom_target(default_wordlist DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/default_wordlist.cpp")

set(DIRB_SOURCES
  src/main.cpp
  src/dirb.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/certs.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/default_wordlist.cpp
)

add_executable(dirb ${DIRB_SOURCES})

target_include_directories(dirb
  PRIVATE ${PROJECT_INCLUDE_DIRS}
  ${OPENSSL_INCLUDE_DIR}
  3rdparty/cpp-httplib
  3rdparty/getopt-cpp/include
  build
)

target_link_libraries(dirb
  ${OPENSSL_LIBRARIES}
)

add_dependencies(dirb certs default_wordlist)

install(TARGETS dirb RUNTIME DESTINATION bin)
