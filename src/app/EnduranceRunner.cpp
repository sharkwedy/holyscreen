#include "app/EnduranceRunner.h"

#include "app/AppLogger.h"
#include "app/ApplicationController.h"
#include "core/CommandBus.h"
#include "core/EventBus.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QSaveFile>
#include <QSysInfo>
#include <QUrl>
#include <QUuid>

#include <algorithm>
#include <cmath>

namespace churchpresenter {
namespace {

//! Um comando só é considerado travado quando a posição não avança por esta
//! quantidade de amostras consecutivas de um segundo.
constexpr int kStallSamples = 6;
//! Acima deste atraso máximo o operador percebe o congelamento na projeção.
constexpr double kMaxLagMs = 1000.0;
constexpr double kMaxP95LagMs = 250.0;
//! O crescimento só vira bloqueador quando é grande em proporção e em valor
//! absoluto, e apenas em execuções longas o bastante para haver tendência.
constexpr double kMaxResidentGrowthRatio = 1.25;
constexpr quint64 kMaxResidentGrowthBytes = 192ull * 1024 * 1024;
constexpr int kGrowthMinimumSamples = 60;
constexpr int kLagMinimumSamples = 20;
//! Uma automação saudável não encadeia dezenas de eventos na mesma correlação.
constexpr quint64 kCorrelationLimit = 64;

double percentile(QList<double> values, double fraction)
{
    if (values.isEmpty()) return 0.0;
    std::sort(values.begin(), values.end());
    const auto index = static_cast<qsizetype>(std::llround(fraction * (values.size() - 1)));
    return values.at(std::clamp<qsizetype>(index, 0, values.size() - 1));
}

double average(const QList<double> &values)
{
    if (values.isEmpty()) return 0.0;
    double total = 0.0;
    for (const auto value : values) total += value;
    return total / values.size();
}

quint64 averageOf(const QList<quint64> &values, qsizetype from, qsizetype count)
{
    if (values.isEmpty() || count <= 0) return 0;
    const auto begin = std::clamp<qsizetype>(from, 0, values.size() - 1);
    const auto end = std::min<qsizetype>(begin + count, values.size());
    if (end <= begin) return 0;
    quint64 total = 0;
    for (auto index = begin; index < end; ++index) total += values.at(index);
    return total / static_cast<quint64>(end - begin);
}

} // namespace

EnduranceRunner::EnduranceRunner(ApplicationController &controller, EnduranceOptions options,
                                 QObject *parent)
    : QObject(parent)
    , m_controller(controller)
    , m_options(std::move(options))
    , m_random(m_options.seed)
{
    m_actionTimer.setTimerType(Qt::PreciseTimer);
    m_sampleTimer.setTimerType(Qt::PreciseTimer);
    m_lagTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_actionTimer, &QTimer::timeout, this, &EnduranceRunner::performAction);
    connect(&m_sampleTimer, &QTimer::timeout, this, &EnduranceRunner::takeSample);
    connect(&m_lagTimer, &QTimer::timeout, this, [this] {
        // O atraso é a diferença entre o intervalo pedido e o intervalo real:
        // ela cresce sempre que o event loop fica bloqueado.
        const auto elapsed = static_cast<double>(m_lagClock.restart());
        m_lagSamples.append(std::max(0.0, elapsed - m_lagTimer.interval()));
    });
    connect(&m_controller.eventBus(), &EventBus::eventPublished,
            this, &EnduranceRunner::observeEvent);
}

void EnduranceRunner::start()
{
    m_startedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_warningBaseline = AppLogger::warningCount();
    m_criticalBaseline = AppLogger::criticalCount();
    m_fatalBaseline = AppLogger::fatalCount();

    prepareContent();

    m_baselineOutputs = m_controller.outputWindows().size();
    m_minimumOutputs = m_baselineOutputs;

    m_firstSample = ProcessMetrics::sample();
    m_previousSample = m_firstSample;
    m_previousSampleMs = 0;
    m_residentSamples.append(m_firstSample.residentBytes);

    m_wallClock.start();
    m_lagClock.start();

    const auto interval = m_options.actionsPerMinute > 0
        ? std::max(1, 60000 / m_options.actionsPerMinute) : 250;
    m_actionTimer.start(interval);
    m_sampleTimer.start(1000);
    m_lagTimer.start(50);

    QTimer::singleShot(std::max(1, m_options.durationSeconds) * 1000, this,
                       &EnduranceRunner::finish);
}

