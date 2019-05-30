cmake_minimum_required(VERSION 3.3)
project(PersistTreeTest)

set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)
# it will cause DSO link error if don't define this
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -lpmem)

#set(ENV{CZL_DEBUG})
#set(ENV{ENERY_1G_PRINT})
#set(ENV{DRAM_CANCLE_LOG})
#set(ENV{CAL_ACCESS_COUNT})

add_definitions(-DCZL_DEBUG)
add_definitions(-DEVERY_1G_PRINT)
add_definitions(-DDRAM_CANCLE_LOG)
add_definitions(-DCAL_ACCESS_COUNT)

set(src
        flush_nvm.cc
        log_batch.cc
        log_batch_wrapper.cc
        persistent_skip_list.cc
        persistent_skiplist_no_transaction.cc
        persistent_skiplist_wrapper.cc
        random.cc
        statistic.cc statistics_test.cc)


include(FindThreads)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBPMEM REQUIRED libpmem)
    pkg_check_modules(LIBPMEM REQUIRED libpmemobj++)
else()
    find_package(LIBPMEM REQUIRED)
    find_package(LIBPMEMOBJ++ REQUIRED)
endif()

link_directories(${LIBPMEM_LIBRARY_DIRS})
link_directories(${LIBPMEMOBJ++_LIBRARY_DIRS})
link_directories(/usr/local/lib)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(/usr/local/include)

#aux_source_directory(. src)

add_executable(PersistTreeTest motivation1_main.cc ${src})
#add_executable(StatsTest statistics_test.cc statistic.cc)

target_include_directories(PersistTreeTest PUBLIC ${LIBPMEM_INCLUDE_DIRS} . ..)
target_link_libraries(PersistTreeTest ${LIBPMEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(PersistTreeTest PUBLIC ${LIBPMEMOBJ++_INCLUDE_DIRS} . ..)
target_link_libraries(PersistTreeTest ${LIBPMEMOBJ++_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(PersistTreeTest cityhash)