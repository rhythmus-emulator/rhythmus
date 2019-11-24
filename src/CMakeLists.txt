project(rhythmus)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
if (MSVC)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
endif (MSVC)

set (RHYTHMUS_SOURCES
    ResourceManager.cpp
    Image.cpp
	BaseObject.cpp
    Sprite.cpp
    Font.cpp
    Game.cpp
    Player.cpp
    Graphic.cpp
    Logger.cpp
    Timer.cpp
    main.cpp
    SceneManager.cpp
    Scene.cpp
	Script.cpp
	Event.cpp
	Song.cpp
	Sound.cpp
	Setting.cpp
	TaskPool.cpp
	Util.cpp
	
    scene/LoadingScene.cpp
    scene/TestScene.cpp
    scene/SelectScene.cpp
    scene/DecideScene.cpp
    scene/PlayScene.cpp
    scene/ResultScene.cpp
    scene/OptionScene.cpp
	
	object/Menu.cpp
	object/MusicWheel.cpp
    object/OptionMenu.cpp
    object/Text.cpp
    object/Number.cpp
    object/NoteField.cpp

    LR2/exdxa.cpp
    LR2/LR2Font.cpp
    LR2/LR2SceneLoader.cpp
    LR2/LR2Flag.cpp
    )

set (RHYTHMUS_HEADERS
    ResourceManager.h
    Image.h
	BaseObject.h
    Sprite.h
    Font.h
    Game.h
    Player.h
    Logger.h
    Timer.h
    Graphic.h
    Error.h
    SceneManager.h
    Scene.h
	Script.h
	Event.h
	Song.h
	Sound.h
	Setting.h
	TaskPool.h
	Util.h
	
    scene/LoadingScene.h
    scene/TestScene.h
    scene/SelectScene.h
    scene/DecideScene.h
    scene/PlayScene.h
    scene/ResultScene.h
    scene/OptionScene.h

	object/Menu.h
	object/MusicWheel.h
    object/OptionMenu.h
    object/Text.h
    object/Number.h
    object/NoteField.h

    LR2/exdxa.h
    LR2/LR2Font.h
    LR2/LR2SceneLoader.h
    LR2/LR2Flag.h
    )

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${RPARSER_INCLUDE_DIR}
	${RENCODER_INCLUDE_DIR}
    )

include_directories(SYSTEM
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${ZIP_INCLUDE_DIR}
    ${FREEIMAGE_INCLUDE_DIR}
    ${TINYXML2_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIR}
    ${RENCODER_INCLUDE_DIR}
	${OGG_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
	${FLAC_INCLUDE_DIR}
    ${TIMIDITY_INCLUDE_DIR}
    ${OPENAL_INCLUDE_DIR}
    ${SQLITE_INCLUDE_DIR}
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
    ${ZLIB_LIBRARY}
    ${ZIP_LIBRARY}
    ${OPENSSL_LIBRARY}
    ${OPENAL_LIBRARY}
    tinyxml2
    rparser
	rencoder_lib
	timidity
    sqlite
    )