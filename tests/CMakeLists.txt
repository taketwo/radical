find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)

get_filename_component(TEST_DATA_DIR "data" ABSOLUTE)
configure_file(test.h.in test.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(tests "${CMAKE_CTEST_COMMAND}" "-V" VERBATIM)

macro(TEST_ADD _name)
  set(options)
  set(one_value_args)
  set(multi_value_args LINK_WITH)
  cmake_parse_arguments(TEST_ADD "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  set(_executable test_${_name})
  add_executable(${_executable} ${_executable}.cpp)
  target_compile_definitions(${_executable}
    PRIVATE "-DBOOST_TEST_MODULE=${_name}"
    PRIVATE "-DBOOST_TEST_DYN_LINK"
  )
  target_link_libraries(${_executable}
    ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${TEST_ADD_LINK_WITH}
  )
  add_test(NAME ${_name} COMMAND ${_executable})
  add_dependencies(tests ${_executable})
endmacro(TEST_ADD)

TEST_ADD(radiometric_response LINK_WITH radical)
TEST_ADD(vignetting_response LINK_WITH radical)
TEST_ADD(nonparametric_vignetting_model LINK_WITH radical)
TEST_ADD(polynomial_vignetting_model LINK_WITH radical)
