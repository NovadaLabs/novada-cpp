include(FetchContent)

# --- GoogleTest --------------------------------------------------------------
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "novada: system GTest not found, fetching via FetchContent")
  set(INSTALL_GTEST
      OFF
      CACHE BOOL "" FORCE)
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(googletest)
else()
  message(STATUS "novada: using system GTest")
endif()

# --- cpp-httplib (test-only local mock HTTP server) --------------------------
# Header-only; not distributed via the platform package managers we target, so
# it is always fetched. It is a test-only dependency and never linked into the
# novada runtime target.
find_package(httplib QUIET)
if(NOT httplib_FOUND)
  message(STATUS "novada: fetching cpp-httplib for tests")
  set(HTTPLIB_COMPILE
      OFF
      CACHE BOOL "" FORCE)
  set(HTTPLIB_REQUIRE_OPENSSL
      OFF
      CACHE BOOL "" FORCE)
  set(HTTPLIB_REQUIRE_ZLIB
      OFF
      CACHE BOOL "" FORCE)
  set(HTTPLIB_REQUIRE_BROTLI
      OFF
      CACHE BOOL "" FORCE)
  FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(httplib)
endif()

add_executable(
  novada_tests
  support/mock_server.hpp
  detail/transport_test.cpp
  detail/envelope_test.cpp
  errors_test.cpp
  client_test.cpp
  proxy/whitelist_service_test.cpp
  proxy/account_service_test.cpp
  proxy/residential_service_test.cpp
  proxy/mobile_service_test.cpp
  proxy/rotating_service_test.cpp
  proxy/static_service_test.cpp
  proxy/unlimited_prohibit_service_test.cpp
  scraper/scraper_service_test.cpp
  scraper/api_service_test.cpp
  scraper/unblocker_service_test.cpp
  scraper/query_service_test.cpp
  wallet/wallet_service_test.cpp)

target_include_directories(novada_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(novada_tests PRIVATE novada::novada GTest::gtest GTest::gtest_main
                                            httplib::httplib)

if(WIN32)
  target_link_libraries(novada_tests PRIVATE ws2_32 crypt32)
endif()

include(GoogleTest)
gtest_discover_tests(novada_tests)
