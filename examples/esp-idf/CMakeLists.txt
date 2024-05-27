# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#if(DEFINED ENV{HTTP_PATH})
#  set(HTTP_PATH $ENV{HTTP_PATH})
#else()
#  set(HTTP_PATH ${CMAKE_CURRENT_LIST_DIR}/../../../PsychicHttp)
#endif(DEFINED ENV{HTTP_PATH})
#set(EXTRA_COMPONENT_DIRS ${HTTP_PATH})

set(EXTRA_COMPONENT_DIRS "../../")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(PsychicHttp_IDF)
