# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Add 'main' to global include path so components like LVGL can find lv_conf.h
add_compile_options("-I${CMAKE_CURRENT_LIST_DIR}/main")

project(sample_project)
