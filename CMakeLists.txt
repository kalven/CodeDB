cmake_minimum_required (VERSION 2.6)

option(USE_RE2 "Use Google's re2 regular expression library." ON)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
endif()

# Set up boost include and library path
find_package(Boost 1.46.0 REQUIRED COMPONENTS system filesystem thread)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARYDIR})

if(NOT MSVC)
    find_package(Threads)
    set(LIBS ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

# Set up re2
if(WIN32)
  # re2 is not available on windows
  set(USE_RE2 OFF)
endif()

if(USE_RE2)

  find_path    (RE2_INC re2.h   PATHS /usr/local/include /opt/local/include /usr/include PATH_SUFFIXES re2)
  find_library (RE2_LIB re2     PATHS /usr/local/lib /opt/local/lib /usr/lib)

  set(LIBS ${LIBS} ${RE2_LIB})

  include_directories (${RE2_INC})

  add_definitions("-DCDB_USE_RE2")
endif()

file(GLOB SOURCES "src/*.cpp")

file(GLOB SNAPPY_SRC "ext/snappy/*.cc")
include_directories("ext/snappy")

add_executable(cdb ${SOURCES} ${SNAPPY_SRC})
target_link_libraries(cdb ${LIBS})
