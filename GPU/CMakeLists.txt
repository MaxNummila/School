cmake_minimum_required(VERSION 3.20)
project(sobel_cpu CXX)
set(CMAKE_CXX_STANDARD 17)

set(OpenCV_DIR "C:/opencv/opencv/build")
find_package(OpenCV REQUIRED)

add_executable(sobel_cpu sobel_CPU.cpp)
target_link_libraries(sobel_cpu PRIVATE ${OpenCV_LIBS})
