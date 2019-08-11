project(rhythmus)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif (MSVC)

set (RHYTHMUS_SOURCES
    ResourceManager.cpp
    Image.cpp
	Sprite.cpp
	Animation.cpp
	Font.cpp
    Game.cpp
    Graphic.cpp
	Logger.cpp
	Timer.cpp
    main.cpp

    SceneManager.cpp
    scene/Scene.cpp
    scene/TestScene.cpp
    )

set (RHYTHMUS_HEADERS
    ResourceManager.h
    Image.h
	Sprite.h
	Animation.h
	Font.h
    Game.h
	Logger.h
	Timer.h
    Graphic.h
	Error.h

    SceneManager.h
    scene/Scene.h
    scene/TestScene.h
    )

include_directories(
	${PROJECT_SOURCE_DIR}
    )

include_directories(SYSTEM
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${FREEIMAGE_INCLUDE_DIR}
    ${TINYXML2_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIR}
    ${RPARSER_INCLUDE_DIR}
    )

if (WIN32)
    add_executable(rhythmus_bin WIN32 ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
else ()
    add_executable(rhythmus_bin ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
endif ()

target_link_libraries(rhythmus_bin
    ${OPENGL_LIBRARIES}
	${GLFW_LIBRARY}
	${GLEW_LIBRARY}
    ${FREEIMAGE_LIBRARY}
	${FREETYPE_LIBRARY}
	${FFMPEG_LIBRARY}
    tinyxml2
    rparser
	)