cmake_minimum_required(VERSION 2.8)

project(io C ASM)

set(HEADERS 
	include/io.h
)

set(SOURCES 
	src/task/task.c
	src/event.c
	src/loop.c
	src/memory.c
	src/moment.c
	src/pool.c
	src/rbtree.c
	src/stopwatch.c
	src/stream.c
	src/thread.c
	src/threadpool.c
	src/time.c
)

if(WIN32)

	set(SOURCES ${SOURCES}
		src/loop-win.c
	)

	if(CMAKE_GENERATOR MATCHES "64")
		set(SOURCES ${SOURCES}
			src/task/win64.asm)
		set_source_files_properties(src/task/win64.asm PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")
	endif(CMAKE_GENERATOR MATCHES "64")

else(WIN32)

	if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
		set(SOURCES ${SOURCES}
			src/fs-linux.c
			src/loop-linux.c
			src/stream-linux.c
			src/tcp-linux.c
		)
	endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	
	set(SOURCES ${SOURCES}
		src/task/asm.s)
	set_source_files_properties(src/task/asm.s PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp")

endif(WIN32)

macro(GroupSources curdir)
	file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
	foreach(child ${children})
		if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			GroupSources(${curdir}/${child})
		else()
			string(REPLACE "/" "\\" groupname ${curdir})
			source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
		endif()
	endforeach()
endmacro()

GroupSources(include)
GroupSources(src)

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_library(io SHARED ${HEADERS} ${SOURCES})
set_target_properties(io PROPERTIES COMPILE_DEFINITIONS "IO_BUILD_SHARED=1")
if(UNIX AND NOT APPLE)
    target_link_libraries(io rt)
endif(UNIX AND NOT APPLE)

install(TARGETS io DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_WRITE GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)
install(FILES ${HEADERS} DESTINATION include/io)

project(hello-server C)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(hello-server samples/hello-server.c)
target_link_libraries(hello-server io)

project(file-server C)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(file-server samples/file-server.c)
target_link_libraries(file-server io)
