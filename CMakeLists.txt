cmake_minimum_required(VERSION 3.20.0)

get_filename_component(ZEPHYR_CIRCULAR_BUFFER_MODULE_DIR
  ${CMAKE_CURRENT_LIST_DIR}/zephyr_circular_buffer
  ABSOLUTE
)

list(APPEND ZEPHYR_EXTRA_MODULES ${ZEPHYR_CIRCULAR_BUFFER_MODULE_DIR})

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Heart_monitor)

target_sources(app PRIVATE src/main.c src/watchdog.c)
zephyr_library_include_directories(src/include)
