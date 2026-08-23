import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "OutputViews"
    width: 640
    height: 360
    visible: true
    when: windowShown

    Component {
        id: audienceComponent
        AudienceView {
            width: 640
            height: 360
            controller: presentationController
            bibleTranslationId: "nvi"
        }
    }

    Component {
        id: stageComponent
        StageOutputView {
            width: 640
            height: 360
            controller: presentationController
            bibleTranslationId: "naa"
        }
    }

    Component {
        id: broadcastComponent
        BroadcastView {
            width: 640
            height: 360
            controller: presentationController
        }
    }

    Component {
        id: windowComponent
        OutputWindow {
            targetScreenIndex: -1
            outputDisplayName: "Teste"
        }
    }

    function test_audience_view_renders_without_errors() {
        const view = createTemporaryObject(audienceComponent, testCase)
        verify(view, "a saída de público deve ser instanciável")
        compare(view.mediaEnabled, true)
    }

    function test_stage_view_shows_the_current_and_next_slide() {
        presentationController.currentPresentationType = "song"
        presentationController.currentSlideText = "Verso atual"
        presentationController.nextSlideText = "Próximo verso"
        const view = createTemporaryObject(stageComponent, testCase)
        verify(view, "a saída de palco deve ser instanciável")
        const stage = view.children[1]
        compare(stage.currentText, "Verso atual")
        compare(stage.nextText, "Próximo verso")
    }

    function test_stage_view_uses_the_translation_of_the_output() {
        presentationController.currentPresentationType = "bible"
        presentationController.currentSlideIndex = 3
        const view = createTemporaryObject(stageComponent, testCase)
        const stage = view.children[1]
        compare(stage.currentText, "naa#3")
        compare(stage.nextText, "naa#4")
        presentationController.currentPresentationType = "song"
    }

    function test_audience_view_paints_the_configured_wallpaper_color() {
        presentationController.blackout = false
        presentationController.wallpaperColor = "#123456"
        const view = createTemporaryObject(audienceComponent, testCase)
        waitForRendering(view)
        const image = grabImage(view)
        compare(image.pixel(10, 10), Qt.rgba(0x12 / 255, 0x34 / 255, 0x56 / 255, 1))

        presentationController.blackout = true
        wait(0)
        const blacked = grabImage(view)
        compare(blacked.pixel(10, 10), Qt.rgba(0, 0, 0, 1))
        presentationController.blackout = false
    }

    function test_stage_view_keeps_its_own_background_during_blackout() {
        presentationController.blackout = true
        const view = createTemporaryObject(stageComponent, testCase)
        waitForRendering(view)
        const image = grabImage(view)
        compare(image.pixel(10, 10), Qt.rgba(0x05 / 255, 0x07 / 255, 0x0b / 255, 1))
        presentationController.blackout = false
    }

    function test_broadcast_view_paints_its_own_background() {
        const view = createTemporaryObject(broadcastComponent, testCase)
        verify(view, "a saída de transmissão deve ser instanciável")
        compare(String(view.backgroundColor), "#000000")
    }

    function test_output_window_routes_each_role_to_its_renderer() {
        const window = createTemporaryObject(windowComponent, testCase)
        verify(window, "a janela de saída deve ser instanciável")

        window.outputRole = "audience"
        verify(String(window.roleView).indexOf("AudienceView") === 0,
               "papel audience deve carregar AudienceView, carregou " + window.roleView)

        window.outputRole = "stage"
        verify(String(window.roleView).indexOf("StageOutputView") === 0,
               "papel stage deve carregar StageOutputView, carregou " + window.roleView)

        window.outputRole = "broadcast"
        verify(String(window.roleView).indexOf("BroadcastView") === 0,
               "papel broadcast deve carregar BroadcastView, carregou " + window.roleView)

        window.outputRole = "confidence"
        verify(String(window.roleView).indexOf("AudienceView") === 0,
               "confidence ainda usa a composição de público, carregou " + window.roleView)

        window.outputRole = "custom"
        verify(String(window.roleView).indexOf("AudienceView") === 0,
               "custom ainda usa a composição de público, carregou " + window.roleView)
    }
}
