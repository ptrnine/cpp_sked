find_package(Threads REQUIRED)

add_executable(tests tests.cpp)

target_link_libraries(tests Threads::Threads Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE "${CMAKE_SOURCE_DIR}/src")