void EnduranceRunner::prepareContent()
{
    // A sessão trabalha apenas com conteúdo próprio: nada da biblioteca do
    // operador é lido, alterado ou apresentado.
    const auto presentationId =
        m_controller.createTextPresentation(QStringLiteral("Endurance"));
    if (presentationId.isEmpty()) {
        recordFailure(QStringLiteral("setup"),
                      QStringLiteral("Não foi possível criar a apresentação de endurance."));
    } else {
        for (int index = 2; index <= 12; ++index) {
            m_controller.addTextSlide(QString::number(index),
                                      QStringLiteral("Slide de endurance %1").arg(index));
        }
        m_slideCount = 12;
    }

    if (!m_options.mediaDirectory.isEmpty()) {
        const QFileInfo directory(m_options.mediaDirectory);
        if (!directory.isDir()) {
            recordFailure(QStringLiteral("setup"),
                          QStringLiteral("Diretório de mídia inexistente: %1")
                              .arg(m_options.mediaDirectory));
        } else if (m_controller.addMediaFolder(QUrl::fromLocalFile(directory.absoluteFilePath()))) {
            const auto addAll = [this](const QVariantList &entries) {
                for (const auto &entry : entries) {
                    const auto path = entry.toMap().value(QStringLiteral("path")).toString();
                    if (path.isEmpty()) continue;
                    const auto id = m_controller.addCatalogFileToPlaylist(path);
                    if (!id.isEmpty()) m_mediaIds.append(id);
                }
            };
            addAll(m_controller.folderVideoFiles());
            addAll(m_controller.folderImageFiles());
            addAll(m_controller.folderAudioFiles());
            if (m_mediaIds.isEmpty()) {
                recordFailure(QStringLiteral("setup"),
                              QStringLiteral("Nenhuma mídia utilizável em %1")
                                  .arg(m_options.mediaDirectory));
            }
        } else {
            recordFailure(QStringLiteral("setup"),
                          QStringLiteral("Não foi possível registrar a pasta de mídia."));
        }
    }

    if (m_controller.outputWindows().isEmpty()) {
        // A tela principal é do operador e nunca vira saída. Sem nenhuma tela
        // externa conectada a sessão continua válida para CPU, memória e event
        // loop, mas o relatório registra `outputsBaseline` igual a zero e o
        // gate de perda de saída fica inerte.
        m_controller.enableAllScreens();
        for (const auto &screen : m_controller.screens()) {
            const auto entry = screen.toMap();
            if (entry.value(QStringLiteral("primary")).toBool()) continue;
            const auto fingerprint = entry.value(QStringLiteral("fingerprint")).toString();
            if (!fingerprint.isEmpty()) m_controller.toggleScreen(fingerprint, true);
        }
    }
}

CommandResult EnduranceRunner::dispatch(const QString &type, const QVariantMap &payload)
{
    const auto result = m_controller.commandBus().dispatch(Command{
        .id = QUuid::createUuid().toString(QUuid::WithoutBraces),
        .type = type,
        .payload = payload,
        .source = QStringLiteral("endurance"),
        .issuedAt = QDateTime::currentDateTimeUtc(),
    });
    ++m_actionCount;
    m_actionsByType[type] += 1;
    if (!result.accepted) {
        ++m_rejectedCommands;
        recordFailure(QStringLiteral("command-rejected"),
                      QStringLiteral("%1 recusado: %2 %3")
                          .arg(type, result.errorCode, result.message).trimmed());
    }
    return result;
}

