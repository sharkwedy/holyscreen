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
    screens/OutputRole.cpp
    screens/OutputRole.h
    screens/BroadcastProfile.cpp
    screens/BroadcastProfile.h
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
    modules/IntegrationCommandModule.cpp
    modules/IntegrationCommandModule.h
    modules/OutputStateModule.cpp
    modules/OutputStateModule.h
    modules/PlaylistCommandModule.cpp
    modules/PlaylistCommandModule.h
    modules/ThemeCommandModule.cpp
    modules/ThemeCommandModule.h
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
target_link_libraries(presenter-modules PUBLIC presenter-core presenter-presentation presenter-screens presenter-integrations Qt6::Core)

add_library(presenter-integrations
    integrations/IntegrationTypes.cpp
    integrations/IntegrationTypes.h
    integrations/IntegrationSanitizer.cpp
    integrations/IntegrationSanitizer.h
    integrations/IntegrationEngine.cpp
    integrations/IntegrationEngine.h
    integrations/ports/IIntegrationAdapter.h
    integrations/ports/IIntegrationRepository.h
    integrations/ports/ISecretStore.h
    integrations/ports/ITransports.h
    integrations/adapters/HttpIntegrationAdapter.cpp
    integrations/adapters/HttpIntegrationAdapter.h
    integrations/adapters/WebSocketIntegrationAdapter.cpp
    integrations/adapters/WebSocketIntegrationAdapter.h
    integrations/adapters/ObsAuthentication.cpp
    integrations/adapters/ObsAuthentication.h
    integrations/adapters/ObsIntegrationAdapter.cpp
    integrations/adapters/ObsIntegrationAdapter.h
    integrations/ports/IObsClient.h
    integrations/transports/ObsWebSocketClient.cpp
    integrations/transports/ObsWebSocketClient.h
    integrations/adapters/MidiIntegrationAdapter.cpp
    integrations/adapters/MidiIntegrationAdapter.h
    integrations/transports/RtMidiTransport.cpp
    integrations/transports/RtMidiTransport.h
    integrations/adapters/OscMessage.cpp
    integrations/adapters/OscMessage.h
    integrations/adapters/OscIntegrationAdapter.cpp
    integrations/adapters/OscIntegrationAdapter.h
    integrations/transports/QtOscTransport.cpp
    integrations/transports/QtOscTransport.h
    integrations/transports/QtHttpTransport.cpp
    integrations/transports/QtHttpTransport.h
    integrations/transports/QtWebSocketTransport.cpp
    integrations/transports/QtWebSocketTransport.h
    integrations/secrets/InMemorySecretStore.cpp
    integrations/secrets/InMemorySecretStore.h
    integrations/secrets/SecretStoreFactory.cpp
    integrations/secrets/SecretStoreFactory.h
)
target_include_directories(presenter-integrations PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-integrations PUBLIC Qt6::Core Qt6::Network Qt6::WebSockets)
target_link_libraries(presenter-integrations PRIVATE rtmidi)
target_include_directories(presenter-integrations PRIVATE ${rtmidi_SOURCE_DIR})

# Cofre de segredos nativo de cada sistema. Sem cofre disponível o motor usa o
# armazenamento em memória e a interface avisa o operador.
if(APPLE)
    target_sources(presenter-integrations PRIVATE
        integrations/secrets/KeychainSecretStore.cpp
        integrations/secrets/KeychainSecretStore.h
    )
    target_link_libraries(presenter-integrations PRIVATE
        "-framework Security" "-framework CoreFoundation")
elseif(WIN32)
    target_sources(presenter-integrations PRIVATE
        integrations/secrets/CredentialManagerSecretStore.cpp
        integrations/secrets/CredentialManagerSecretStore.h
    )
    target_link_libraries(presenter-integrations PRIVATE Advapi32)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBSECRET IMPORTED_TARGET libsecret-1)
    endif()
    if(TARGET PkgConfig::LIBSECRET)
        target_sources(presenter-integrations PRIVATE
            integrations/secrets/SecretServiceSecretStore.cpp
            integrations/secrets/SecretServiceSecretStore.h
        )
        target_compile_definitions(presenter-integrations PRIVATE HOLYSCREEN_HAS_LIBSECRET)
        target_link_libraries(presenter-integrations PRIVATE PkgConfig::LIBSECRET)
    else()
        message(STATUS "libsecret-1 não encontrada: os segredos ficarão apenas em memória")
    endif()
