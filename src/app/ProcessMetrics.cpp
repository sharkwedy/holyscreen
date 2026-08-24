#include "app/ProcessMetrics.h"

#include <QSysInfo>
#include <QThread>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/resource.h>
#else
#include <QByteArray>
#include <QFile>
#include <sys/resource.h>
#include <unistd.h>
#endif

namespace churchpresenter {

ProcessSample ProcessMetrics::sample()
{
    ProcessSample result;
#if defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
        result.residentBytes = static_cast<quint64>(counters.WorkingSetSize);

    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernelTime, &userTime)) {
        const auto toSeconds = [](const FILETIME &value) {
            ULARGE_INTEGER converted{};
            converted.LowPart = value.dwLowDateTime;
            converted.HighPart = value.dwHighDateTime;
            // FILETIME conta intervalos de 100 nanossegundos.
            return static_cast<double>(converted.QuadPart) / 1.0e7;
        };
        result.cpuSeconds = toSeconds(kernelTime) + toSeconds(userTime);
    }
#else
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        result.cpuSeconds = static_cast<double>(usage.ru_utime.tv_sec)
            + static_cast<double>(usage.ru_utime.tv_usec) / 1.0e6
            + static_cast<double>(usage.ru_stime.tv_sec)
            + static_cast<double>(usage.ru_stime.tv_usec) / 1.0e6;
    }
#if defined(Q_OS_MACOS)
    mach_task_basic_info info{};
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        result.residentBytes = static_cast<quint64>(info.resident_size);
    }
#else
    QFile statm(QStringLiteral("/proc/self/statm"));
    if (statm.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto fields = statm.readLine().trimmed().split(' ');
        if (fields.size() > 1) {
            const auto pageSize = static_cast<quint64>(sysconf(_SC_PAGESIZE));
            result.residentBytes = fields.at(1).toULongLong() * pageSize;
        }
    }
#endif
#endif
    return result;
}

QString ProcessMetrics::operatingSystem() { return QSysInfo::prettyProductName(); }

QString ProcessMetrics::kernel()
{
    return QSysInfo::kernelType() + QStringLiteral(" ") + QSysInfo::kernelVersion();
}

int ProcessMetrics::logicalCores() { return QThread::idealThreadCount(); }

quint64 ProcessMetrics::totalMemoryBytes()
{
#if defined(Q_OS_WIN)
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) return static_cast<quint64>(status.ullTotalPhys);
    return 0;
#elif defined(Q_OS_MACOS)
    quint64 total = 0;
    size_t size = sizeof(total);
    if (sysctlbyname("hw.memsize", &total, &size, nullptr, 0) == 0) return total;
    return 0;
#else
    const auto pages = sysconf(_SC_PHYS_PAGES);
    const auto pageSize = sysconf(_SC_PAGESIZE);
    if (pages > 0 && pageSize > 0)
        return static_cast<quint64>(pages) * static_cast<quint64>(pageSize);
    return 0;
#endif
}

} // namespace churchpresenter
