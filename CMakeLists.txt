cmake_minimum_required(VERSION 3.3)
project(PersistTreeTest)

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)

include(FindThreads)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBPMEM REQUIRED libpmem)
else()
    find_package(LIBPMEM REQUIRED)
endif()

link_directories(${LIBPMEM_LIBRARY_DIRS})

include_directories(${PROJECT_SOURCE_DIR})

add_executable(PersistTreeTest motivation1_main.cc)

target_include_directories(PersistTreeTest PUBLIC ${LIBPMEM_INCLUDE_DIRS} . ..)
target_link_libraries(PersistTreeTest ${LIBPMEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
