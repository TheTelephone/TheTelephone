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
set(CMAKE_REQUIRED_LIBRARIES sndfile)
CHECK_LIBRARY_EXISTS(sndfile sf_open "" HAVE_SNDFILE)

#Source folders
add_subdirectory(src/support)

#PD-External: basics
add_library(audiorouting~ SHARED src/support/audiorouting_tilde.c)

add_library(exit SHARED src/support/exit.c)

#PD-External: I/O
if (NOT HAVE_SNDFILE)
  message(FATAL_ERROR "libsndfile not found." )
else()
  add_library(readsfnow~ SHARED src/support/readsfnow_tilde.c)
  target_link_libraries(readsfnow~ sndfile resample)

  add_library(writesfnow~ SHARED src/support/writesfnow_tilde.c)
  target_link_libraries(writesfnow~ sndfile)
endif()

#Installation
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/)
install(DIRECTORY pd-help/ DESTINATION ${CMAKE_INSTALL_PREFIX}/)
