cmake_minimum_required(VERSION 3.0.2)
project(StaticInlineBug)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "StaticInlineBug")

add_executable(StaticInlineBug main.cpp TU1.cpp TU2.cpp TypeIndex.h)
set_target_properties(StaticInlineBug PROPERTIES
	DEBUG_POSTFIX "_d"
	CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON
)
