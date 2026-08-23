#pragma once

#include "integrations/ports/ITransports.h"

#include <QList>
#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace churchpresenter {

//! Executa o processo já validado. Nunca usa shell: o programa e a lista de
//! argumentos vão direto para o sistema operacional.
class QtProcessRunner final : public QObject, public IProcessRunner {
    Q_OBJECT

public:
    explicit QtProcessRunner(QObject *parent = nullptr);
    ~QtProcessRunner() override;

    void run(const ProcessRequest &request, Completion completion) override;
    void cancelAll() override;

private:
    QList<QPointer<QProcess>> m_running;
};

} // namespace churchpresenter
