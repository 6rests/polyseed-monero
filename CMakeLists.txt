# Copyright (c) 2020-2021 tevador <tevador@gmail.com> 
# See LICENSE for licensing information

cmake_minimum_required(VERSION 3.5)

project(polyseed)

set(polyseed_sources
src/dependency.c
src/features.c
src/gf.c
src/lang.c
src/lang_cs.c
src/lang_en.c
src/lang_es.c
src/lang_fr.c
src/lang_it.c
src/lang_jp.c
src/lang_ko.c
src/lang_pt.c
src/lang_zh_s.c
src/lang_zh_t.c
src/polyseed.c
src/storage.c
src/build_wrapper.c)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif()

add_library(polyseed SHARED ${polyseed_sources})
set_property(TARGET polyseed PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET polyseed PROPERTY PUBLIC_HEADER include/polyseed.h)
include_directories(polyseed
  include/)
target_compile_definitions(polyseed PRIVATE POLYSEED_SHARED)
set_target_properties(polyseed PROPERTIES VERSION 2.1.0
                                          SOVERSION 2
                                          C_STANDARD 11
                                          C_STANDARD_REQUIRED ON)

add_library(polyseed_static STATIC ${polyseed_sources})
set_property(TARGET polyseed_static PROPERTY POSITION_INDEPENDENT_CODE ON)
include_directories(polyseed_static
  include/)
target_compile_definitions(polyseed_static PRIVATE POLYSEED_STATIC)
set_target_properties(polyseed_static PROPERTIES OUTPUT_NAME polyseed
                                                 C_STANDARD 11
                                                 C_STANDARD_REQUIRED ON)

add_executable(polyseed-tests
  tests/tests.c)
include_directories(polyseed-tests
  include/)
target_compile_definitions(polyseed-tests PRIVATE POLYSEED_STATIC)
target_link_libraries(polyseed-tests
  PRIVATE polyseed_static)

include(GNUInstallDirs)
install(TARGETS polyseed polyseed_static
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
