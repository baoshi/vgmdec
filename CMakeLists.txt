# CMakeList.txt : CMake project for vgm-decoder, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project ("vgmdec")

# Add source to this project's executable.
add_executable (reader_test
	file_reader_cached.c
	reader_test.c
)

if (WIN32)
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	set(SDL2_DIR ${PROJECT_SOURCE_DIR}/windows)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})

	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

	add_executable (vgmdec
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmdec.c
	)
	target_link_libraries(vgmdec ${SDL2_LIBRARIES})
	set_property(TARGET vgmdec PROPERTY C_STANDARD 99)

elseif (UNIX)

	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)
	include_directories(${SDL2_INCLUDE_DIRS})
	add_executable (vgmdec
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmdec.c
	)
	target_link_libraries(vgmdec ${SDL2_LDFLAGS})
	set_property(TARGET vgmdec PROPERTY C_STANDARD 99)

endif()	
