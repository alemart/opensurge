set(ALLEGRO_LIBS
  allegro
  allegro_main
  allegro_image
  allegro_primitives
  allegro_color
  allegro_font
  allegro_ttf
  allegro_acodec
  allegro_audio
  allegro_dialog
  allegro_memfile
  allegro_physfs
)

if(ALLEGRO_MONOLITH)
  set(ALLEGRO_LIBS allegro_monolith)
endif()

if(ALLEGRO_STATIC)

  # add -static suffix
  set(A5_LIBS "")
  foreach(A5_LIB ${ALLEGRO_LIBS})
    list(APPEND A5_LIBS "${A5_LIB}-static")
  endforeach()
  set(ALLEGRO_LIBS ${A5_LIBS})

  # Win32 libs
  if(WIN32)
    list(APPEND ALLEGRO_LIBS
      mingw32
      #dumb
      #openmpt
      #FLAC
      vorbisfile
      vorbis
      freetype
      ogg
      physfs
      png16
      z
      #zlibstatic
      gdiplus
      uuid
      kernel32
      winmm
      psapi
      opengl32
      glu32
      user32
      comdlg32
      gdi32
      shell32
      ole32
      advapi32
      ws2_32
      shlwapi
      jpeg
      d3d9
      dinput8
      dsound
      #opusfile
      #opus
      #webp
    )
  else()
    message(FATAL_ERROR "Static linking is unavailable for the target platform")
  endif()

endif()

set(SURGESCRIPT_LIB surgescript)
if(SURGESCRIPT_STATIC)
  set(SURGESCRIPT_LIB surgescript-static)
endif()
