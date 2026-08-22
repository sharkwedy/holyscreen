add_library(presenter-core
    core/CommandCatalog.cpp
    core/CommandCatalog.h
    core/CommandTypes.h
    core/CommandBus.cpp
    core/CommandBus.h
    core/EventTypes.h
    core/EventBus.cpp
    core/EventBus.h
    core/UndoManager.cpp
    core/UndoManager.h
)
target_include_directories(presenter-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-core PUBLIC Qt6::Core)

add_library(presenter-screens
    screens/ScreenManager.cpp
    screens/ScreenManager.h
    screens/OutputManager.cpp
    screens/OutputManager.h
    screens/OutputRouting.cpp
    screens/OutputRouting.h
)
target_include_directories(presenter-screens PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-screens PUBLIC Qt6::Core Qt6::Gui)

add_library(presenter-presentation
    presentation/PresentationController.cpp
    presentation/Clock.cpp
    presentation/Clock.h
    presentation/MediaTypes.h
    presentation/AudioTypes.h
    presentation/IAudioEngine.h
    presentation/AudioEngine.cpp
    presentation/VideoTypes.h
    presentation/IVideoEngine.h
    presentation/VideoFrameBus.cpp
    presentation/VideoFrameBus.h
    presentation/VideoEngine.cpp
    presentation/ImagePresentationController.cpp
    presentation/ImagePresentationController.h
    presentation/PresentationTypes.h
    presentation/TextPresentationController.cpp
    presentation/TextPresentationController.h
    presentation/TimedMediaPlayback.cpp
    presentation/TimedMediaPlayback.h
    presentation/OverlayController.cpp
    presentation/OverlayController.h
    presentation/ThemeTypes.h
)
target_include_directories(presenter-presentation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-presentation PUBLIC Qt6::Core Qt6::Gui Qt6::Multimedia)

add_library(presenter-modules
    modules/EventCommandModule.cpp
    modules/EventCommandModule.h
    modules/BibleCommandModule.cpp
    modules/BibleCommandModule.h
    modules/OutputRoutingCommandModule.cpp
    modules/OutputRoutingCommandModule.h
    modules/StageCommandModule.cpp
    modules/StageCommandModule.h
    modules/PresentationCommandModule.cpp
    modules/PresentationCommandModule.h
    modules/OutputModule.cpp
    modules/OutputModule.h
    modules/OverlayCommandModule.cpp
    modules/OverlayCommandModule.h
    modules/MediaCommandModule.cpp
    modules/MediaCommandModule.h
    modules/UndoCommandModule.cpp
    modules/UndoCommandModule.h
)
target_include_directories(presenter-modules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-modules PUBLIC presenter-core presenter-presentation Qt6::Core)

add_library(presenter-persistence
    persistence/ApplicationDatabase.cpp
    persistence/ApplicationDatabase.h
    persistence/DatabaseMigrator.cpp
    persistence/DatabaseMigrator.h
    persistence/SettingsRepository.cpp
)
target_include_directories(presenter-persistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-persistence PUBLIC Qt6::Core Qt6::Sql)

add_library(presenter-library
    library/MediaFolderScanner.cpp
    library/MediaFolderScanner.h
    library/MediaRepository.cpp
    library/MediaRepository.h
    library/PresentationRepository.cpp
    library/PresentationRepository.h
    library/ThemeRepository.cpp
    library/ThemeRepository.h
    library/EventRepository.cpp
    library/EventRepository.h
    library/HistoryRepository.cpp
    library/HistoryRepository.h
)
target_include_directories(presenter-library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-library PUBLIC Qt6::Core Qt6::Sql presenter-presentation)

add_library(presenter-bible
    bible/BibleTypes.h
    bible/PathContainment.h
    bible/PathContainment.cpp
    bible/BibleImportTypes.h
    bible/BibleImportTypes.cpp
    bible/IBibleSourceImporter.h
    bible/BibleReferenceParser.h
    bible/BibleReferenceParser.cpp
    bible/BibleRepository.h
    bible/BibleRepository.cpp
    bible/BibleJsonImporter.h
    bible/BibleJsonImporter.cpp
    bible/CanonicalBibleImporter.h
    bible/CanonicalBibleImporter.cpp
    bible/BibleSourceStager.h
    bible/BibleSourceStager.cpp
    bible/BibleImportService.h
    bible/BibleImportService.cpp
)
target_include_directories(presenter-bible PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(presenter-bible PRIVATE
    ${libgit2_SOURCE_DIR}/include
    ${libgit2_BINARY_DIR}/include
)
target_link_libraries(presenter-bible PUBLIC Qt6::Core Qt6::Sql Qt6::Network)
target_link_libraries(presenter-bible PRIVATE libgit2package miniz)

add_library(presenter-app
    app/ApplicationController.cpp
    app/AutosaveCoordinator.cpp
    app/AutosaveCoordinator.h
    app/DataRecoveryService.cpp
    app/DataRecoveryService.h
    app/AppLogger.cpp
    app/AppLogger.h
    app/UpdateChecker.cpp
    app/UpdateChecker.h
    remote/LocalApiServer.cpp
    remote/LocalApiServer.h
)
target_include_directories(presenter-app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-app PUBLIC
    presenter-core presenter-modules presenter-persistence presenter-screens presenter-presentation presenter-library presenter-bible Qt6::Gui Qt6::Network Qt6::Concurrent Qt6::HttpServer Qt6::WebSockets
)

qt_add_executable(church-presenter
    WIN32
    MACOSX_BUNDLE
    app/main.cpp
)
target_compile_definitions(church-presenter PRIVATE HOLYSCREEN_VERSION="${PROJECT_VERSION}")
set_target_properties(church-presenter PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "HolyScreen"
    MACOSX_BUNDLE_GUI_IDENTIFIER "app.holyscreen.presenter"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
)
if(APPLE)
    set(holyscreen_macos_icon "${CMAKE_SOURCE_DIR}/packaging/macos/HolyScreen.icns")
    set_source_files_properties(${holyscreen_macos_icon} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(church-presenter PRIVATE ${holyscreen_macos_icon})
    set_target_properties(church-presenter PROPERTIES
        OUTPUT_NAME "HolyScreen"
        MACOSX_BUNDLE_ICON_FILE "HolyScreen.icns"
    )
endif()
if(WIN32)
    set_target_properties(church-presenter PROPERTIES OUTPUT_NAME "holyscreen")
    target_sources(church-presenter PRIVATE
        ${CMAKE_SOURCE_DIR}/packaging/windows/holyscreen.rc
        ${CMAKE_SOURCE_DIR}/packaging/windows/holyscreen.ico
    )
endif()
qt_add_qml_module(church-presenter
    URI ChurchPresenter
    VERSION 1.0
    QML_FILES
        ui/operator/MainWindow.qml
        ui/operator/Dashboard.qml
        ui/operator/SettingsDialog.qml
        ui/operator/BibleBrowser.qml
        ui/operator/QuickBibleSearch.qml
        ui/output/OutputWindow.qml
        ui/output/SimulatedOutput.qml
        ui/output/PresentationImageLayer.qml
        ui/output/PresentationTextLayer.qml
        ui/output/StageView.qml
        ui/output/LiveOverlays.qml
)
target_link_libraries(church-presenter
    PRIVATE
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Multimedia
        Qt6::HttpServer Qt6::WebSockets
        presenter-app presenter-screens presenter-presentation presenter-persistence presenter-library
        presenter-bible
)
