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

#Default installation target is: ~/pd-externals
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/pd-externals")
  message("Setting install directory to ${CMAKE_INSTALL_PREFIX} - to override #set CMAKE_INSTALL_PREFIX.")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/)

#Dependencies
INCLUDE (CheckLibraryExists)
set(CMAKE_REQUIRED_LIBRARIES fftw3 json m sndfile websockets)
CHECK_LIBRARY_EXISTS(fftw3 fftw_execute "" HAVE_FFTW)
CHECK_LIBRARY_EXISTS(sndfile sf_open "" HAVE_SNDFILE)
CHECK_LIBRARY_EXISTS(json json_tokener_parse "" HAVE_JSON)
CHECK_LIBRARY_EXISTS(websockets lws_client_connect "" HAVE_WEBSOCKETS)

#Source folders
add_subdirectory(src/support)
add_subdirectory(src/degradations)
add_subdirectory(src/connectivity)

include_directories(src/include)


#PD-External: basics
add_library(audiorouting~ SHARED src/support/audiorouting_tilde.c)

add_library(exit SHARED src/support/exit.c)

add_library(delay~ SHARED src/degradations/delay_tilde.c)

#PD-External: external with CPP
add_library(cpp_example~ SHARED src/programming/cpp_example_tilde.cpp)
set_target_properties(cpp_example~ PROPERTIES PREFIX "")
set_target_properties(cpp_example~ PROPERTIES SUFFIX ".pd_linux")

#PD-External: I/O
if (NOT HAVE_SNDFILE)
  message(FATAL_ERROR "libsndfile not found." )
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
	message(WARNING "libjson or libwebsockets not found: websocket_recv will not be build.")
else()
	add_library(websocket_recv SHARED src/connectivity/websocket_recv.c)
	target_link_libraries(websocket_recv websockets json)
endif()

#Installation
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/)
install(DIRECTORY pd-help/ DESTINATION ${CMAKE_INSTALL_PREFIX}/)
