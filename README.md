nmea_parser
===========

A small C library to parse NMEA sentences most commonly used in GPS receivers:

- GGA - Global Positioning System Fix Data
- GLL - Geographic Position - Latitude/Longitude
- GSA - GNSS DOP and Active Satellites
- GSV - GNSS Satellites in View
- RMC - Recommended Minimum Specific GNSS Data
- VTG - Course Over Ground and Ground Speed

Tested with u-blox NEO-6M GPS module.

Installation:
-------------
You can add this repo as a submodule in your project by running:
```sh
git submodule add https://github.com/grappas/nmea_parser.git extern/nmea_parser
```
Or you can simply add the files in your project.

Example CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.10)
set(PROJECT_NAME my_epic_project)
project(${PROJECT_NAME} C)

# Add the nmea_parser as a submodule
add_subdirectory(extern/nmea_parser)

# Add the include path for nmea_parser
include_directories(extern/nmea_parser)

# Add the executable
add_executable(${PROJECT_NAME} src/main.c extern/nmea_parser/nmea_parser.c)

# NMEA_BUFFER_SIZE is the maximum length of the NMEA sentence - use redefinition with caution
# Printing is disabled by default
# You can disable the sentences you don't need to save memory.
target_compile_definitions(${PROJECT_NAME} PRIVATE
NMEA_PRINT=0 NMEA_BUFFER_SIZE=83 NMEA_GSV_ENABLED=0 NMEA_VTG_ENABLED=0 NMEA_GLL_ENABLED=0 NMEA_GSA_ENABLED=0
)

# Link the nmea_parser
target_link_libraries(${PROJECT_NAME} nmea_parser)
```
Example usage:
--------------
```c
// examle sentence
// $GPRMC,195413.00,A,4936.86732,N,01907.19394,E,0.117,,010624,,,A*74

// define a data structure
// every nullified pointer is ignored by the parser
xxGGA_t gga;
xxRMC_t rmc;

// nullify the pointers
nmea_nullify(&data);

// assign the pointers to the data structure
data.rmc = &rmc;
data.gga = &gga;

// create a buffer - it's important to have a fixed size buffer
nmeaBuffer_t buffer;

// initialize the data structure, set the sentence type and beginning of the parsing cycle
// GP stands for GPS. It can be changed to GN for GLONASS, etc.
// RMC stands for Recommended Minimum Specific GNSS Data. It can be changed to GGA, GLL, etc.
// It zeroes the data.cycle variable, which is used to determine if the struct if fully populated in current cycle.
// to be defined by the user
nmea_init(&data, "GP", "RMC");
// example filling the buffer with a sentence
int bytes_read = read(fd, buffer.str, sizeof(buffer.str));
// parsing the sentence
nmea_parse(&buffer, &data);
// End of cycle is determined by the cycles_max variable, which is the number of fields in the navData_t.
// defined by nmea_init function
if (data.cycle == data.cycles_max) {
    // do something with the data
}
```
Full example can be found here: https://github.com/grappas/json_parser_aviatech

NMEA 0183 protocol: https://tronico.fi/OH6NT/docs/NMEA0183.pdf
