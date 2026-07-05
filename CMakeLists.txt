cmake_minimum_required(VERSION 3.25.0)

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT VCPKG_TARGET_TRIPLET MATCHES "$ENV{VCPKG_DEFAULT_TRIPLET}")
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "" FORCE)
endif()

project(Clock LANGUAGES C CXX)
include(GNUInstallDirs)

include("${CMAKE_SOURCE_DIR}/cmake/include/init_runtime_dependencies_directories.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/include/flags.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/include/clang_tidy.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/include/boost.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/include/icu.cmake")

include_directories("${CMAKE_SOURCE_DIR}")

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/external/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/external/include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")
add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang|GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -municode -mwindows)
  target_link_options(${PROJECT_NAME} PRIVATE -municode -mwindows)
endIf()
target_link_libraries(${PROJECT_NAME} comctl32 oleacc ${EXTRA_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
if(ICU_FOUND)
  target_link_libraries(${PROJECT_NAME} ${ICU_LIBRARIES})
endif()
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

install(CODE "
  set(RUNTIME_DEPENDENCIES_DIRECTORIES [[${RUNTIME_DEPENDENCIES_DIRECTORIES}]])
  set(TARGET_FILE [[$<TARGET_FILE:${PROJECT_NAME}>]])
  include([[${CMAKE_SOURCE_DIR}/cmake/include/install_dependencies.cmake]])
")
