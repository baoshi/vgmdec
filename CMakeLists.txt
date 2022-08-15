cmake_minimum_required (VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE ON)

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

	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

	add_executable (vgmplay
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay ${SDL2_LIBRARIES})
	set_target_properties(vgmplay PROPERTIES C_STANDARD 99)

	add_executable (vgmdump
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmdump.c
	)
	set_target_properties(vgmdump PROPERTIES C_STANDARD 99)

elseif (UNIX)

	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)
	add_executable (vgmplay
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay ${SDL2_LDFLAGS})
	set_property(TARGET vgmplay PROPERTY C_STANDARD 99)

	add_executable (vgmdump
		blip_buf.c
		file_reader_cached.c
		fixedpoint.c
		vgm.c
		nesapu.c
		vgmdump.c
	)
	set_property(TARGET vgmdump PROPERTY C_STANDARD 99)

endif()	
