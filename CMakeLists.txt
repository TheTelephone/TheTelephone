cmake_minimum_required (VERSION 2.8)
project(TheTelephone)

#Set prefix and suffix by OS
cmake_policy(SET CMP0037 OLD)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(CMAKE_SHARED_LIBRARY_PREFIX_C "")
  set(CMAKE_SHARED_LIBRARY_SUFFIX_C ".pd_linux")
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  set(CMAKE_SHARED_LIBRARY_PREFIX_C "")
  set(CMAKE_SHARED_LIBRARY_SUFFIX_C ".pd_linux")
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  set(CMAKE_SHARED_LIBRARY_PREFIX_C "")
  set(CMAKE_SHARED_LIBRARY_SUFFIX_C ".pd_darwin")
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  set(CMAKE_SHARED_LIBRARY_PREFIX_C "")
  set(CMAKE_SHARED_LIBRARY_SUFFIX_C ".dll")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#Setting default installation target
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/pd-externals")
  endif()
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Library/Pd")
  endif()
  if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_INSTALL_PREFIX "$ENV{AppData}/Pd") #ATTENTION: untested
  endif()

  message("Setting install directory to ${CMAKE_INSTALL_PREFIX} - to override #set CMAKE_INSTALL_PREFIX.")
endif()

#Installation
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY pd-help/ DESTINATION ${CMAKE_INSTALL_PREFIX})

#Dependencies
INCLUDE (CheckLibraryExists)
set(CMAKE_REQUIRED_LIBRARIES fftw3 gsm json m opus sndfile speex websockets)
find_library(HAVE_FFTW fftw3)
find_library(HAVE_GSM gsm)
find_library(HAVE_OPUS opus)
find_library(HAVE_SNDFILE sndfile)
find_library(HAVE_SPEEX speex)
find_library(HAVE_JSON json-c)
find_library(HAVE_WEBSOCKETS websockets)

INCLUDE (CheckIncludeFiles)
CHECK_INCLUDE_FILES(m_pd.h HAVE_PD_H)
if(NOT HAVE_PD_H)
  message(FATAL_ERROR "PureData's header file <<m_pd.h>> could not be found: cannot build.")
endif()

#Source folders
add_subdirectory(src/support)
add_subdirectory(src/degradations)
add_subdirectory(src/connectivity)

include_directories(src/include)

