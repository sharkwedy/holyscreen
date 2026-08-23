qt_add_executable(test_command_bus unit/core/CommandBusTest.cpp)
target_link_libraries(test_command_bus PRIVATE Qt6::Test presenter-core)
add_test(NAME test_command_bus COMMAND test_command_bus)

qt_add_executable(test_command_catalog unit/core/CommandCatalogTest.cpp)
target_link_libraries(test_command_catalog PRIVATE Qt6::Test presenter-core)
add_test(NAME test_command_catalog COMMAND test_command_catalog)

qt_add_executable(test_event_bus unit/core/EventBusTest.cpp)
target_link_libraries(test_event_bus PRIVATE Qt6::Test presenter-core)
add_test(NAME test_event_bus COMMAND test_event_bus)

qt_add_executable(test_undo_manager unit/core/UndoManagerTest.cpp)
target_link_libraries(test_undo_manager PRIVATE Qt6::Test presenter-core)
add_test(NAME test_undo_manager COMMAND test_undo_manager)

qt_add_executable(test_output_module unit/modules/OutputModuleTest.cpp)
target_link_libraries(test_output_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_output_module COMMAND test_output_module)

qt_add_executable(test_bible_command_module unit/modules/BibleCommandModuleTest.cpp)
target_link_libraries(test_bible_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_bible_command_module COMMAND test_bible_command_module)

qt_add_executable(test_event_command_module unit/modules/EventCommandModuleTest.cpp)
target_link_libraries(test_event_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_event_command_module COMMAND test_event_command_module)

qt_add_executable(test_theme_command_module unit/modules/ThemeCommandModuleTest.cpp)
target_link_libraries(test_theme_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_theme_command_module COMMAND test_theme_command_module)

qt_add_executable(test_playlist_command_module unit/modules/PlaylistCommandModuleTest.cpp)
target_link_libraries(test_playlist_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_playlist_command_module COMMAND test_playlist_command_module)

qt_add_executable(test_output_state_module unit/modules/OutputStateModuleTest.cpp)
target_link_libraries(test_output_state_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_output_state_module COMMAND test_output_state_module)

qt_add_executable(test_output_routing_command_module unit/modules/OutputRoutingCommandModuleTest.cpp)
target_link_libraries(test_output_routing_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_output_routing_command_module COMMAND test_output_routing_command_module)

qt_add_executable(test_presentation_command_module unit/modules/PresentationCommandModuleTest.cpp)
target_link_libraries(test_presentation_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_presentation_command_module COMMAND test_presentation_command_module)

qt_add_executable(test_stage_command_module unit/modules/StageCommandModuleTest.cpp)
target_link_libraries(test_stage_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_stage_command_module COMMAND test_stage_command_module)

qt_add_executable(test_overlay_command_module unit/modules/OverlayCommandModuleTest.cpp)
target_link_libraries(test_overlay_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_overlay_command_module COMMAND test_overlay_command_module)

qt_add_executable(test_media_command_module unit/modules/MediaCommandModuleTest.cpp)
target_link_libraries(test_media_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_media_command_module COMMAND test_media_command_module)

qt_add_executable(test_undo_command_module unit/modules/UndoCommandModuleTest.cpp)
target_link_libraries(test_undo_command_module PRIVATE Qt6::Test presenter-modules)
add_test(NAME test_undo_command_module COMMAND test_undo_command_module)

qt_add_executable(test_screens
    unit/screens/OutputManagerTest.cpp
)
target_link_libraries(test_screens PRIVATE Qt6::Test presenter-screens)
add_test(NAME test_screens COMMAND test_screens)

qt_add_executable(test_screen_manager
    unit/screens/ScreenManagerTest.cpp
)
target_link_libraries(test_screen_manager PRIVATE Qt6::Test presenter-screens)
add_test(NAME test_screen_manager COMMAND test_screen_manager)

qt_add_executable(test_output_role unit/screens/OutputRoleTest.cpp)
target_link_libraries(test_output_role PRIVATE Qt6::Test presenter-screens)
add_test(NAME test_output_role COMMAND test_output_role)

qt_add_executable(test_broadcast_profile unit/screens/BroadcastProfileTest.cpp)
target_link_libraries(test_broadcast_profile PRIVATE Qt6::Test presenter-screens)
add_test(NAME test_broadcast_profile COMMAND test_broadcast_profile)

qt_add_executable(test_output_routing unit/screens/OutputRoutingTest.cpp)
target_link_libraries(test_output_routing PRIVATE Qt6::Test presenter-screens)
add_test(NAME test_output_routing COMMAND test_output_routing)

qt_add_executable(test_presentation
    unit/presentation/PresentationControllerTest.cpp
)
target_link_libraries(test_presentation PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_presentation COMMAND test_presentation)

qt_add_executable(test_clock
    unit/presentation/ClockTest.cpp
)
target_link_libraries(test_clock PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_clock COMMAND test_clock)

qt_add_executable(test_audio_engine
    unit/presentation/AudioEngineTest.cpp
)
target_link_libraries(test_audio_engine PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_audio_engine COMMAND test_audio_engine)

