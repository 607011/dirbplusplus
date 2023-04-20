cmake_minimum_required(VERSION 3.16)

project(dirb
        VERSION 0.9
        DESCRIPTION "Dir Buster Clone"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

if(UNIX)
  set(CMAKE_CXX_FLAGS "-pthread -Wno-deprecated-declarations")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O0 -g3 -ggdb -DDEBUG -pedantic -Wall -Wextra -Wconversion -Wreorder -Wuninitialized -Wtype-limits -Wno-pragmas")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -Wno-deprecated")
elseif(WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG "/Od /Wall /Zi /wd5045 /wd4514 /wd4820 /D_CRT_SECURE_NO_WARNINGS /D__STDC_WANT_SECURE_LIB__")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /W1 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /D__STDC_WANT_SECURE_LIB__")
endif(UNIX)

if (CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-security")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -glldb")
endif()

add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

add_subdirectory(3rdparty/cpp-httplib)
# add_subdirectory(3rdparty/gflags)

set(OPENSSL_ROOT $ENV{OPENSSL_ROOT_DIR})
find_package(OpenSSL)
message(STATUS "OpenSSL version: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libs: ${OPENSSL_LIBRARIES}")

add_executable(dirb
  src/main.cpp
  src/dirb.cpp
)

target_include_directories(dirb
	PRIVATE ${PROJECT_INCLUDE_DIRS}
  # ${GFLAGS_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  3rdparty/cpp-httplib
)

target_link_libraries(dirb
  # gflags::gflags
  ${OPENSSL_LIBRARIES}
)

install(TARGETS dirb RUNTIME DESTINATION bin)