QStringList EnduranceRunner::availableActions() const
{
    QStringList actions{
        QStringLiteral("slide.next"),
        QStringLiteral("slide.next"),
        QStringLiteral("slide.previous"),
        QStringLiteral("slide.show"),
        QStringLiteral("audience-message"),
        QStringLiteral("lower-third"),
        QStringLiteral("alert"),
        QStringLiteral("stage-message"),
        QStringLiteral("blackout"),
        QStringLiteral("countdown"),
        QStringLiteral("presentation-cycle"),
    };
    if (!m_controller.outputWindows().isEmpty()) {
        actions << QStringLiteral("output-role") << QStringLiteral("output-media");
    }
    if (!m_mediaIds.isEmpty()) {
        actions << QStringLiteral("media-play") << QStringLiteral("media-play");
        // Pausar ou parar sem mídia carregada é recusado pelo domínio, e com
        // razão. O executor só oferece essas ações quando há reprodução, para
        // que uma recusa continue sendo sinal de defeito.
        const auto state = m_controller.mediaState();
        if (state == QStringLiteral("playing") || state == QStringLiteral("paused"))
            actions << QStringLiteral("media-pause") << QStringLiteral("media-stop");
    }
    return actions;
}

void EnduranceRunner::performAction()
{
    const auto actions = availableActions();
    if (actions.isEmpty()) return;
    std::uniform_int_distribution<int> pick(0, static_cast<int>(actions.size()) - 1);
    const auto action = actions.at(pick(m_random));

    if (action == QStringLiteral("slide.next")) {
        dispatch(QStringLiteral("presentation.slide.next"));
    } else if (action == QStringLiteral("slide.previous")) {
        dispatch(QStringLiteral("presentation.slide.previous"));
    } else if (action == QStringLiteral("slide.show")) {
        std::uniform_int_distribution<int> slide(0, std::max(0, m_slideCount - 1));
        dispatch(QStringLiteral("presentation.slide.show"),
                 {{QStringLiteral("index"), slide(m_random)}});
    } else if (action == QStringLiteral("audience-message")) {
        const auto even = (m_actionCount % 2) == 0;
        dispatch(QStringLiteral("overlay.audience-message.set"),
                 {{QStringLiteral("message"),
                   even ? QStringLiteral("Aviso de endurance") : QString{}}});
    } else if (action == QStringLiteral("lower-third")) {
        dispatch(QStringLiteral("overlay.lower-third.set"),
                 {{QStringLiteral("title"), QStringLiteral("Endurance")},
                  {QStringLiteral("subtitle"), QString::number(m_actionCount)}});
    } else if (action == QStringLiteral("alert")) {
        dispatch(QStringLiteral("overlay.alert.set"),
                 {{QStringLiteral("message"), QStringLiteral("Alerta de endurance")}});
    } else if (action == QStringLiteral("stage-message")) {
        dispatch(QStringLiteral("stage.message.set"),
                 {{QStringLiteral("message"), QStringLiteral("Palco %1").arg(m_actionCount)}});
    } else if (action == QStringLiteral("blackout")) {
        m_blackout = !m_blackout;
        dispatch(QStringLiteral("presentation.blackout.set"),
                 {{QStringLiteral("enabled"), m_blackout}});
    } else if (action == QStringLiteral("countdown")) {
        if ((m_actionCount % 2) == 0) {
            dispatch(QStringLiteral("timer.countdown.start"),
                     {{QStringLiteral("seconds"), 300}});
        } else {
            dispatch(QStringLiteral("timer.countdown.stop"));
        }
    } else if (action == QStringLiteral("presentation-cycle")) {
        dispatch(QStringLiteral("presentation.stop"));
        dispatch(QStringLiteral("presentation.slide.show"), {{QStringLiteral("index"), 0}});
    } else if (action == QStringLiteral("output-role")) {
        static const QStringList roles{
            QStringLiteral("audience"), QStringLiteral("stage"),
            QStringLiteral("broadcast"), QStringLiteral("confidence"),
        };
        const auto outputs = m_controller.outputWindows();
        const auto fingerprint = outputs.first().toMap().value(QStringLiteral("id")).toString();
        const auto role = roles.at(m_roleCursor % roles.size());
        ++m_roleCursor;
        dispatch(QStringLiteral("output.role.set"),
                 {{QStringLiteral("fingerprint"), fingerprint},
                  {QStringLiteral("role"), role}});
    } else if (action == QStringLiteral("output-media")) {
        const auto outputs = m_controller.outputWindows();
        const auto first = outputs.first().toMap();
        dispatch(QStringLiteral("output.media-enabled.set"),
                 {{QStringLiteral("fingerprint"), first.value(QStringLiteral("id")).toString()},
                  {QStringLiteral("enabled"),
                   !first.value(QStringLiteral("mediaEnabled")).toBool()}});
    } else if (action == QStringLiteral("media-play")) {
        std::uniform_int_distribution<int> media(0, static_cast<int>(m_mediaIds.size()) - 1);
        dispatch(QStringLiteral("media.play"),
                 {{QStringLiteral("mediaId"), m_mediaIds.at(media(m_random))}});
    } else if (action == QStringLiteral("media-pause")) {
        dispatch(QStringLiteral("media.pause.toggle"));
    } else if (action == QStringLiteral("media-stop")) {
        dispatch(QStringLiteral("media.stop"));
    }
}

