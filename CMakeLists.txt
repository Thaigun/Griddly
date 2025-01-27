cmake_minimum_required(VERSION 3.10.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
project(Griddly VERSION 1.3.0)

set(BINARY ${CMAKE_PROJECT_NAME})

set(BIN_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)

if(MSVC)
message("Compiling with MSVC")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR} )
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
elseif(MINGW)
    message("Compiling with Mingw.")
    add_compile_options("-Wa,-mbig-obj")
endif()



set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})

# use position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
# Require (at least) it
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Don't use e.g. GNU extension (like -std=gnu++11) for portability
set(CMAKE_CXX_EXTENSIONS OFF)
# Relevant cmake files are in this folder
set(CMAKE_CONFIG_FOLDER ${CMAKE_SOURCE_DIR}/cmake)

# GLM
add_subdirectory("libs/glm")

# PyBind
add_subdirectory("libs/pybind11")

# Yaml-Cpp
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "disable yaml tests")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "disable yaml tools")
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "disable yaml contrib")
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "disable shared libs")
add_subdirectory("libs/yaml-cpp")

#enable clang-format and clang-tidy project wide
include(${CMAKE_CONFIG_FOLDER}/settings/Clang-cxx-dev-tools.cmake)

# standard compiler warnings
add_library(project_warnings INTERFACE)
include(${CMAKE_CONFIG_FOLDER}/settings/CompilerWarnings.cmake)
set_project_warnings(project_warnings)


find_package(Vulkan REQUIRED FATAL_ERROR)
set(VULKAN_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Griddly/Core/Observers/Vulkan/resources/shaders)

file(GLOB_RECURSE GRIDDLY_SOURCES "src/*.cpp")

set (GRIDDLY_INCLUDE_DIRS "")
foreach (_headerFile ${GRIDDLY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND GRIDDLY_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES GRIDDLY_INCLUDE_DIRS)

message(STATUS "Compiling shaders...")

# Compile shaders and copy them into resources directory in build output

if(MSVC)
    execute_process ( COMMAND ${VULKAN_SHADER_DIR}/compile_shaders.bat  
                    WORKING_DIRECTORY ${VULKAN_SHADER_DIR} 
                    RESULT_VARIABLE rv)
else()
    execute_process ( COMMAND bash ${VULKAN_SHADER_DIR}/compile_shaders.sh  
                    WORKING_DIRECTORY ${VULKAN_SHADER_DIR} 
                    RESULT_VARIABLE rv)
endif()

# Add the spdlog libraries to the build
set(SPDLOG_DIR "libs/spdlog")
include_directories(${SPDLOG_DIR})

# Add the stb libraries to the build
set(STB_DIR "libs/stb")
include_directories(${STB_DIR})

add_library(${BINARY} STATIC ${GRIDDLY_SOURCES})
target_link_libraries(${BINARY} PRIVATE project_warnings Vulkan::Vulkan yaml-cpp glm)

# Add the pybind11 module
set(PYTHON_MODULE python_griddly)
pybind11_add_module(${PYTHON_MODULE} bindings/python.cpp)

# Want the python lib to be output in the same directory as the other dll/so
if(MSVC)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set_target_properties(${PYTHON_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${BIN_OUTPUT_DIR})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
endif()

set_target_properties(${PYTHON_MODULE} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${BIN_OUTPUT_DIR})
target_link_libraries(${PYTHON_MODULE} PRIVATE ${BINARY} project_warnings Vulkan::Vulkan yaml-cpp glm )

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# Testing stuff
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
