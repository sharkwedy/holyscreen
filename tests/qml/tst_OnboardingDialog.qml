import QtQuick
import QtTest
import ChurchPresenter

TestCase {
    id: testCase
    name: "OnboardingDialog"
    width: 900
    height: 700
    visible: true
    when: windowShown

    QtObject {
        id: fakeOutputContext
        property var outputWindows: []
        property var screens: []
    }

    QtObject {
        id: fakeMediaContext
        property bool audioOutputConfigured: false
        property var mediaFolders: []
    }

    QtObject {
        id: fakeBibleContext
        property var bibleTranslations: []
    }

    QtObject {
        id: fakeController
        property var outputContext: fakeOutputContext
        property var mediaContext: fakeMediaContext
        property var bibleContext: fakeBibleContext
        property bool remotePasswordConfigured: false
        property bool onboardingCompleted: true
        property var onboardingSkippedSteps: []
        signal onboardingChanged()

        function completeOnboarding() {
            onboardingCompleted = true
            onboardingChanged()
        }

        function skipOnboardingStep(stepId) {
            if (onboardingSkippedSteps.indexOf(stepId) >= 0)
                return false
            onboardingSkippedSteps = onboardingSkippedSteps.concat([stepId])
            onboardingChanged()
            return true
        }

        function resumeOnboardingStep(stepId) {
            const index = onboardingSkippedSteps.indexOf(stepId)
            if (index < 0)
                return false
            const updated = onboardingSkippedSteps.slice()
            updated.splice(index, 1)
            onboardingSkippedSteps = updated
            onboardingChanged()
            return true
        }
    }

    OnboardingDialog {
        id: onboarding
        parent: testCase
        controller: fakeController
    }

    SignalSpy {
        id: settingsSpy
        target: onboarding
        signalName: "openSettings"
    }

    SignalSpy {
        id: bibleSpy
        target: onboarding
        signalName: "openBible"
    }

    function init() {
        onboarding.close()
        settingsSpy.clear()
        bibleSpy.clear()
        fakeOutputContext.outputWindows = []
        fakeOutputContext.screens = []
        fakeMediaContext.audioOutputConfigured = false
        fakeMediaContext.mediaFolders = []
        fakeBibleContext.bibleTranslations = []
        fakeController.remotePasswordConfigured = false
        fakeController.onboardingCompleted = true
        fakeController.onboardingSkippedSteps = []
    }

    function test_reportsEveryChecklistState() {
        verify(!onboarding.stepComplete("screens"))
        verify(!onboarding.stepComplete("audio"))
        verify(!onboarding.stepComplete("library"))
        verify(!onboarding.stepComplete("bible"))
        verify(!onboarding.stepComplete("remote"))
        verify(!onboarding.stepComplete("broadcast"))

        fakeOutputContext.outputWindows = [{"identifier": "audience"}]
        fakeOutputContext.screens = [{"role": "audience"}, {"role": "broadcast"}]
        fakeMediaContext.audioOutputConfigured = true
        fakeMediaContext.mediaFolders = ["C:/media"]
        fakeBibleContext.bibleTranslations = [{"id": "arc"}]
        fakeController.remotePasswordConfigured = true

        verify(onboarding.stepComplete("screens"))
        verify(onboarding.stepComplete("audio"))
        verify(onboarding.stepComplete("library"))
        verify(onboarding.stepComplete("bible"))
        verify(onboarding.stepComplete("remote"))
        verify(onboarding.stepComplete("broadcast"))
    }

    function test_routesEachStepToItsConfiguration() {
        const expectedTabs = [1, 2, 0, -1, 4, 1]
        for (let index = 0; index < expectedTabs.length; ++index) {
            onboarding.currentStep = index
            onboarding.configureCurrentStep()
            if (index === 3) {
                compare(bibleSpy.count, 1)
                compare(settingsSpy.count, index)
            } else {
                compare(settingsSpy.signalArguments[index < 3 ? index : index - 1][0],
                        expectedTabs[index])
            }
        }
        compare(settingsSpy.count, 5)
        compare(bibleSpy.count, 1)
    }

    function test_selectsFirstIncompleteStep() {
        fakeOutputContext.outputWindows = [{"identifier": "audience"}]
        fakeMediaContext.audioOutputConfigured = true
        compare(onboarding.firstIncompleteStep(), 2)

        fakeMediaContext.mediaFolders = ["C:/media"]
        fakeBibleContext.bibleTranslations = [{"id": "arc"}]
        fakeController.remotePasswordConfigured = true
        fakeOutputContext.screens = [{"role": "broadcast"}]
        compare(onboarding.firstIncompleteStep(), onboarding.steps.length - 1)
    }

    function test_skippedStepsAreIgnoredUntilResumed() {
        verify(fakeController.skipOnboardingStep("screens"))
        verify(onboarding.stepSkipped("screens"))
        compare(onboarding.firstIncompleteStep(), 1)

        onboarding.currentStep = 0
        onboarding.configureCurrentStep()
        verify(!onboarding.stepSkipped("screens"))
        compare(settingsSpy.signalArguments[0][0], 1)
    }
}