qt_add_executable(test_video_frame_bus
    unit/presentation/VideoFrameBusTest.cpp
)
target_link_libraries(test_video_frame_bus PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_video_frame_bus COMMAND test_video_frame_bus)

qt_add_executable(test_video_engine
    unit/presentation/VideoEngineTest.cpp
)
target_link_libraries(test_video_engine PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_video_engine COMMAND test_video_engine)

qt_add_executable(test_image_presentation
    unit/presentation/ImagePresentationControllerTest.cpp
)
target_link_libraries(test_image_presentation PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_image_presentation COMMAND test_image_presentation)

qt_add_executable(test_text_presentation
    unit/presentation/TextPresentationControllerTest.cpp
)
target_link_libraries(test_text_presentation PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_text_presentation COMMAND test_text_presentation)

qt_add_executable(test_timed_media_playback unit/presentation/TimedMediaPlaybackTest.cpp)
target_link_libraries(test_timed_media_playback PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_timed_media_playback COMMAND test_timed_media_playback)

qt_add_executable(test_overlay_controller unit/presentation/OverlayControllerTest.cpp)
target_link_libraries(test_overlay_controller PRIVATE Qt6::Test presenter-presentation)
add_test(NAME test_overlay_controller COMMAND test_overlay_controller)

qt_add_executable(test_integration_engine unit/integrations/IntegrationEngineTest.cpp)
target_link_libraries(test_integration_engine PRIVATE Qt6::Test presenter-integrations)
add_test(NAME test_integration_engine COMMAND test_integration_engine)

qt_add_executable(test_http_integration_adapter unit/integrations/HttpIntegrationAdapterTest.cpp)
target_link_libraries(test_http_integration_adapter PRIVATE Qt6::Test Qt6::HttpServer presenter-integrations)
add_test(NAME test_http_integration_adapter COMMAND test_http_integration_adapter)

qt_add_executable(test_websocket_integration_adapter unit/integrations/WebSocketIntegrationAdapterTest.cpp)
target_link_libraries(test_websocket_integration_adapter PRIVATE Qt6::Test Qt6::WebSockets presenter-integrations)
add_test(NAME test_websocket_integration_adapter COMMAND test_websocket_integration_adapter)

qt_add_executable(test_osc_integration_adapter unit/integrations/OscIntegrationAdapterTest.cpp)
target_link_libraries(test_osc_integration_adapter PRIVATE Qt6::Test Qt6::Network presenter-integrations)
add_test(NAME test_osc_integration_adapter COMMAND test_osc_integration_adapter)

qt_add_executable(test_midi_integration_adapter unit/integrations/MidiIntegrationAdapterTest.cpp)
target_link_libraries(test_midi_integration_adapter PRIVATE Qt6::Test presenter-integrations)
add_test(NAME test_midi_integration_adapter COMMAND test_midi_integration_adapter)

qt_add_executable(test_secret_store unit/integrations/SecretStoreTest.cpp)
target_link_libraries(test_secret_store PRIVATE Qt6::Test presenter-integrations)
add_test(NAME test_secret_store COMMAND test_secret_store)

qt_add_executable(test_persistence
    unit/persistence/SettingsRepositoryTest.cpp
)
target_link_libraries(test_persistence PRIVATE Qt6::Test presenter-persistence)
add_test(NAME test_persistence COMMAND test_persistence)

qt_add_executable(test_database_migrator unit/persistence/DatabaseMigratorTest.cpp)
target_link_libraries(test_database_migrator PRIVATE Qt6::Test presenter-persistence)
add_test(NAME test_database_migrator COMMAND test_database_migrator)

qt_add_executable(test_application_database unit/persistence/ApplicationDatabaseTest.cpp)
target_link_libraries(test_application_database PRIVATE Qt6::Test presenter-persistence)
add_test(NAME test_application_database COMMAND test_application_database)

qt_add_executable(test_broadcast_profile_repository unit/library/BroadcastProfileRepositoryTest.cpp)
target_link_libraries(test_broadcast_profile_repository PRIVATE Qt6::Test presenter-library presenter-persistence)
add_test(NAME test_broadcast_profile_repository COMMAND test_broadcast_profile_repository)

qt_add_executable(test_integration_repository unit/library/IntegrationRepositoryTest.cpp)
target_link_libraries(test_integration_repository PRIVATE Qt6::Test presenter-library presenter-persistence)
add_test(NAME test_integration_repository COMMAND test_integration_repository)

qt_add_executable(test_media_repository
    unit/library/MediaRepositoryTest.cpp
)
target_link_libraries(test_media_repository PRIVATE Qt6::Test presenter-library)
add_test(NAME test_media_repository COMMAND test_media_repository)

qt_add_executable(test_media_folder_scanner unit/library/MediaFolderScannerTest.cpp)
target_link_libraries(test_media_folder_scanner PRIVATE Qt6::Test presenter-library)
add_test(NAME test_media_folder_scanner COMMAND test_media_folder_scanner)

qt_add_executable(test_presentation_repository
    unit/library/PresentationRepositoryTest.cpp
)
target_link_libraries(test_presentation_repository PRIVATE Qt6::Test presenter-library)
add_test(NAME test_presentation_repository COMMAND test_presentation_repository)

