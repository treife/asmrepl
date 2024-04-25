cmake_minimum_required(VERSION 3.7)
project(asmrepl CXX)
add_executable(asmrepl
	assembler.cpp
	helpers.cpp
	interactive.cpp
	main.cpp
	pretty_print.cpp
	zombie.cpp
	zombie_main.cpp)
target_compile_features(asmrepl PRIVATE cxx_std_20)