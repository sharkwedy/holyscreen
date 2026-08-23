#include "app/contexts/OutputContext.h"

#include "app/ApplicationController.h"

namespace churchpresenter {

OutputContext::OutputContext(ApplicationController &controller, QObject *parent)
    : QObject(parent), m_controller(controller)
{
    connect(&controller, &ApplicationController::screensChanged,
            this, &OutputContext::screensChanged);
    connect(&controller, &ApplicationController::outputWindowsChanged,
            this, &OutputContext::outputWindowsChanged);
    connect(&controller, &ApplicationController::blackoutChanged,
            this, &OutputContext::blackoutChanged);
    connect(&controller, &ApplicationController::identifyVisibleChanged,
            this, &OutputContext::identifyVisibleChanged);
}

QVariantList OutputContext::screens() const { return m_controller.screens(); }
QVariantList OutputContext::outputWindows() const { return m_controller.outputWindows(); }
bool OutputContext::blackout() const { return m_controller.blackout(); }
void OutputContext::setBlackout(bool enabled) { m_controller.setBlackout(enabled); }
bool OutputContext::identifyVisible() const { return m_controller.identifyVisible(); }
bool OutputContext::broadcastTransparencySupported() const { return m_controller.broadcastTransparencySupported(); }
QString OutputContext::broadcastTransparencyWarning() const { return m_controller.broadcastTransparencyWarning(); }
bool OutputContext::toggleScreen(const QString &id, bool enabled) { return m_controller.toggleScreen(id, enabled); }
void OutputContext::enableAllScreens() { m_controller.enableAllScreens(); }
bool OutputContext::setOutputBibleTranslation(const QString &id, const QString &translationId) { return m_controller.setOutputBibleTranslation(id, translationId); }
bool OutputContext::setOutputRole(const QString &id, const QString &role) { return m_controller.setOutputRole(id, role); }
bool OutputContext::setOutputMediaEnabled(const QString &id, bool enabled) { return m_controller.setOutputMediaEnabled(id, enabled); }
QVariantMap OutputContext::outputBroadcastProfile(const QString &id) const { return m_controller.outputBroadcastProfile(id); }
bool OutputContext::setOutputBroadcastProfile(const QString &id, const QVariantMap &changes) { return m_controller.setOutputBroadcastProfile(id, changes); }
bool OutputContext::setOutputDisplayName(const QString &id, const QString &name) { return m_controller.setOutputDisplayName(id, name); }
void OutputContext::identifyScreens() { m_controller.identifyScreens(); }

} // namespace churchpresenter