#PD-External: basics
add_library(audiorouting~ SHARED src/support/audiorouting_tilde.c)
target_link_libraries(audiorouting~ m)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT ${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  message(WARNING "CLOCK_MONOTONIC not available (Linux/FreeBSD only): benchmark_delay~ will not be build.")
elseif()
  add_library(benchmark_delay~ SHARED src/support/benchmark_delay_tilde.c)
  target_link_libraries(benchmark_delay~ m)
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(WARNING "sys_exit() not available (Linux only): exit will not be build.")
elseif()
  add_library(exit SHARED src/support/exit.c)
endif()

add_library(delay~ SHARED src/degradations/delay_tilde.c)

#PD-External: external with CPP
add_library(cpp_example~ SHARED src/programming/cpp_example_tilde.cpp)
set_target_properties(cpp_example~ PROPERTIES PREFIX "")
set_target_properties(cpp_example~ PROPERTIES SUFFIX ".pd_linux")

#PD-External: I/O
if (NOT HAVE_SNDFILE)
  message(WARNING "libsndfile not found: readsfnow~ and writesfnow~ will not be build." )
else()
  add_library(readsfnow~ SHARED src/support/readsfnow_tilde.c)
  target_link_libraries(readsfnow~ sndfile)

  add_library(writesfnow~ SHARED src/support/writesfnow_tilde.c)
  target_link_libraries(writesfnow~ sndfile)
endif()

#PD-External: convolution
if(NOT HAVE_FFTW AND NOT HAVE_SNDFILE)
	message(WARNING "libfftw3 or libsndfile not found: convolve_dynamic~ will not be build.")
else()
	add_library(convolve_dynamic~ SHARED src/signal-processing/convolve_dynamic_tilde.c)
	target_link_libraries(convolve_dynamic~ resample m fftw3 sndfile)
endif()

#PD-External: connectivity
if(NOT HAVE_JSON OR NOT HAVE_WEBSOCKETS)
	message(WARNING "libjson-c or libwebsockets not found: websocket_recv_client and websocket_recv_server will not be build.")
else()
	add_library(websocket_recv_client SHARED src/connectivity/websocket_recv_client.c)
	target_link_libraries(websocket_recv_client websockets json-c)

	add_library(websocket_recv_server SHARED src/connectivity/websocket_recv_server.c)
	target_link_libraries(websocket_recv_server websockets json-c)
endif()

#PD-External: G.711
include_directories(third-party/itu-t_stl2009_g711)
file(GLOB G711_SRC third-party/itu-t_stl2009_g711/*)
add_library(g711~ SHARED src/degradations/g711_tilde.c ${G711_SRC})
target_link_libraries(g711~ resample m)

#PD-External: G.722
include_directories(third-party/spanddsp_g722)
file(GLOB G722_SRC third-party/spanddsp_g722/*)
add_library(g722~ SHARED src/degradations/g722_tilde.c  ${G722_SRC})
target_link_libraries(g722~ resample m)

#PD-External: GSM
if(NOT HAVE_GSM)
  message(WARNING "libgsm not found: gsm~ will not be build.")
else()
  add_library(gsm~ SHARED src/degradations/gsm_tilde.c)
  target_link_libraries(gsm~ resample m gsm)
endif()

#PD-External: LPC-10
include_directories(third-party/lpc10)
file(GLOB LPC10_SRC third-party/lpc10/*)
add_library(lpc10~ SHARED src/degradations/lpc10_tilde.c ${LPC10_SRC})
target_link_libraries(lpc10~ resample m)

#PD-External: MNRU
include_directories(third-party/itu-t_stl2009_mnru)
file(GLOB MNRU_SRC third-party/itu-t_stl2009_mnru/*)
add_library(mnru~ SHARED src/degradations/mnru_tilde.c ${MNRU_SRC})
target_link_libraries(mnru~ resample m)

#PD-External: OPUS
if(NOT HAVE_OPUS)
  message(WARNING "libopus not found: opus~ will not be build.")
else()
  add_library(opus~ SHARED src/degradations/opus_tilde.c)
  target_link_libraries(opus~ resample opus)
endif()

#PD-External: SPEEX
if(NOT HAVE_SPEEX)
  message(WARNING "libspeex not found: speex~ will not be build.")
else()
  add_library(vad_speex~ SHARED src/signal-processing/vad_speex_tilde.c)
  target_link_libraries(vad_speex~ resample speexdsp)

  add_library(speex~ SHARED src/degradations/speex_tilde.c)
  target_link_libraries(speex~ resample speex)
endif()


#TESTING
enable_testing()
add_subdirectory(tests)
add_custom_target(test-verbose COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

##TESTING: unresolved symbols
add_executable(test_undefined_symbols EXCLUDE_FROM_ALL tests/test_undefined_symbols.c)
target_link_libraries(test_undefined_symbols dl ${CMAKE_REQUIRED_LIBRARIES})

#Compiler-specific settings
if ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set_property(TARGET ${target_name} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-export-dynamic")

  set(CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS} -undefined dynamic_lookup")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  set_property(TARGET ${target_name} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--export-dynamic")
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
  message(FATAL_ERROR "For compiler <<Intel>> the flag for exporting symbols is not set." )
elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  message(FATAL_ERROR "For compiler <<MSVC>> the flag for exporting symbols is not set." )
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set_property(TARGET ${target_name} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-export-dynamic")

  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_property(TARGET ${target_name} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,--export-dynamic")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  message(FATAL_ERROR "For compiler <<Intel>> the flag for exporting symbols is not set." )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  message(FATAL_ERROR "For compiler <<MSVC>> the flag for exporting symbols is not set." )
endif()

file(GLOB PUREDATA_EXTERNALS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*)
add_test(test_undefined_symbols ./test_undefined_symbols ${PUREDATA_EXTERNALS})

#DOCUMENTATION
find_package(Doxygen)
if(DOXYGEN_FOUND)
#  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxyfile @ONLY)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with Doxygen")
endif(DOXYGEN_FOUND)