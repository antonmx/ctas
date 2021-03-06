cmake_minimum_required (VERSION 2.6)
cmake_policy(SET CMP0045 NEW)

project (ctas)
set(VERSION 0.7.0)

SET(CMAKE_MODULE_PATH ${ctas_SOURCE_DIR}/cmake)
set(CMAKE_C_STANDARD "11")
set(CMAKE_CXX_STANDARD "11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
set(CMAKE_CXX_FLAGS "$ENV{CPPFLAGS} ${CMAKE_CXX_FLAGS} -ffast-math")

add_custom_target(dist
    COMMAND git archive --prefix=${PROJECT_NAME}.${VERSION}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.${VERSION}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

include(CheckIncludeFile)
include(CheckLibraryExists)

set(LIBEXEC_DESTINATION "libexec/${PROJECT_NAME}")
set(LIB_DESTINATION "lib/${PROJECT_NAME}")
set(MAN_DESTINATION "man/man1")
add_custom_target(doc)

# Doxygen documentation
find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                 ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
                 @ONLY)
  add_custom_target(doxygen
    COMMAND echo "  -- Running doxygen. Output will be redirected to \"\\\"${CMAKE_CURRENT_BINARY_DIR}/doxygen.log\\\"\"."
    COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile &> doxygen.log)
endif()

# Just a workaroun to show cl files in Qt creator
FILE(GLOB_RECURSE OpenCLfiles "*.cl")
add_custom_target(OpenCLsource SOURCES ${OpenCLfiles})


add_subdirectory(src)






