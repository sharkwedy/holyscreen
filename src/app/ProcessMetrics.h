#pragma once

#include <QString>

#include <QtGlobal>

namespace churchpresenter {

//! Amostra instantânea do custo do processo. Os campos ficam em zero quando o
//! sistema não expõe a informação, para que o relatório de endurance registre
//! a ausência em vez de inventar um número.
struct ProcessSample {
    quint64 residentBytes = 0;
    double cpuSeconds = 0.0;
};

//! Leituras de processo e de ambiente usadas pelo modo de endurance. Cada
//! sistema tem o seu ramo nativo e nenhuma dependência nova é introduzida.
class ProcessMetrics final {
public:
    [[nodiscard]] static ProcessSample sample();
    [[nodiscard]] static QString operatingSystem();
    [[nodiscard]] static QString kernel();
    [[nodiscard]] static int logicalCores();
    [[nodiscard]] static quint64 totalMemoryBytes();
};

} // namespace churchpresenter
