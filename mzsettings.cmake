set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

set(LIBRARY_TYPE SHARED)

if(APPLE)
  include_directories(/opt/local/include) # MacPorts
  link_directories(/opt/local/lib)
  find_library(OPENGL_LIBRARY OpenGL)
else()
  find_library(OPENGL_LIBRARY GL)
  find_library(GLU_LIBRARY GLU)
  set(OPENGL_LIBRARY ${OPENGL_LIBRARY} ${GLU_LIBRARY})
endif()

include(FindPkgConfig)

find_library(PNG_LIBRARY png)
find_library(EXPAT_LIBRARY expat)
find_library(GLUT_LIBRARY glut)

#use this to find the FindNLopt.cmake file.
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")
find_package(NLopt)
if (NLOPT_FOUND)
    include_directories(${NLOPT_INCLUDE_DIRS})
endif()

pkg_search_module(EIGEN3 REQUIRED eigen3>=3)
pkg_search_module(CAIRO cairo)

if (CAIRO_FOUND)
add_definitions(-DMZ_HAVE_CAIRO)
endif ()

include_directories(${EIGEN3_INCLUDE_DIRS})

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions( -DDEBUG )
else()
    add_definitions( -DRELEASE )
endif()

add_definitions( -DDO_TIMING )

set(CMAKE_C_FLAGS "-Wall -g -fPIC")
set(CMAKE_CXX_FLAGS "-Wall -g -fPIC")

if(APPLE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -Wno-deprecated")
  set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -Wsign-compare -Wno-deprecated")
endif()

set(CMAKE_C_FLAGS_DEBUG "-O")
set(CMAKE_CXX_FLAGS_DEBUG "-O -pg")

set(CMAKE_C_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O3 -pg")

macro(add_gui_app name)
  if(APPLE)
    add_executable(${name} MACOSX_BUNDLE ${ARGN})
  else()
    add_executable(${name} ${ARGN})
  endif()
endmacro(add_gui_app)

include_directories(${PROJECT_SOURCE_DIR})
