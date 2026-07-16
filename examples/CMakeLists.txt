cmake_minimum_required(VERSION 3.16)

# Each example is a standalone executable linking the novada library. They read
# NOVADA_API_KEY from the environment and make live API calls, so they are built
# but not run as part of the test suite.
foreach(example proxy scraper wallet)
  add_executable(novada_example_${example} ${example}.cpp)
  target_link_libraries(novada_example_${example} PRIVATE novada::novada)
  set_target_properties(novada_example_${example} PROPERTIES OUTPUT_NAME ${example})
endforeach()
