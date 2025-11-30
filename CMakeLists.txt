cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

list(APPEND KCONFIG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Kconfig")

project(app1)

zephyr_include_directories(include)

add_subdirectory(src/apps)

target_sources(app PRIVATE
  src/main.c
  src/gui.c
  src/tool.c
  src/app_mgr.c
  src/app_list.c
  src/app_thread.c
)
