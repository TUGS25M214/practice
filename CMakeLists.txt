cmake_minimum_required(VERSION 3.25)
project(PRACTICE LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(cv_test cv_test.cpp jpgheaders.cpp)
target_include_directories(cv_test PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cv_test ${OpenCV_LIBS})