endif()

add_library(presenter-automation
    automation/AutomationTypes.cpp
    automation/AutomationTypes.h
    automation/ConditionEvaluator.cpp
    automation/ConditionEvaluator.h
    automation/AutomationEngine.cpp
    automation/AutomationEngine.h
    automation/LocalTimeTriggerScheduler.cpp
    automation/LocalTimeTriggerScheduler.h
    automation/TriggerTranslator.cpp
    automation/TriggerTranslator.h
    automation/AuthorizedExecutables.cpp
    automation/AuthorizedExecutables.h
    automation/QtProcessRunner.cpp
    automation/QtProcessRunner.h
)
target_include_directories(presenter-automation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(presenter-automation PUBLIC Qt6::Core presenter-core presenter-integrations)

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
    library/AutomationRepository.cpp
    library/AutomationRepository.h
    library/BroadcastProfileRepository.cpp
    library/BroadcastProfileRepository.h
    library/IntegrationRepository.cpp
    library/IntegrationRepository.h
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
target_link_libraries(presenter-library PUBLIC Qt6::Core Qt6::Sql presenter-presentation presenter-screens presenter-integrations presenter-automation)

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
    app/ConfigurationProfileService.cpp
    app/ConfigurationProfileService.h
    app/contexts/AutomationContext.cpp
    app/contexts/AutomationContext.h
    app/contexts/BibleContext.cpp
    app/contexts/BibleContext.h
    app/contexts/EventContext.cpp
    app/contexts/EventContext.h
    app/contexts/IntegrationContext.cpp
    app/contexts/IntegrationContext.h
    app/contexts/MaintenanceContext.cpp
    app/contexts/MaintenanceContext.h
    app/contexts/MediaContext.cpp
    app/contexts/MediaContext.h
    app/contexts/OutputContext.cpp
    app/contexts/OutputContext.h
    app/DiagnosticExporter.cpp
    app/DiagnosticExporter.h
    app/AutosaveCoordinator.cpp
    app/AutosaveCoordinator.h
    app/DataRecoveryService.cpp
    app/DataRecoveryService.h
    app/AppLogger.cpp
    app/AppLogger.h
    app/MediaThumbnailer.cpp
    app/MediaThumbnailer.h
    app/OnlineLyricsService.cpp
    app/OnlineLyricsService.h
    app/EnduranceRunner.cpp
    app/EnduranceRunner.h
    app/ProcessMetrics.cpp
    app/ProcessMetrics.h
    app/UpdateChecker.cpp
    app/UpdateChecker.h
    app/UpdateDownloader.cpp
    app/UpdateDownloader.h
    app/UpdateInstaller.cpp
    app/UpdateInstaller.h
    remote/LocalApiServer.cpp
    remote/LocalApiServer.h
    remote/RemoteAuthManager.cpp
    remote/RemoteAuthManager.h
    remote/RemoteQrCode.cpp
    remote/RemoteQrCode.h
    ${qrcodegen_SOURCE_DIR}/cpp/qrcodegen.cpp
)
target_include_directories(presenter-app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(presenter-app PRIVATE ${qrcodegen_SOURCE_DIR}/cpp)
target_link_libraries(presenter-app PUBLIC
    presenter-core presenter-modules presenter-persistence presenter-screens presenter-presentation presenter-library presenter-bible Qt6::Gui Qt6::Network Qt6::Concurrent Qt6::HttpServer Qt6::WebSockets
)
target_link_libraries(presenter-app PRIVATE Qt6::Multimedia)
qt_add_resources(presenter-app "presenter_remote_web"
    PREFIX "/holyscreen/remote"
    BASE remote/web
    FILES
        remote/web/index.html
        remote/web/manifest.webmanifest
        remote/web/sw.js
)
target_link_libraries(presenter-app PRIVATE miniz)
if(WIN32)
    # GetProcessMemoryInfo, usada pela amostragem de memória do endurance.
    target_link_libraries(presenter-app PRIVATE Psapi)
endif()

qt_add_executable(church-presenter
    WIN32
    MACOSX_BUNDLE
    app/main.cpp
)
if(WIN32)
    # Auxiliar sem Qt: ele roda fora do diretório de instalação enquanto o NSIS
    # substitui o aplicativo e o reinicia com o token normal do operador.
    add_library(holyscreen-update-verifier STATIC
        app/WinUpdateVerifier.cpp
        app/WinUpdateVerifier.h
    )
    target_include_directories(holyscreen-update-verifier PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(holyscreen-update-verifier PRIVATE UNICODE _UNICODE)
    target_link_libraries(holyscreen-update-verifier PRIVATE Bcrypt)

    add_executable(holyscreen-update-helper WIN32 app/WinUpdateHelper.cpp)
    target_compile_definitions(holyscreen-update-helper PRIVATE UNICODE _UNICODE)
    target_link_libraries(holyscreen-update-helper PRIVATE holyscreen-update-verifier Shell32)
    set_target_properties(holyscreen-update-helper PROPERTIES
        OUTPUT_NAME "holyscreen-update-helper")
    install(TARGETS holyscreen-update-helper RUNTIME DESTINATION bin)
endif()
target_compile_definitions(church-presenter PRIVATE HOLYSCREEN_VERSION="${PROJECT_VERSION}")
set_target_properties(church-presenter PROPERTIES
    OUTPUT_NAME "holyscreen"
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
    target_sources(church-presenter PRIVATE
        ${CMAKE_SOURCE_DIR}/packaging/windows/holyscreen.rc
        ${CMAKE_SOURCE_DIR}/packaging/windows/holyscreen.ico
    )
endif()
qt_add_library(presenter-ui STATIC)
set_source_files_properties(ui/operator/UiScale.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
qt_add_qml_module(presenter-ui
    URI ChurchPresenter
    VERSION 1.0
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ChurchPresenter"
    QML_FILES
        ui/operator/UiScale.qml
        ui/operator/MainWindow.qml
        ui/operator/OperatorHeader.qml
        ui/operator/EventsDialog.qml
        ui/operator/Dashboard.qml
        ui/operator/BiblePanel.qml
        ui/operator/LyricsPanel.qml
        ui/operator/ThemeEditorDialog.qml
        ui/operator/LibraryPanel.qml
        ui/operator/PlaylistPanel.qml
        ui/operator/PlayerButton.qml
        ui/operator/MediaThumbnail.qml
        ui/operator/PlaybackPanel.qml
        ui/operator/MediaLibraryDialog.qml
        ui/operator/OnlineLyricsDialog.qml
        ui/operator/BibleSettingsFlow.qml
        ui/operator/LiveCommunicationDialog.qml
        ui/operator/MaintenanceDialogs.qml
        ui/operator/MediaImportFlow.qml
        ui/operator/OperatorShortcuts.qml
        ui/operator/SettingsDialog.qml
        ui/operator/BibleBrowser.qml
        ui/operator/BibleComparisonDialog.qml
        ui/operator/QuickBibleSearch.qml
        ui/output/OutputWindow.qml
        ui/output/AudienceView.qml
        ui/output/BroadcastView.qml
        ui/output/StageOutputView.qml
        ui/output/OutputClock.qml
        ui/output/BroadcastPreview.qml
        ui/operator/BroadcastSettings.qml
        ui/operator/IntegrationsArea.qml
        ui/operator/AutomationsArea.qml
        ui/operator/OnboardingDialog.qml
        ui/operator/EventsArea.qml
        ui/operator/MaintenanceArea.qml
        ui/output/SimulatedOutput.qml
        ui/output/PresentationImageLayer.qml
        ui/output/PresentationTextLayer.qml
        ui/output/StageView.qml
        ui/output/LiveOverlays.qml
)
target_link_libraries(presenter-ui PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Multimedia)

# Preserva o nome do alvo de lint usado pela documentação e pela CI depois de
# mover os arquivos QML para a biblioteca presenter-ui.
add_custom_target(church-presenter_qmllint)
add_dependencies(church-presenter_qmllint presenter-ui_qmllint)

target_link_libraries(church-presenter
    PRIVATE
        Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2 Qt6::Sql Qt6::Multimedia
        Qt6::HttpServer Qt6::WebSockets
        presenter-app presenter-screens presenter-presentation presenter-persistence presenter-library
        presenter-bible presenter-ui presenter-uiplugin
)
