#include "FakePresentationController.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

//! Disponibiliza o controlador falso como propriedade de contexto, do mesmo
//! modo que o executável faz com o ApplicationController real.
class QmlTestSetup final : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void qmlEngineAvailable(QQmlEngine *engine)
    {
        engine->rootContext()->setContextProperty(QStringLiteral("presentationController"),
                                                  &m_controller);
    }

private:
    FakePresentationController m_controller;
};

QUICK_TEST_MAIN_WITH_SETUP(holyscreen_qml, QmlTestSetup)

#include "main.moc"
