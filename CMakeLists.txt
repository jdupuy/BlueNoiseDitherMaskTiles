cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# disable GLFW docs, examples and tests
# see http://www.glfw.org/docs/latest/build_guide.html
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# set path to dependencies
add_subdirectory(submodules/glfw)
include_directories(submodules/glfw/include)
include_directories(submodules/stb)
include_directories(submodules/dj_opengl)

# ------------------------------------------------------------------------------
project (dither)

# compiler specific options
if(MSVC OR MSVC_IDE)
	add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif()

# ------------------------------------------------------------------------------
set(SRC_DIR src)
include_directories(${SRC_DIR})
add_executable(dither ${SRC_DIR}/dither.cpp ${SRC_DIR}/glad/glad.c)
target_link_libraries(dither glfw)
target_compile_definitions(dither PUBLIC -DPATH_TO_SRC_DIRECTORY="${CMAKE_SOURCE_DIR}/${SRC_DIR}/")
unset(SRC_FILES)

