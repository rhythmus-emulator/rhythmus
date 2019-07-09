project(rhythmus)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
endif (MSVC)

set (RHYTHMUS_SOURCES
    Game.cpp
    main.cpp
    )

set (RHYTHMUS_HEADERS
    Game.h
    )

include_directories(
    )

include_directories(SYSTEM
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    )

if (WIN32)
    add_executable(rhythmus_bin WIN32 ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
else ()
    add_executable(rhythmus_bin ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
endif ()

target_link_libraries(rhythmus_bin
    ${OPENGL_LIBRARIES}
	${GLFW_LIBRARY}
	)