void EnduranceRunner::takeSample()
{
    const auto elapsedMs = m_wallClock.elapsed();
    const auto sample = ProcessMetrics::sample();
    const auto deltaSeconds = static_cast<double>(elapsedMs - m_previousSampleMs) / 1000.0;
    if (deltaSeconds > 0.0) {
        const auto cpu = (sample.cpuSeconds - m_previousSample.cpuSeconds) / deltaSeconds * 100.0;
        m_cpuPercentSamples.append(std::max(0.0, cpu));
    }
    m_previousSample = sample;
    m_previousSampleMs = elapsedMs;
    m_residentSamples.append(sample.residentBytes);

    const auto outputs = static_cast<int>(m_controller.outputWindows().size());
    m_minimumOutputs = std::min(m_minimumOutputs, outputs);
    if (outputs < m_baselineOutputs) {
        recordFailure(QStringLiteral("output-loss"),
                      QStringLiteral("Saídas ativas caíram de %1 para %2 sem comando.")
                          .arg(m_baselineOutputs).arg(outputs));
        // A linha de base acompanha a queda para não repetir o mesmo registro
        // a cada segundo; o bloqueador já foi anotado.
        m_baselineOutputs = outputs;
    }

    const auto state = m_controller.mediaState();
    const auto position = m_controller.mediaPositionMs();
    if (state == QStringLiteral("playing") && m_controller.mediaDurationMs() > 0) {
        if (state == m_lastMediaState && position == m_lastMediaPosition) {
            ++m_stalledSamples;
            if (m_stalledSamples >= kStallSamples && !m_stallReported) {
                m_stallReported = true;
                recordFailure(QStringLiteral("playback-stall"),
                              QStringLiteral("Reprodução parada em %1 ms por %2 s.")
                                  .arg(position).arg(m_stalledSamples));
            }
        } else {
            m_stalledSamples = 0;
        }
    } else {
        m_stalledSamples = 0;
    }
    m_lastMediaState = state;
    m_lastMediaPosition = position;
}

void EnduranceRunner::observeEvent(const DomainEvent &event)
{
    if (event.correlationId.isEmpty()) return;
    const auto count = m_eventsByCorrelation[event.correlationId] + 1;
    m_eventsByCorrelation[event.correlationId] = count;
    if (count > kCorrelationLimit && !m_loopReported) {
        m_loopReported = true;
        recordFailure(QStringLiteral("automation-loop"),
                      QStringLiteral("Correlação %1 gerou %2 eventos encadeados.")
                          .arg(event.correlationId).arg(count));
    }
}

void EnduranceRunner::recordFailure(const QString &category, const QString &description)
{
    m_failures.append(Failure{
        .atSeconds = m_wallClock.isValid()
            ? static_cast<double>(m_wallClock.elapsed()) / 1000.0 : 0.0,
        .category = category,
        .description = description,
    });
}

