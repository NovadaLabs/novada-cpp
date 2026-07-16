include(FetchContent)

# --- CURL (HTTP transport) --------------------------------------------------
# Prefer a system/package-manager install; fall back to building from source
# when find_package cannot locate one (e.g. a bare CI image).
find_package(CURL QUIET)
if(NOT CURL_FOUND)
  message(STATUS "novada: system CURL not found, fetching via FetchContent")
  set(BUILD_CURL_EXE
      OFF
      CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS
      OFF
      CACHE BOOL "" FORCE)
  set(CURL_USE_SCHANNEL
      ${WIN32}
      CACHE BOOL "" FORCE)
  set(CURL_USE_OPENSSL
      $<NOT:${WIN32}>
      CACHE BOOL "" FORCE)
  FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG curl-8_10_1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(curl)
  if(NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl ALIAS libcurl)
  endif()
else()
  message(STATUS "novada: using system CURL ${CURL_VERSION_STRING}")
endif()

# --- nlohmann/json (JSON encode/decode) -------------------------------------
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  message(STATUS "novada: system nlohmann_json not found, fetching via FetchContent")
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(nlohmann_json)
else()
  message(STATUS "novada: using system nlohmann_json ${nlohmann_json_VERSION}")
endif()
