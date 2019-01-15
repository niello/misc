cmake_minimum_required(VERSION 3.0.2)
project(RawInputLocale)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "RawInputLocale")

add_executable(RawInputLocale WIN32 main.cpp)
set_target_properties(RawInputLocale PROPERTIES DEBUG_POSTFIX "_d")
