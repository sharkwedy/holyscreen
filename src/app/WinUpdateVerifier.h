#pragma once

#if !defined(_WIN32)
#error "WinUpdateVerifier is a Windows-only component."
#endif

#include <cstdint>
#include <string>

namespace holyscreen_update {

//! Confere tamanho e SHA-256 do instalador imediatamente antes da elevação.
bool verifiedInstaller(const std::wstring &path, const std::wstring &expectedDigest,
                       std::uint64_t expectedSize);

} // namespace holyscreen_update
