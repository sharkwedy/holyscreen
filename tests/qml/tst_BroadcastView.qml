import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "BroadcastView"
    width: 640
    height: 360
    visible: true
    when: windowShown

    Component {
        id: broadcastComponent
        BroadcastView {
            width: 640
            height: 360
            controller: presentationController
        }
    }

    function init() {
        presentationController.blackout = false
        presentationController.audienceMessage = "Aviso"
        presentationController.alertMessage = "Alerta"
        presentationController.lowerThirdTitle = "Pastor"
        presentationController.lowerThirdSubtitle = "Mensagem"
    }

    function cleanup() {
        presentationController.audienceMessage = ""
        presentationController.alertMessage = ""
        presentationController.lowerThirdTitle = ""
        presentationController.lowerThirdSubtitle = ""
        presentationController.blackout = false
    }

    function test_chroma_mode_fills_the_output_with_the_configured_color() {
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"backgroundMode": "chroma", "chromaColor": "#00b140"}
        })
        waitForRendering(view)
        const image = grabImage(view)
        compare(image.pixel(2, 2), Qt.rgba(0x00 / 255, 0xb1 / 255, 0x40 / 255, 1))
    }

    function test_transparent_mode_leaves_the_background_unpainted() {
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"backgroundMode": "transparent"}
        })
        const background = findChild(view, "broadcastBackground")
        verify(background, "o fundo de chroma deve existir como item")
        verify(!background.visible, "no modo transparente nada é pintado atrás")
        verify(!view.transparentBackground === false,
               "a view precisa reportar o modo transparente para a janela")

        view.profile = {"backgroundMode": "chroma", "chromaColor": "#00b140"}
        verify(background.visible, "no modo chroma o fundo volta a ser pintado")
        compare(String(background.color), "#00b140")
        verify(!view.transparentBackground)
    }

    function test_blackout_keeps_the_background_and_hides_the_overlays() {
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"backgroundMode": "chroma", "chromaColor": "#0047bb"}
        })
        const overlays = findChild(view, "broadcastOverlays")
        verify(overlays, "a camada de overlays deve existir")
        verify(overlays.visible, "sem blackout os overlays ficam visíveis")

        presentationController.blackout = true
        wait(0)
        verify(!overlays.visible, "o blackout esconde os overlays da transmissão")
        waitForRendering(view)
        const image = grabImage(view)
        compare(image.pixel(2, 2), Qt.rgba(0x00 / 255, 0x47 / 255, 0xbb / 255, 1))

        presentationController.blackout = false
        wait(0)
        verify(overlays.visible, "sair do blackout restaura os overlays")
    }

    function test_safe_area_follows_the_preset_and_the_configured_margins() {
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"aspectPreset": "16:9", "aspectRatio": 16 / 9,
                      "safeAreaLeft": 10, "safeAreaRight": 10,
                      "safeAreaTop": 5, "safeAreaBottom": 5}
        })
        waitForRendering(view)
        const safeArea = findChild(view, "broadcastSafeArea")
        verify(safeArea, "a zona segura deve existir")
        // 640x360 já é 16:9, então a caixa de composição ocupa a tela inteira.
        compare(safeArea.width, 640 - 640 * 0.2)
        compare(safeArea.height, 360 - 360 * 0.1)

        view.profile = {"aspectPreset": "9:16", "aspectRatio": 9 / 16,
                        "safeAreaLeft": 0, "safeAreaRight": 0,
                        "safeAreaTop": 0, "safeAreaBottom": 0}
        waitForRendering(view)
        compare(safeArea.height, 360)
        compare(safeArea.width, 360 * 9 / 16)
    }

    function test_overlay_switches_hide_only_what_was_disabled() {
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"showAudienceMessage": false, "showAlerts": true,
                      "showLowerThird": false}
        })
        const overlays = findChild(view, "broadcastOverlays")
        compare(overlays.message, "")
        compare(overlays.alertMessage, "Alerta")
        compare(overlays.lowerThirdTitle, "")
        compare(overlays.lowerThirdSubtitle, "")

        view.profile = {"showAudienceMessage": true, "showAlerts": false,
                        "showLowerThird": true}
        compare(overlays.message, "Aviso")
        compare(overlays.alertMessage, "")
        compare(overlays.lowerThirdTitle, "Pastor")
    }

    function test_clock_follows_the_profile_switch() {
        presentationController.clockVisible = true
        const view = createTemporaryObject(broadcastComponent, testCase, {
            profile: {"showClock": false}
        })
        const clock = findChild(view, "broadcastSafeArea").children[1]
        verify(!clock.visible, "sem showClock o relógio não aparece na transmissão")

        view.profile = {"showClock": true}
        verify(clock.visible, "com showClock o relógio aparece na transmissão")
        presentationController.clockVisible = false
    }
}