qt_add_executable(test_theme_repository unit/library/ThemeRepositoryTest.cpp)
target_link_libraries(test_theme_repository PRIVATE Qt6::Test presenter-library)
add_test(NAME test_theme_repository COMMAND test_theme_repository)

qt_add_executable(test_event_repository unit/library/EventRepositoryTest.cpp)
target_link_libraries(test_event_repository PRIVATE Qt6::Test presenter-library)
add_test(NAME test_event_repository COMMAND test_event_repository)

qt_add_executable(test_history_repository unit/library/HistoryRepositoryTest.cpp)
target_link_libraries(test_history_repository PRIVATE Qt6::Test presenter-library)
add_test(NAME test_history_repository COMMAND test_history_repository)

qt_add_executable(test_data_recovery unit/app/DataRecoveryServiceTest.cpp)
target_link_libraries(test_data_recovery PRIVATE Qt6::Test presenter-app)
add_test(NAME test_data_recovery COMMAND test_data_recovery)

qt_add_executable(test_autosave_coordinator unit/app/AutosaveCoordinatorTest.cpp)
target_link_libraries(test_autosave_coordinator PRIVATE Qt6::Test presenter-app)
add_test(NAME test_autosave_coordinator COMMAND test_autosave_coordinator)

qt_add_executable(test_application_command_bridge unit/app/ApplicationCommandBridgeTest.cpp)
target_link_libraries(test_application_command_bridge PRIVATE Qt6::Test presenter-app)
add_test(NAME test_application_command_bridge COMMAND test_application_command_bridge)
set_tests_properties(test_application_command_bridge PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 15
)

qt_add_executable(test_app_logger unit/app/AppLoggerTest.cpp)
target_link_libraries(test_app_logger PRIVATE Qt6::Test presenter-app)
add_test(NAME test_app_logger COMMAND test_app_logger)

qt_add_executable(test_diagnostic_exporter unit/app/DiagnosticExporterTest.cpp)
target_link_libraries(test_diagnostic_exporter PRIVATE Qt6::Test presenter-app miniz)
add_test(NAME test_diagnostic_exporter COMMAND test_diagnostic_exporter)

qt_add_executable(test_local_api_server unit/remote/LocalApiServerTest.cpp)
target_link_libraries(test_local_api_server PRIVATE Qt6::Test presenter-app)
add_test(NAME test_local_api_server COMMAND test_local_api_server)

qt_add_executable(test_remote_auth_manager unit/remote/RemoteAuthManagerTest.cpp)
target_link_libraries(test_remote_auth_manager PRIVATE Qt6::Test presenter-app)
add_test(NAME test_remote_auth_manager COMMAND test_remote_auth_manager)

qt_add_executable(test_remote_qr_code unit/remote/RemoteQrCodeTest.cpp)
target_link_libraries(test_remote_qr_code PRIVATE Qt6::Test presenter-app)
add_test(NAME test_remote_qr_code COMMAND test_remote_qr_code)

qt_add_executable(test_bible_reference_parser unit/bible/BibleReferenceParserTest.cpp)
target_link_libraries(test_bible_reference_parser PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_reference_parser COMMAND test_bible_reference_parser)

qt_add_executable(test_path_containment unit/bible/PathContainmentTest.cpp)
target_link_libraries(test_path_containment PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_path_containment COMMAND test_path_containment)

qt_add_executable(test_bible_repository unit/bible/BibleRepositoryTest.cpp)
target_link_libraries(test_bible_repository PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_repository COMMAND test_bible_repository)

qt_add_executable(test_bible_json_importer unit/bible/BibleJsonImporterTest.cpp)
target_link_libraries(test_bible_json_importer PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_json_importer COMMAND test_bible_json_importer)

qt_add_executable(test_canonical_bible_importer unit/bible/CanonicalBibleImporterTest.cpp)
target_link_libraries(test_canonical_bible_importer PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_canonical_bible_importer COMMAND test_canonical_bible_importer)

qt_add_executable(test_bible_source_stager unit/bible/BibleSourceStagerTest.cpp)
target_link_libraries(test_bible_source_stager PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_source_stager COMMAND test_bible_source_stager)

qt_add_executable(test_bible_import_service unit/bible/BibleImportServiceTest.cpp)
target_link_libraries(test_bible_import_service PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_import_service COMMAND test_bible_import_service)

qt_add_executable(test_qml_output_views qml/main.cpp qml/FakePresentationController.h)
target_include_directories(test_qml_output_views PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/qml)
target_link_libraries(test_qml_output_views PRIVATE
    Qt6::QuickTest Qt6::Qml Qt6::Quick presenter-ui presenter-uiplugin)
add_test(NAME test_qml_output_views
    COMMAND test_qml_output_views -input ${CMAKE_CURRENT_SOURCE_DIR}/qml)
set_tests_properties(test_qml_output_views PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 60
)

add_test(NAME test_app_starts COMMAND $<TARGET_FILE:church-presenter> --quit-after-startup)
set_tests_properties(test_app_starts PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 15
)
