cmake_minimum_required (VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE ON)

project ("vgmdec" VERSION 0.9.0)

add_library(vgmdec INTERFACE)
target_sources(vgmdec INTERFACE
	blip_buf.c
	vgm.c
	nesapu.c
)
if (PICO_PLATFORM MATCHES rp2040)
	target_sources(vgmdec INTERFACE
		file_reader_cached_fatfs.c
	)
	target_link_libraries(vgmdec INTERFACE
		fatfs
	)
else()
	target_sources(vgmdec INTERFACE
		file_reader_cached.c
	)
endif()
target_include_directories(vgmdec INTERFACE ${CMAKE_CURRENT_LIST_DIR})


if (WIN32)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	set(SDL2_DIR ${PROJECT_SOURCE_DIR}/windows)
	find_package(SDL2 REQUIRED)

	add_compile_definitions(VGM_WINDOWS _CRT_SECURE_NO_WARNINGS)

	add_executable (vgmplay
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay vgmdec ${SDL2_LIBRARIES})
	set_target_properties(vgmplay PROPERTIES C_STANDARD 99)

	add_executable (vgmdump
		vgmdump.c
	)
	target_link_libraries(vgmdump vgmdec)
	set_target_properties(vgmdump PROPERTIES C_STANDARD 99)

	add_executable (reader_test
		file_reader_cached.c
		reader_test.c
	)

	add_executable(make_const_tables 
		make_const_tables.c
	)

elseif (UNIX AND NOT APPLE)

	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)

	# we want to know all warnings especially implicit conversion
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wconversion")
	# turn off conversion warning on blip_buf.c
	set_source_files_properties(blip_buf.c PROPERTIES COMPILE_FLAGS -Wno-conversion)

	add_compile_definitions(VGM_LINUX)

	add_executable (vgmplay
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay vgmdec ${SDL2_LDFLAGS})
	set_property(TARGET vgmplay PROPERTY C_STANDARD 99)

	add_executable (vgmdump
		vgmdump.c
	)
	target_link_libraries(vgmdump vgmdec)
	set_property(TARGET vgmdump PROPERTY C_STANDARD 99)

	add_executable (reader_test
		file_reader_cached.c
		reader_test.c
	)

	add_executable(make_const_tables 
		make_const_tables.c
	)

endif()	
