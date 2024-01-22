cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

project(corahbot)

file(GLOB_RECURSE SOURCE "src/*.cpp" "src/*.hpp")

add_executable(${PROJECT_NAME} ${SOURCE})

set(GLBASE_BUILD_TESTS OFF)

add_subdirectory(deps/glbase)

find_package(RapidJSON CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
  glbase
  ${OpenCV_LIBS}
  rapidjson
  Psapi
)

target_include_directories(${PROJECT_NAME} PRIVATE
  deps/glbase/src
  deps/fontawesome
  ${OpenCV_INCLUDE_DIRS}
)

set(DATA_DESTINATION ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/data)

# postbuild command to copy the data folder
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/data ${DATA_DESTINATION}
)

add_custom_target(
  copy_data_folder ALL
  DEPENDS ${DATA_DESTINATION}
)