cmake_minimum_required(VERSION 3.9)

project(driver)

include(../config.cmake)

option(TORCH_ENABLED "A flag to control Torch build linkage" OFF)

if (TORCH_ENABLED)
    message(STATUS "Don't forget to add Torch path build option [-DCMAKE_PREFIX_PATH=`python3 -c 'import torch;print(torch.utils.cmake_prefix_path)'`]")
endif()

add_subdirectory(../lib/fasttext EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/fasttext_build)
add_subdirectory(../lib/tinyxml2 ${CMAKE_CURRENT_BINARY_DIR}/xml_build)
add_subdirectory(../lib/eigen EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/eigen_build)
add_subdirectory(../lib/drogon EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/drogon_build)
add_subdirectory(../lib/rocksdb EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/rocksdb_build)
add_subdirectory(../lib/onmt_tokenizer EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/tokenizer_build)

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
    annotator/annotator.cpp
    cluster/cluster.cpp
    clustering/impl/single_linkage.cpp
    clustering/server/index.cpp
    clustering/clusterer.cpp
    controller/controller.cpp
    detect/detect.cpp
    document/document.cpp
    document/impl/db_document.cpp
    embedder/impl/ft_embedder.cpp
    nasty/nasty.cpp
    rating/rating.cpp
    ranker/ranker.cpp
    server/server.cpp
    summarizer/summarizer.cpp
    thread_pool/thread_pool.cpp
    utils.cpp
)

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto")

if(TORCH_ENABLED)
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
else()
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

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../lib")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../lib/rocksdb/include")
target_include_directories(fasttext-static SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/fasttext/src")
target_include_directories(OpenNMTTokenizer SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../lib/onmt_tokenizer/include")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${PROTO_SRCS} main.cpp)
target_link_libraries(${PROJECT_NAME} ${LIB_LIST})

target_compile_options(${PROJECT_NAME} PUBLIC "${POSTLY_CXX_FLAGS}")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Debug>:${POSTLY_CXX_DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:${POSTLY_CXX_RELEASE_FLAGS}>")
