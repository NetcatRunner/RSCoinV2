cmake_minimum_required(VERSION 3.28)

project(RSCoin2
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_subdirectory(vendor/RSTools)
add_subdirectory(vendor/nlohmann)
add_subdirectory(vendor/secp256k1)
add_subdirectory(vendor/cpp-httplib)

set(RSCOIN_WARNINGS
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wcast-align
    -Wvla
    -Wnon-virtual-dtor
)

file(GLOB_RECURSE LIB_SOURCES
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)
set(ENTRY_POINTS
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${PROJECT_SOURCE_DIR}/src/wallet/cli/main.cpp
)
list(REMOVE_ITEM LIB_SOURCES ${ENTRY_POINTS})

add_library(rscoin_core STATIC ${LIB_SOURCES})
target_include_directories(rscoin_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(rscoin_core
    PUBLIC
        RST
        nlohmann_json::nlohmann_json
        secp256k1
        httplib
)
target_compile_options(rscoin_core PRIVATE ${RSCOIN_WARNINGS})

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE rscoin_core)
target_compile_options(${PROJECT_NAME} PRIVATE ${RSCOIN_WARNINGS})

add_executable(rscoin-wallet src/wallet/cli/main.cpp)
target_link_libraries(rscoin-wallet PRIVATE rscoin_core)
target_compile_options(rscoin-wallet PRIVATE ${RSCOIN_WARNINGS})