void EnduranceRunner::finish()
{
    if (m_finished) return;
    m_actionTimer.stop();
    m_sampleTimer.stop();
    m_lagTimer.stop();
    m_finished = true;
    buildReport();
    emit completed();
}

void EnduranceRunner::buildReport()
{
    const auto elapsedSeconds = static_cast<double>(m_wallClock.elapsed()) / 1000.0;
    const auto warnings = AppLogger::warningCount() - m_warningBaseline;
    const auto criticals = AppLogger::criticalCount() - m_criticalBaseline;
    const auto fatals = AppLogger::fatalCount() - m_fatalBaseline;

    const auto window = std::max<qsizetype>(1, m_residentSamples.size() / 4);
    const auto residentStart = averageOf(m_residentSamples, 0, window);
    const auto residentEnd =
        averageOf(m_residentSamples, m_residentSamples.size() - window, window);
    const auto residentPeak = m_residentSamples.isEmpty()
        ? 0ull : *std::max_element(m_residentSamples.cbegin(), m_residentSamples.cend());
    quint64 residentTotal = 0;
    for (const auto value : m_residentSamples) residentTotal += value;
    const auto residentAverage = m_residentSamples.isEmpty()
        ? 0ull : residentTotal / static_cast<quint64>(m_residentSamples.size());
    const auto growthRatio = residentStart > 0
        ? static_cast<double>(residentEnd) / static_cast<double>(residentStart) : 1.0;

    const auto lagAverage = average(m_lagSamples);
    const auto lagP95 = percentile(m_lagSamples, 0.95);
    const auto lagMax = m_lagSamples.isEmpty()
        ? 0.0 : *std::max_element(m_lagSamples.cbegin(), m_lagSamples.cend());

    QStringList blockers;
    if (criticals > 0)
        blockers << QStringLiteral("%1 mensagem(ns) crítica(s) registrada(s)").arg(criticals);
    if (fatals > 0)
        blockers << QStringLiteral("%1 mensagem(ns) fatal(is) registrada(s)").arg(fatals);
    if (m_rejectedCommands > 0)
        blockers << QStringLiteral("%1 comando(s) recusado(s)").arg(m_rejectedCommands);
    for (const auto &failure : m_failures) {
        if (failure.category == QStringLiteral("output-loss")
            || failure.category == QStringLiteral("playback-stall")
            || failure.category == QStringLiteral("automation-loop")
            || failure.category == QStringLiteral("setup")) {
            blockers << failure.description;
        }
    }
    if (m_lagSamples.size() >= kLagMinimumSamples) {
        if (lagMax > kMaxLagMs) {
            blockers << QStringLiteral("atraso máximo do event loop de %1 ms")
                            .arg(lagMax, 0, 'f', 1);
        }
        if (lagP95 > kMaxP95LagMs) {
            blockers << QStringLiteral("p95 do atraso do event loop de %1 ms")
                            .arg(lagP95, 0, 'f', 1);
        }
    }
    if (m_residentSamples.size() >= kGrowthMinimumSamples
        && growthRatio > kMaxResidentGrowthRatio
        && residentEnd > residentStart
        && residentEnd - residentStart > kMaxResidentGrowthBytes) {
        blockers << QStringLiteral("memória residente cresceu %1%")
                        .arg((growthRatio - 1.0) * 100.0, 0, 'f', 1);
    }
    blockers.removeDuplicates();
    m_blockers = blockers;

    QJsonObject actionsByType;
    for (auto it = m_actionsByType.cbegin(); it != m_actionsByType.cend(); ++it)
        actionsByType.insert(it.key(), static_cast<double>(it.value()));

    QJsonArray failures;
    for (const auto &failure : m_failures) {
        failures.append(QJsonObject{
            {QStringLiteral("atSeconds"), failure.atSeconds},
            {QStringLiteral("category"), failure.category},
            {QStringLiteral("description"), failure.description},
        });
    }

    m_report = QJsonObject{
        {QStringLiteral("schema"), QStringLiteral("holyscreen.endurance/1")},
        {QStringLiteral("environment"), QJsonObject{
            {QStringLiteral("application"), QCoreApplication::applicationName()},
            {QStringLiteral("version"), QCoreApplication::applicationVersion()},
            {QStringLiteral("qt"), QLibraryInfo::version().toString()},
            {QStringLiteral("operatingSystem"), ProcessMetrics::operatingSystem()},
            {QStringLiteral("kernel"), ProcessMetrics::kernel()},
            {QStringLiteral("architecture"), QSysInfo::currentCpuArchitecture()},
            {QStringLiteral("logicalCores"), ProcessMetrics::logicalCores()},
            {QStringLiteral("totalMemoryBytes"),
             static_cast<double>(ProcessMetrics::totalMemoryBytes())},
            {QStringLiteral("platformPlugin"), qEnvironmentVariable("QT_QPA_PLATFORM")},
        }},
        {QStringLiteral("run"), QJsonObject{
            {QStringLiteral("startedAt"), m_startedAt},
            {QStringLiteral("finishedAt"),
             QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
            {QStringLiteral("plannedSeconds"), m_options.durationSeconds},
            {QStringLiteral("elapsedSeconds"), elapsedSeconds},
            {QStringLiteral("seed"), static_cast<double>(m_options.seed)},
            {QStringLiteral("actionsPerMinute"), m_options.actionsPerMinute},
            {QStringLiteral("actions"), static_cast<double>(m_actionCount)},
            {QStringLiteral("actionsByType"), actionsByType},
            {QStringLiteral("mediaItems"), static_cast<int>(m_mediaIds.size())},
            {QStringLiteral("slides"), m_slideCount},
        }},
        {QStringLiteral("metrics"), QJsonObject{
            {QStringLiteral("samples"), static_cast<int>(m_residentSamples.size())},
            {QStringLiteral("cpuPercentAverage"), average(m_cpuPercentSamples)},
            {QStringLiteral("cpuPercentPeak"), m_cpuPercentSamples.isEmpty() ? 0.0
                : *std::max_element(m_cpuPercentSamples.cbegin(), m_cpuPercentSamples.cend())},
            {QStringLiteral("residentBytesStart"), static_cast<double>(residentStart)},
            {QStringLiteral("residentBytesAverage"), static_cast<double>(residentAverage)},
            {QStringLiteral("residentBytesPeak"), static_cast<double>(residentPeak)},
            {QStringLiteral("residentBytesEnd"), static_cast<double>(residentEnd)},
            {QStringLiteral("residentGrowthPercent"), (growthRatio - 1.0) * 100.0},
            {QStringLiteral("eventLoopLagMsAverage"), lagAverage},
            {QStringLiteral("eventLoopLagMsP95"), lagP95},
            {QStringLiteral("eventLoopLagMsMax"), lagMax},
            {QStringLiteral("logWarnings"), static_cast<double>(warnings)},
            {QStringLiteral("logCriticals"), static_cast<double>(criticals)},
            {QStringLiteral("logFatals"), static_cast<double>(fatals)},
            {QStringLiteral("commandsRejected"), static_cast<double>(m_rejectedCommands)},
            {QStringLiteral("outputsBaseline"), m_baselineOutputs},
            {QStringLiteral("outputsMinimum"), m_minimumOutputs},
        }},
        {QStringLiteral("failures"), failures},
        {QStringLiteral("verdict"), QJsonObject{
            {QStringLiteral("passed"), blockers.isEmpty()},
            {QStringLiteral("blockers"), QJsonArray::fromStringList(blockers)},
        }},
    };
}

bool EnduranceRunner::passed() const { return m_finished && m_blockers.isEmpty(); }

bool EnduranceRunner::writeReport(QString *error) const
{
    if (m_options.reportPath.isEmpty()) {
        if (error) *error = QStringLiteral("Nenhum caminho de relatório informado.");
        return false;
    }
    QDir().mkpath(QFileInfo(m_options.reportPath).absolutePath());
    QSaveFile file(m_options.reportPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error) *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(m_report).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

} // namespace churchpresenter
