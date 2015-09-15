cmake_minimum_required(VERSION 2.8.4)

project(colorspace_converter)
file (GLOB HEADERS "*.hpp")
file (GLOB SOURCES "*.cpp")

if (MSVC)
  if (MSVC_VERSION LESS 1800)
    # C99 not supported at all
    set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)
  endif ()
  set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS /arch:SSE2)
  set (DEBUG_FLAGS   "/Od /Ob0 /MTd /Zi")
  set (RELEASE_FLAGS "/O2 /MT /D NDEBUG")
else (MSVC)
  if (APPLE)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
  else ()
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
  endif ()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set (COMMON_FLAGS "-Wall -msse2")
  set (DEBUG_FLAGS "-g -O0")
  set (RELEASE_FLAGS "-O2 -DNDEBUG")
endif (MSVC)

set (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")

set (CMAKE_C_FLAGS_DEBUG   "${DEBUG_FLAGS}")
set (CMAKE_CXX_FLAGS_DEBUG "${DEBUG_FLAGS}")

set (CMAKE_C_FLAGS_RELEASE   "${RELEASE_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "${RELEASE_FLAGS}")

add_executable(colorspace_converter ${SOURCES} ${HEADERS})
