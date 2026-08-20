qt_add_executable(test_command_bus unit/core/CommandBusTest.cpp)
target_link_libraries(test_command_bus PRIVATE Qt6::Test presenter-core)
add_test(NAME test_command_bus COMMAND test_command_bus)

qt_add_executable(test_event_bus unit/core/EventBusTest.cpp)
target_link_libraries(test_event_bus PRIVATE Qt6::Test presenter-core)
add_test(NAME test_event_bus COMMAND test_event_bus)

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

qt_add_executable(test_local_api_server unit/remote/LocalApiServerTest.cpp)
target_link_libraries(test_local_api_server PRIVATE Qt6::Test presenter-app)
add_test(NAME test_local_api_server COMMAND test_local_api_server)

qt_add_executable(test_bible_reference_parser unit/bible/BibleReferenceParserTest.cpp)
target_link_libraries(test_bible_reference_parser PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_reference_parser COMMAND test_bible_reference_parser)

qt_add_executable(test_bible_repository unit/bible/BibleRepositoryTest.cpp)
target_link_libraries(test_bible_repository PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_repository COMMAND test_bible_repository)

qt_add_executable(test_bible_json_importer unit/bible/BibleJsonImporterTest.cpp)
target_link_libraries(test_bible_json_importer PRIVATE Qt6::Test presenter-bible)
add_test(NAME test_bible_json_importer COMMAND test_bible_json_importer)

add_test(NAME test_app_starts COMMAND church-presenter --quit-after-startup)
set_tests_properties(test_app_starts PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    TIMEOUT 15
)
