cmake_minimum_required (VERSION 2.6)

if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -flto -Wall")
endif()

# Set up boost include and library path
find_package(Boost 1.46.0 REQUIRED COMPONENTS system filesystem)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARYDIR})

if(NOT MSVC)
    find_package(Threads)
    set(LIBS ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

file(GLOB SOURCES "src/*.cpp")

add_executable(cdb ${SOURCES})
target_link_libraries(cdb ${LIBS})
