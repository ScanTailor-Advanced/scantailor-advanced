set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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
  string(REPLACE "/" "\\\\" SCANTAILOR_WIN_DIR_NATIVE "${SCANTAILOR_RESOURCES_DIR}/win32")
  configure_file("${SCANTAILOR_RESOURCES_DIR}/win32/resources.rc.in" "${CMAKE_BINARY_DIR}/resources.rc" @ONLY)
  set(rc_file "${CMAKE_BINARY_DIR}/resources.rc")
  file(GLOB win32_resources "${SCANTAILOR_RESOURCES_DIR}/win32/*.ico")
  set_source_files_properties(
      "${rc_file}" PROPERTIES
      OBJECT_DEPENDS ${win32_resources})
  if (MINGW)
    # CMake doesn't know how to process .rc files with MinGW.
    set(win32_resource_file "${CMAKE_BINARY_DIR}/win32_resources.o")
    add_custom_command(
        OUTPUT "${win32_resource_file}"
        WORKING_DIRECTORY "${SCANTAILOR_RESOURCES_DIR}/win32"
        COMMAND windres -i "${rc_file}" -o "${win32_resource_file}"
        MAIN_DEPENDENCY "${rc_file}"
        DEPENDS ${win32_resources})
  else()
    set(win32_resource_file "${rc_file}")
  endif()
endif()

if(IOS)
  list(APPEND gui_only_sources ios/IosDirectoryPicker.mm ios/IosDirectoryPicker.h)
  set_source_files_properties(ios/IosDirectoryPicker.mm
    PROPERTIES COMPILE_FLAGS "-x objective-c++")
  # iOS app icon via asset catalog
  set(IOS_ICON_CATALOG "${CMAKE_SOURCE_DIR}/ios/Assets.xcassets")
  list(APPEND gui_only_sources "${IOS_ICON_CATALOG}")
  set_source_files_properties("${IOS_ICON_CATALOG}"
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  # iOS: MACOSX_BUNDLE required; use all sources (UI will be stubbed later)
  # Resource files excluded — .qrc with desktop assets won't compile for iOS
  add_executable(
      ${target_name} MACOSX_BUNDLE
      ${gui_only_sources} ${gui_only_ui_files})
elseif(APPLE)
  add_executable(
      ${target_name} MACOSX_BUNDLE
      ${gui_only_sources} ${gui_only_ui_files}
      ${resource_files})
else()
  add_executable(
      ${target_name} WIN32
      ${gui_only_sources} ${gui_only_ui_files}
      ${resource_files} ${win32_resource_file})
endif()

target_link_libraries(
    ${target_name}
    PRIVATE core ${EXTRA_LIBS})

target_include_directories(
    ${target_name}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

# TIFF include dir only available on desktop — not cross-compiled for iOS
if(NOT IOS)
  target_include_directories(
      ${target_name}
      PRIVATE "$<TARGET_PROPERTY:TIFF::TIFF,INTERFACE_INCLUDE_DIRECTORIES>")
endif()

install(TARGETS ${target_name}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION "${CMAKE_INSTALL_APPDIR}")

if(Qt6_FOUND AND WIN32)
  #manage plugins with windeployqt.
  add_custom_command(
      TARGET ${target_name}
      POST_BUILD
      COMMAND ${QT_BINDIR}/windeployqt $<TARGET_FILE:${target_name}>
      COMMENT "deploy qt files alongside binary (e.g. plugins)")
  install(CODE "execute_process(COMMAND \"${QT_BINDIR}/windeployqt\" \"\$\{CMAKE_INSTALL_PREFIX\}\")")
endif()

translation_sources(${target_name} ${gui_only_sources} ${gui_only_ui_files})

# macOS: set app icon and bundle name
if(APPLE AND NOT IOS)
  set_target_properties(${target_name} PROPERTIES OUTPUT_NAME "ScanTailor Advanced")
  set(MACOS_ICON "${CMAKE_SOURCE_DIR}/src/resources/appicon.icns")
  if(EXISTS "${MACOS_ICON}")
    target_sources(${target_name} PRIVATE "${MACOS_ICON}")
    set_source_files_properties("${MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(${target_name} PROPERTIES MACOSX_BUNDLE_ICON_FILE appicon.icns)
  endif()
endif()
