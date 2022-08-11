# CMakeList.txt : CMake project for vgm-decoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("vgmdec")

if (WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Add source to this project's executable.
add_executable (reader_test
	file_reader_cached.c
	reader_test.c
)

add_executable (vgmdec
	file_reader_cached.c
	vgm.c
	nesapu.c
	vgmdec.c
)

set_property(TARGET vgmdec PROPERTY C_STANDARD 99)


