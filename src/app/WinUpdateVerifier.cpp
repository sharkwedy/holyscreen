#include "app/WinUpdateVerifier.h"

#include <windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <array>
#include <cwctype>
#include <vector>

namespace {

std::wstring lower(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](wchar_t character) {
        return static_cast<wchar_t>(std::towlower(character));
    });
    return value;
}

bool isRegularFile(const std::wstring &path)
{
    const auto attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES
        && !(attributes & FILE_ATTRIBUTE_DIRECTORY)
        && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

} // namespace

namespace holyscreen_update {

bool verifiedInstaller(const std::wstring &path, const std::wstring &expectedDigest,
                       std::uint64_t expectedSize)
{
    if (!isRegularFile(path)) return false;

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0
        || static_cast<std::uint64_t>(size.QuadPart) != expectedSize) {
        CloseHandle(file);
        return false;
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectLength = 0;
    DWORD resultLength = 0;
    bool valid = false;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0
        || BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                             reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength),
                             &resultLength, 0) < 0) {
        CloseHandle(file);
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        return false;
    }

    std::vector<UCHAR> hashObject(objectLength);
    std::array<UCHAR, 32> digest{};
    if (BCryptCreateHash(algorithm, &hash, hashObject.data(), objectLength,
                         nullptr, 0, 0) >= 0) {
        // Este buffer precisa ficar no heap. O helper reserva 1 MiB de pilha no
        // Windows; o antigo std::array de 1 MiB derrubava o processo antes que
        // o instalador pudesse ser elevado.
        std::vector<UCHAR> buffer(64 * 1024);
        bool readSucceeded = true;
        while (true) {
            DWORD received = 0;
            if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()),
                          &received, nullptr)) {
                readSucceeded = false;
                break;
            }
            if (received == 0) break;
            if (BCryptHashData(hash, buffer.data(), received, 0) < 0) {
                readSucceeded = false;
                break;
            }
        }
        if (readSucceeded
            && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) >= 0) {
            static constexpr wchar_t digits[] = L"0123456789abcdef";
            std::wstring actual;
            actual.reserve(64);
            for (const auto byte : digest) {
                actual.push_back(digits[(byte >> 4) & 0x0f]);
                actual.push_back(digits[byte & 0x0f]);
            }
            valid = actual == lower(expectedDigest);
        }
    }

    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    CloseHandle(file);
    return valid;
}

} // namespace holyscreen_update
