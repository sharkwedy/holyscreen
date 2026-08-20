add_library(presenter-core
    core/CommandTypes.h
    core/CommandBus.cpp
    core/CommandBus.h
    core/EventTypes.h
    core/EventBus.cpp
    core/EventBus.h
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
    bible/BibleReferenceParser.h
    bible/BibleReferenceParser.cpp
    bible/BibleRepository.h
    bible/BibleRepository.cpp
    bible/BibleJsonImporter.h
    bible/BibleJsonImporter.cpp
)
target_include_directories(presenter-bible PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-bible PUBLIC Qt6::Core Qt6::Sql)

add_library(presenter-app
    app/ApplicationController.cpp
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
    presenter-core presenter-persistence presenter-screens presenter-presentation presenter-library presenter-bible Qt6::Gui Qt6::Network Qt6::HttpServer Qt6::WebSockets
)

qt_add_executable(church-presenter
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
qt_add_qml_module(church-presenter
    URI ChurchPresenter
    VERSION 1.0
    QML_FILES
        ui/operator/MainWindow.qml
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

install(TARGETS church-presenter
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
if(APPLE)
    install(CODE "
        execute_process(
            COMMAND \"${MACDEPLOYQT_EXECUTABLE}\"
                    \"\${CMAKE_INSTALL_PREFIX}/church-presenter.app\"
                    \"-qmldir=${CMAKE_CURRENT_SOURCE_DIR}/ui\"
                    -always-overwrite
            RESULT_VARIABLE deploy_result
        )
        if(NOT deploy_result EQUAL 0)
            message(FATAL_ERROR \"macdeployqt failed with exit code \${deploy_result}\")
        endif()
    ")
else()
    qt_generate_deploy_qml_app_script(
        TARGET church-presenter
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()
