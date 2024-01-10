cmake_minimum_required(VERSION 3.15)

project(pyclipr VERSION 0.1.0)

# Set c++ to use cx17 as a requirement defined in ClipperLib2
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ version selection")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cMake")

# Copy the include directory if specified
set(INCLUDE_INSTALL_DIR "${INCLUDE_INSTALL_DIR}/pyclipr")

# Generate DLL Pragma for creating shared libraries in Windows
include (GenerateExportHeader)

add_definitions("-DPROJECT_VERSION=\"${PROJECT_VERSION}\"" )
add_definitions("-DNOMINMAX -DFMT_HEADER_ONLY")

# removed -DCPU86

option(BUILD_PYTHON "Builds a python extension" ON)

FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

if(WIN32)
    # Remove Security definitions for the library
    # Remove run time checks for windows

    if(MSVC)

        set(COMMON_LANGUAGE_RUNTIME "")

        set(EIGEN3_INCLUDE_DIR external/eigen)
        #target_compile_options(Clipper2 PRIVATE /W4 /WX)

        add_compile_options("/GS-"
                            "/INCREMENTAL:NO")


    endif(MSVC)

    # Copy the shared libraries and libs to the main directory
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

else(WIN32)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

    set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

    #target_compile_options(Clipper2 PRIVATE -Wall -Wextra -Wpedantic -Werror)
    #target_link_libraries(Clipper2 PUBLIC -lm)

    # Find Required packages
    find_package(Eigen3 3.3 REQUIRED)

endif(WIN32)

find_package(OpenMP)

if(OpenMP_CXX_FOUND)
    message(STATUS "Found OpenMP")

    set(CLIPR_LIBRARIES
        OpenMP::OpenMP_CXX
    )
endif(OpenMP_CXX_FOUND)

# Set the base directory for cxx clipperLib2
set(CLIPPER2_INCLUDE_DIR
    external/clipper2/CPP
)


# Use the replacement of Boost::filesystem from a git submodule provided by WJakob
# in order to reduce compile time dependencies
# https://github.com/wjakob/filesystem

set(CPP_FILESYSTEM
    external/filesystem
)

## A replacement library for std::Format
set(CPP_FMT
    external/fmt/include
)

add_definitions(-DUSINGZ=1)

link_directories(
)

if(BUILD_PYTHON)
    message(STATUS "Building PyClipr Python Module")

    add_subdirectory(external/pybind11)

    set(PYBIND11_CPP_STANDARD /std:c++17)


endif(BUILD_PYTHON)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${CLIPPER2_INCLUDE_DIR}
    ${CLIPPER2_INCLUDE_DIR}/Clipper2Lib/include
    ${CPP_FMT}
)


link_directories(
)



set(CLIPPER2_INC
  Clipper2Lib/include/clipper2/clipper.h
  Clipper2Lib/include/clipper2/clipper.core.h
  Clipper2Lib/include/clipper2/clipper.engine.h
  Clipper2Lib/include/clipper2/clipper.export.h
  Clipper2Lib/include/clipper2/clipper.minkowski.h
  Clipper2Lib/include/clipper2/clipper.offset.h
  Clipper2Lib/include/clipper2/clipper.rectclip.h
)

set(CLIPPER2_SRCS
  Clipper2Lib/src/clipper.engine.cpp
  Clipper2Lib/src/clipper.offset.cpp
  Clipper2Lib/src/clipper.rectclip.cpp
)


set(APP_SRCS
    ${CLIPPER2_INC}
    ${CLIPPER2_SRCS}
)

PREPEND(APP_SRCS "${CLIPPER2_INCLUDE_DIR}/" ${APP_SRCS})


SOURCE_GROUP("App" FILES
    ${APP_SRCS}
)

set(LIBCLIPR_SRCS
    ${APP_SRCS}
)

set(PYCLIPR_LIBS
    ${CLIPR_LIBRARIES}
)

if(BUILD_PYTHON)
    # Add the library

    message(STATUS "Building PyClipr Python -  Static Library")

    add_definitions("-DCLIPR_BUILT_AS_STATIC")

    add_library(clipr_static STATIC ${LIBCLIPR_SRCS})

    target_link_libraries(clipr_static PRIVATE ${PYCLIPR_LIBS})

    GENERATE_EXPORT_HEADER(clipr_static
                 BASE_NAME CLIPR
                 EXPORT_MACRO_NAME CLIPR_EXPORT
                 EXPORT_FILE_NAME CLIPR_Export.h
                 STATIC_DEFINE CLIPR_BUILT_AS_STATIC)


else(BUILD_PYTHON)
    message(STATUS "Building PyClipr Module Standalone - Dynamic Library")

    add_library(clipr SHARED ${LIBCLIPR_SRCS})
    target_link_libraries(clipr ${PYCLIPR_LIBS})

    GENERATE_EXPORT_HEADER(clipr
                 BASE_NAME CLIPR
                 EXPORT_MACRO_NAME CLIPR_EXPORT
                 EXPORT_FILE_NAME CLIPR_Export.h
                 STATIC_DEFINE CLIPR_BUILT_AS_STATIC)

endif(BUILD_PYTHON)

set(App_SRCS
    main.cpp
)

SOURCE_GROUP("App" FILES ${App_SRCS})

if(BUILD_PYTHON)

    set(PYCLIPR_SRCS
        python/pyclipr/module.cpp
    )

    SOURCE_GROUP("Python" FILES ${PYCLIPR_SRCS})

    pybind11_add_module(pyclipr ${PYCLIPR_SRCS})

    #add_library(example MODULE main.cpp)

    target_link_libraries(pyclipr PRIVATE pybind11::module clipr_static ${PYCLIPR_LIBS})

    set_target_properties(pyclipr PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                             SUFFIX "${PYTHON_MODULE_EXTENSION}")

    install(TARGETS pyclipr DESTINATION lib/pyclipr)


else(BUILD_PYTHON)

    add_executable(main ${App_SRCS})
    target_link_libraries(main clipr ${PYCLIPR_LIBS})

    #install(TARGETS mpir DESTINATION )

endif(BUILD_PYTHON)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CLIPR_Export.h
    ${APP_H_SRCS}
    DESTINATION include/pyclipr
)

