project(rhythmus)

if (MSVC)
  add_definitions(-DUNICODE -D_UNICODE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /SAFESEH:NO")
endif (MSVC)

# build configs
if (DEFINED USE_GLEW_ES)
  add_definitions(-DUSE_GLEW_ES=${USE_GLEW_ES})
endif()

set (RHYTHMUS_SOURCES
  ResourceManager.cpp
  Image.cpp
  BaseObject.cpp
  Sprite.cpp
  Font.cpp
  Game.cpp
  Player.cpp
  Graphic.cpp
  Error.cpp
  Logger.cpp
  Timer.cpp
  main.cpp
  SceneManager.cpp
  Scene.cpp
  Script.cpp
  Event.cpp
  Song.cpp
  SongPlayer.cpp
  Sound.cpp
  Setting.cpp
  KeyPool.cpp
  TaskPool.cpp
  Util.cpp

  PlaySession.cpp
  PlaySession7Key.cpp

  scene/LoadingScene.cpp
  scene/TestScene.cpp
  scene/SelectScene.cpp
  scene/DecideScene.cpp
  scene/PlayScene.cpp
  scene/ResultScene.cpp
  scene/OptionScene.cpp
  scene/OverlayScene.cpp

  object/ListView.cpp
  object/MusicWheel.cpp
  object/OptionMenu.cpp
  object/Text.cpp
  object/Number.cpp
  object/Slider.cpp
  object/Bargraph.cpp
  object/Quad.cpp
  object/Line.cpp
  object/Button.cpp
  object/OnMouse.cpp
  object/NoteField.cpp
  object/Dialog.cpp

  LR2/exdxa.cpp
  )

set (RHYTHMUS_HEADERS
  common.h
  config.h
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
  SongPlayer.h
  Sound.h
  Setting.h
  KeyPool.h
  TaskPool.h
  Util.h

  PlaySession.h
  PlaySession7Key.h

  scene/LoadingScene.h
  scene/TestScene.h
  scene/SelectScene.h
  scene/DecideScene.h
  scene/PlayScene.h
  scene/ResultScene.h
  scene/OptionScene.h
  scene/OverlayScene.h

  object/ListView.h
  object/MusicWheel.h
  object/OptionMenu.h
  object/Text.h
  object/Number.h
  object/Slider.h
  object/Bargraph.h
  object/Quad.h
  object/Line.h
  object/Button.h
  object/OnMouse.h
  object/NoteField.h
  object/Dialog.h

  LR2/exdxa.h
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

set(BIN_LIB "")
if (WIN32)
  add_executable(rhythmus_bin WIN32 ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
else ()
  add_executable(rhythmus_bin ${RHYTHMUS_SOURCES} ${RHYTHMUS_HEADERS})
  set(BIN_LIB X11 lzma va va-drm va-x11 vdpau)
endif ()

target_link_libraries(rhythmus_bin
  tinyxml2
  rparser
  rencoder_lib
  timidity
  sqlite
  ${OPENGL_LIBRARIES}
  ${GLFW_LIBRARY}
  ${GLEW_LIBRARIES}
  ${FREEIMAGE_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${FFMPEG_LIBRARIES}
  ${ZLIB_LIBRARY}
  ${OPENSSL_LIBRARY}
  ${OPENAL_LIBRARY}
  ${ZIP_LIBRARY}
  ${CMAKE_DL_LIBS}
  ${BIN_LIB}
  )
