cmake_minimum_required(VERSION 3.9)

project(postly)

option(TORCH_ENABLED "A flag to control Torch build linkage" OFF)

if (TORCH_ENABLED)
    message(STATUS "Don't forget to add Torch path build option [-DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'`]")
endif()

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(POSTLY_CXX_FLAGS "-pthread;-Wall;-Wextra;-Wno-sign-compare;-fno-omit-frame-pointer;")
set(POSTLY_CXX_DEBUG_FLAGS "-fsanitize=address;")
set(POSTLY_CXX_RELEASE_FLAGS "-funroll-loops;-mtune=generic;")

add_subdirectory(lib/fasttext EXCLUDE_FROM_ALL)
add_subdirectory(lib/tinyxml2)
add_subdirectory(lib/eigen EXCLUDE_FROM_ALL)

set(BUILD_DROGON_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_CTL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_ORM OFF CACHE BOOL "" FORCE)
set(COZ_PROFILING OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/drogon EXCLUDE_FROM_ALL)
unset(COZ_PROFILIN)
unset(BUILD_ORM)
unset(BUILD_EXAMPLES)
unset(BUILD_CTL)

set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_CORE_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/rocksdb EXCLUDE_FROM_ALL)
unset(WITH_GFLAGS)
unset(WITH_TESTS)
unset(WITH_TOOLS)
unset(WITH_CORE_TOOLS)
unset(WITH_BENCHMARK_TOOLS)

set(LIB_ONLY True)
add_subdirectory(lib/onmt_tokenizer EXCLUDE_FROM_ALL)
unset(LIB_ONLY)

unset(BUILD_TESTING)

find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

if (NOT APPLE)
    find_package(OpenMP)
    if (OPENMP_FOUND OR OPENMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    endif()
endif()

find_package(Protobuf REQUIRED)
find_package(Python REQUIRED Development)

if (TORCH_ENABLED)
    find_package(Torch REQUIRED)
endif ()

set(SOURCE_FILES
    src/annotator/annotator.cpp
    src/cluster/cluster.cpp
    src/clustering/impl/single_linkage.cpp
    src/clustering/server/index.cpp
    src/clustering/clusterer.cpp
    src/controller/controller.cpp
    src/detect/detect.cpp
    src/document/document.cpp
    src/document/impl/db_document.cpp
    src/embedder/impl/ft_embedder.cpp
    src/nasty/nasty.cpp
    src/rating/rating.cpp
    src/ranker/ranker.cpp
    src/server/server.cpp
    src/summarizer/summarizer.cpp
    src/thread_pool/thread_pool.cpp
    src/utils.cpp
)

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.proto")

if (TORCH_ENABLED)
    set(LIB_LIST
        ${Boost_LIBRARIES}
        ${Protobuf_LIBRARIES}
        ${TORCH_LIBRARIES}
        tinyxml2
        OpenNMTTokenizer
        fasttext-static
        eigen
        rocksdb
        drogon
    )
else ()
set(LIB_LIST
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    tinyxml2
    OpenNMTTokenizer
    fasttext-static
    eigen
    rocksdb
    drogon
)
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/lib/rocksdb/include")
target_include_directories(fasttext-static SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/fasttext/src")
target_include_directories(OpenNMTTokenizer SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/onmt_tokenizer/include")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${PROTO_SRCS} src/main.cpp)
target_link_libraries(${PROJECT_NAME} ${LIB_LIST})
target_compile_options(${PROJECT_NAME} PUBLIC "${POSTLY_CXX_FLAGS}")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Debug>:${POSTLY_CXX_DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:${POSTLY_CXX_RELEASE_FLAGS}>")
