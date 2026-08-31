#if !defined(_WIN32)
#error "WinUpdateHelper is a Windows-only executable."
#endif

#include "app/WinUpdateVerifier.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdint>
#include <cwctype>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::wstring argumentValue(const std::vector<std::wstring> &arguments,
                           const std::wstring &name)
{
    for (std::size_t index = 0; index + 1 < arguments.size(); ++index) {
        if (arguments[index] == name) return arguments[index + 1];
    }
    return {};
}

void showError(const wchar_t *message)
{
    MessageBoxW(nullptr, message, L"HolyScreen Update", MB_OK | MB_ICONERROR);
}

bool isHexDigest(const std::wstring &value)
{
    return value.size() == 64
        && std::all_of(value.begin(), value.end(), [](wchar_t character) {
               return std::iswxdigit(character) != 0;
           });
}

std::wstring lower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t character) {
        return static_cast<wchar_t>(std::towlower(character));
    });
    return value;
}

std::wstring fileName(const std::wstring &path)
{
    const auto separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? path : path.substr(separator + 1);
}

bool hasExpectedInstallerName(const std::wstring &path)
{
    const auto name = lower(fileName(path));
    return name.starts_with(L"holyscreen-") && name.ends_with(L".exe");
}

bool hasExpectedApplicationName(const std::wstring &path)
{
    return lower(fileName(path)) == L"holyscreen.exe";
}

bool isRegularFile(const std::wstring &path)
{
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES
        && !(attributes & FILE_ATTRIBUTE_DIRECTORY)
        && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

bool launchElevatedInstaller(const std::wstring &installer, HANDLE &process)
{
    SHELLEXECUTEINFOW execution{};
    execution.cbSize = sizeof(execution);
    execution.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    execution.lpVerb = L"runas";
    execution.lpFile = installer.c_str();
    execution.lpParameters = L"/S";
    execution.nShow = SW_HIDE;
    if (!ShellExecuteExW(&execution) || !execution.hProcess) return false;
    process = execution.hProcess;
    return true;
}

bool relaunch(const std::wstring &application)
{
    if (!hasExpectedApplicationName(application) || !isRegularFile(application)) return false;
    SHELLEXECUTEINFOW execution{};
    execution.cbSize = sizeof(execution);
    execution.fMask = SEE_MASK_NOASYNC;
    execution.lpVerb = L"open";
    execution.lpFile = application.c_str();
    execution.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&execution) != FALSE;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int count = 0;
    auto **rawArguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!rawArguments) return 2;
    std::vector<std::wstring> arguments;
    arguments.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) arguments.emplace_back(rawArguments[index]);
    LocalFree(rawArguments);

    const auto installer = argumentValue(arguments, L"--installer");
    const auto expectedDigest = argumentValue(arguments, L"--sha256");
    const auto sizeText = argumentValue(arguments, L"--size");
    const auto application = argumentValue(arguments, L"--application");
    const auto processText = argumentValue(arguments, L"--wait-pid");
    if (installer.empty() || application.empty() || !isHexDigest(expectedDigest)
        || !hasExpectedInstallerName(installer) || !hasExpectedApplicationName(application)) {
        showError(L"Os parâmetros da atualização são inválidos.");
        return 3;
    }

    std::uint64_t expectedSize = 0;
    DWORD processId = 0;
    try {
        std::size_t sizePosition = 0;
        std::size_t processPosition = 0;
        expectedSize = std::stoull(sizeText, &sizePosition);
        const auto parsedProcessId = std::stoull(processText, &processPosition);
        if (sizePosition != sizeText.size() || processPosition != processText.size()
            || expectedSize == 0 || parsedProcessId == 0
            || parsedProcessId > (std::numeric_limits<DWORD>::max)()) {
            throw std::invalid_argument("out of range");
        }
        processId = static_cast<DWORD>(parsedProcessId);
    } catch (...) {
        showError(L"Os parâmetros numéricos da atualização são inválidos.");
        return 4;
    }

    if (HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, processId)) {
        const auto waitResult = WaitForSingleObject(parent, 120'000);
        CloseHandle(parent);
        if (waitResult != WAIT_OBJECT_0) {
            showError(L"O HolyScreen não encerrou a tempo; a atualização foi cancelada.");
            return 5;
        }
    } else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        showError(L"Não foi possível aguardar o encerramento seguro do HolyScreen.");
        return 5;
    }

    if (!holyscreen_update::verifiedInstaller(installer, expectedDigest, expectedSize)) {
        showError(L"O instalador mudou depois do download e não será executado.");
        return 6;
    }

    HANDLE installerProcess = nullptr;
    if (!launchElevatedInstaller(installer, installerProcess)) {
        showError(L"O Windows não permitiu iniciar o instalador do HolyScreen.");
        return 7;
    }
    const auto installerWait = WaitForSingleObject(installerProcess, INFINITE);
    DWORD exitCode = 1;
    const auto readExitCode = GetExitCodeProcess(installerProcess, &exitCode);
    CloseHandle(installerProcess);
    if (installerWait != WAIT_OBJECT_0 || !readExitCode || exitCode != 0) {
        showError(L"A instalação do HolyScreen não foi concluída.");
        return 8;
    }
    if (!relaunch(application)) {
        showError(L"A atualização terminou, mas o HolyScreen não pôde ser reaberto.");
        return 9;
    }
    return 0;
}
