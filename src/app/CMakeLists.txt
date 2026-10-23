set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(target_name scantailor-advanced)

set(resource_files
    resources.qrc
    dark_scheme/dark_scheme.qrc
    light_scheme/light_scheme.qrc)
list_items_prepend(resource_files "${SCANTAILOR_RESOURCES_DIR}/")

set(gui_only_sources
    RelinkablePathVisualization.cpp RelinkablePathVisualization.h
    RelinkingModel.cpp RelinkingModel.h
    RelinkingSortingModel.cpp RelinkingSortingModel.h
    RelinkingListView.cpp RelinkingListView.h
    RelinkingDialog.cpp RelinkingDialog.h
    SettingsDialog.cpp SettingsDialog.h
    FixDpiDialog.cpp FixDpiDialog.h
    LoadFilesStatusDialog.cpp LoadFilesStatusDialog.h
    ProjectCreationContext.cpp ProjectCreationContext.h
    ProjectOpeningContext.cpp ProjectOpeningContext.h
    OutOfMemoryDialog.cpp OutOfMemoryDialog.h
    ThumbnailSequence.cpp ThumbnailSequence.h
    ProjectFilesDialog.cpp ProjectFilesDialog.h
    NewOpenProjectPanel.cpp NewOpenProjectPanel.h
    SystemLoadWidget.cpp SystemLoadWidget.h
    MainWindow.cpp MainWindow.h
    main.cpp
    StatusBarPanel.cpp StatusBarPanel.h
    DefaultParamsDialog.cpp DefaultParamsDialog.h)

set(gui_only_ui_files
    AboutDialog.ui
    BatchProcessingLowerPanel.ui
    DefaultParamsDialog.ui
    FixDpiDialog.ui
    LoadFilesStatusDialog.ui
    MainWindow.ui
    NewOpenProjectPanel.ui
    OutOfMemoryDialog.ui
    ProjectFilesDialog.ui
    RelinkingDialog.ui
    RemovePagesDialog.ui
    SettingsDialog.ui
    StatusBarPanel.ui
    SystemLoadWidget.ui)

set(win32_resource_file "")
if (WIN32)
  if (MINGW)
    set(SCANTAILOR_WIN_ICON "${SCANTAILOR_RESOURCES_DIR}/win32/scantailor-advanced.ico")
  else()
    string(REPLACE "/" "\\\\" SCANTAILOR_WIN_DIR_NATIVE "${SCANTAILOR_RESOURCES_DIR}/win32")
    set(SCANTAILOR_WIN_ICON "${SCANTAILOR_WIN_DIR_NATIVE}\\scantailor-advanced.ico")
  endif()
  configure_file("${SCANTAILOR_RESOURCES_DIR}/win32/resources.rc.in" "${CMAKE_BINARY_DIR}/resources.rc" @ONLY)
  set(rc_file "${CMAKE_BINARY_DIR}/resources.rc")
  file(GLOB win32_resources "${SCANTAILOR_RESOURCES_DIR}/win32/*.ico")
  set_source_files_properties(
      "${rc_file}" PROPERTIES
      OBJECT_DEPENDS ${win32_resources})
  if (MINGW)
    # CMake doesn't know how to process .rc files with MinGW.
    if (CMAKE_RC_COMPILER)
      if (IS_ABSOLUTE "${CMAKE_RC_COMPILER}" AND EXISTS "${CMAKE_RC_COMPILER}")
        set(windres_command "${CMAKE_RC_COMPILER}")
      else()
        find_program(windres_command NAMES "${CMAKE_RC_COMPILER}")
      endif()
    endif()
    if (NOT windres_command)
      get_filename_component(c_compiler_name "${CMAKE_C_COMPILER}" NAME)
      string(REGEX REPLACE "gcc(-[0-9.]+)?$" "windres" windres_name "${c_compiler_name}")
      find_program(windres_command NAMES "${windres_name}" windres)
    endif()
    if (NOT windres_command)
      message(FATAL_ERROR "Could not find the MinGW windres compiler.")
    endif()

    set(win32_resource_file "${CMAKE_BINARY_DIR}/win32_resources.o")
    add_custom_command(
        OUTPUT "${win32_resource_file}"
        WORKING_DIRECTORY "${SCANTAILOR_RESOURCES_DIR}/win32"
        COMMAND "${windres_command}" -i "${rc_file}" -o "${win32_resource_file}"
        MAIN_DEPENDENCY "${rc_file}"
        DEPENDS ${win32_resources})
  else()
    set(win32_resource_file "${rc_file}")
  endif()
endif()

add_executable(
    ${target_name} WIN32 ${gui_only_sources} ${gui_only_ui_files}
    ${resource_files} ${win32_resource_file})
target_link_libraries(
    ${target_name}
    PRIVATE core ${EXTRA_LIBS})
target_include_directories(
    ${target_name}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
install(TARGETS ${target_name} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

if (Qt6_FOUND AND WIN32)
  #manage plugins with windeployqt.
  add_custom_command(
    TARGET ${target_name}
    POST_BUILD
    COMMAND ${QT_BINDIR}/windeployqt  $<TARGET_FILE:${target_name}>
    COMMENT "deploy qt files alongside binary (e.g. plugins)"
  )
  install(CODE "execute_process(COMMAND \"${QT_BINDIR}/windeployqt\" \"\$\{CMAKE_INSTALL_PREFIX\}\")")
endif()

translation_sources(${target_name} ${gui_only_sources} ${gui_only_ui_files})
