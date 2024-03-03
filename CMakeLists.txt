cmake_minimum_required(VERSION 3.20...3.27)

project(mre)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost COMPONENTS serialization REQUIRED)

add_executable(mre main.cpp)
target_link_libraries(mre PRIVATE Boost::serialization)

configure_file("${CMAKE_SOURCE_DIR}/Dockerfile.in" "${CMAKE_BINARY_DIR}/Dockerfile")

add_custom_target(docker
    COMMAND docker build -t twinkle-docker -f "${CMAKE_BINARY_DIR}/Dockerfile" .
    COMMAND docker run --rm -v "${CMAKE_BINARY_DIR}/archives:/archives" twinkle-docker
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Change this line to use the source directory as the context
)


