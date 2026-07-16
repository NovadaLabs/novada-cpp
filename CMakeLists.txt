cmake_minimum_required(VERSION 3.16)

project(
  novada
  VERSION 0.1.0
  DESCRIPTION "C++ SDK for the Novada API (proxy / scraper / wallet)"
  HOMEPAGE_URL "https://github.com/NovadaLabs/novada-cpp"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(NOVADA_BUILD_TESTS "Build the novada test suite" ${PROJECT_IS_TOP_LEVEL})
option(NOVADA_BUILD_EXAMPLES "Build the novada examples" ${PROJECT_IS_TOP_LEVEL})
option(NOVADA_ENABLE_SANITIZERS "Build with -fsanitize=address,undefined" OFF)
option(NOVADA_INSTALL "Generate the novada install/export rules"
       ${PROJECT_IS_TOP_LEVEL})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Dependencies)

add_library(
  novada
  src/client.cpp
  src/errors.cpp
  src/detail/envelope.cpp
  src/detail/transport.cpp
  src/proxy/types.cpp
  src/proxy/whitelist_service.cpp
  src/proxy/account_service.cpp
  src/proxy/residential_service.cpp
  src/proxy/mobile_service.cpp
  src/proxy/rotating_service.cpp
  src/proxy/static_isp_service.cpp
  src/proxy/dedicated_dc_service.cpp
  src/proxy/unlimited_service.cpp
  src/proxy/prohibit_domain_service.cpp
  src/scraper/scraper_service.cpp
  src/scraper/scrape_exec.cpp
  src/scraper/types.cpp
  src/scraper/youtube_service.cpp
  src/scraper/google_service.cpp
  src/scraper/unblocker_service.cpp
  src/scraper/universal_service.cpp
  src/scraper/browser_service.cpp
  src/wallet/wallet_service.cpp)
add_library(novada::novada ALIAS novada)

target_include_directories(
  novada PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
         PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(novada PUBLIC nlohmann_json::nlohmann_json
                                     PRIVATE CURL::libcurl)

target_compile_features(novada PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(novada PRIVATE /W4)
  target_compile_definitions(novada PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX
                                             WIN32_LEAN_AND_MEAN)
else()
  target_compile_options(novada PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NOVADA_ENABLE_SANITIZERS)
  target_compile_options(novada PUBLIC -fsanitize=address,undefined
                                        -fno-sanitize-recover=all)
  target_link_options(novada PUBLIC -fsanitize=address,undefined)
endif()

if(WIN32)
  target_link_libraries(novada PRIVATE ws2_32 crypt32)
endif()

if(NOVADA_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(NOVADA_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# --- Install / export --------------------------------------------------------
if(NOVADA_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  set_target_properties(novada PROPERTIES VERSION ${PROJECT_VERSION}
                                          SOVERSION ${PROJECT_VERSION_MAJOR})

  install(
    TARGETS novada
    EXPORT novada-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  set(NOVADA_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/novada)

  install(
    EXPORT novada-targets
    FILE novada-targets.cmake
    NAMESPACE novada::
    DESTINATION ${NOVADA_CONFIG_INSTALL_DIR})

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/novada-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/novada-config.cmake
    INSTALL_DESTINATION ${NOVADA_CONFIG_INSTALL_DIR})

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/novada-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/novada-config.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/novada-config-version.cmake
          DESTINATION ${NOVADA_CONFIG_INSTALL_DIR})
endif()
