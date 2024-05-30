# extern/nmea_parser/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(nmea_parser C)

# Add the source files for the nmea_parser library
add_library(nmea_parser STATIC nmea_parser.c)

# Specify the include directories for the nmea_parser library
target_include_directories(nmea_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
