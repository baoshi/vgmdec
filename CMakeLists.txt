cmake_minimum_required (VERSION 3.13)

set(CMAKE_VERBOSE_MAKEFILE ON)

project ("vgmdec" VERSION 0.9.0)

add_subdirectory(lib/fft_q15)
add_subdirectory(lib/cwalk)
add_subdirectory(core)


include_directories(${PROJECT_SOURCE_DIR})

if (WIN32)

	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
	set(SDL2_DIR ${PROJECT_SOURCE_DIR}/windows)
	find_package(SDL2 REQUIRED)

	add_compile_definitions(VGM_WINDOWS _CRT_SECURE_NO_WARNINGS)

	add_executable (vgmplay
		ansicon.c
		cached_file_reader.c
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay vgmcore ${SDL2_LIBRARIES})
	set_target_properties(vgmplay PROPERTIES C_STANDARD 99)


	add_executable (vgmspectrum
		cached_file_reader.c
		vgmspectrum.c
	)
	target_include_directories(vgmspectrum PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmspectrum vgmcore fft_q15 ${SDL2_LIBRARIES})
	set_target_properties(vgmspectrum PROPERTIES C_STANDARD 99)

	add_executable (vgmdump
		cached_file_reader.c
		vgmdump.c
	)
	target_link_libraries(vgmdump vgmcore)
	set_target_properties(vgmdump PROPERTIES C_STANDARD 99)

	add_executable (reader_test
		cached_file_reader.c
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
		ansicon.c
		cached_file_reader.c
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay vgmcore ${SDL2_LDFLAGS})
	set_target_properties(vgmplay PROPERTIES C_STANDARD 99)

	add_executable (vgmspectrum
		cached_file_reader.c
		vgmspectrum.c
	)
	target_include_directories(vgmspectrum PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmspectrum vgmcore fft_q15 ${SDL2_LDFLAGS})
	set_property(TARGET vgmspectrum PROPERTY C_STANDARD 99)

	add_executable (vgmdump
		cached_file_reader.c
		vgmdump.c
	)
	target_link_libraries(vgmdump vgmcore)
	set_property(TARGET vgmdump PROPERTY C_STANDARD 99)

	add_executable (reader_test
		cached_file_reader.c
		reader_test.c
	)

	add_executable(make_const_tables 
		make_const_tables.c
	)

elseif (UNIX AND APPLE)

	include(FindPkgConfig)
	pkg_search_module(SDL2 REQUIRED sdl2)

	add_compile_definitions(VGM_MACOSX)

	add_executable (vgmplay
		ansicon.c
		cached_file_reader.c
		vgmplay.c
	)
	target_include_directories(vgmplay PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmplay vgmcore ${SDL2_LDFLAGS})
	set_target_properties(vgmplay PROPERTIES C_STANDARD 99)

	add_executable (vgmspectrum
		cached_file_reader.c
		vgmspectrum.c
	)
	target_include_directories(vgmspectrum PRIVATE ${SDL2_INCLUDE_DIRS})
	target_link_libraries(vgmspectrum vgmcore fft_q15 ${SDL2_LDFLAGS})
	set_property(TARGET vgmspectrum PROPERTY C_STANDARD 99)

	add_executable (vgmdump
		cached_file_reader.c
		vgmdump.c
	)
	target_link_libraries(vgmdump vgmcore)
	set_property(TARGET vgmdump PROPERTY C_STANDARD 99)

	add_executable (reader_test
		cached_file_reader.c
		reader_test.c
	)

	add_executable(make_const_tables 
		make_const_tables.c
	)

endif()	
