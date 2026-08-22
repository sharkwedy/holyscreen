# HolyScreen

[![CI](https://github.com/sharkwedy/holyscreen/actions/workflows/ci.yml/badge.svg)](https://github.com/sharkwedy/holyscreen/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/sharkwedy/holyscreen?include_prereleases)](https://github.com/sharkwedy/holyscreen/releases)
[![Downloads](https://img.shields.io/github/downloads/sharkwedy/holyscreen/total)](https://github.com/sharkwedy/holyscreen/releases)
[![License: GPL v3](https://img.shields.io/github/license/sharkwedy/holyscreen)](LICENSE)

[Português do Brasil](README.pt-BR.md)

HolyScreen is an open-source desktop presentation engine for churches. Built
with C++20 and Qt 6/QML, it works offline and keeps screen discovery,
presentation state, persistence, and rendering separate.

> Status: `0.10.3` development preview. This is not yet the stable `1.0.0` release.

![HolyScreen operator dashboard](.stitch/painel-principal.png)

## What works today

- a dedicated operator screen and up to five persistent external outputs;
- Audience and Stage output roles, monitor identification, blackout, and simulations;
- wallpaper, clock, text, lyrics, Bible, image, audio, and video presentation;
- unified media player with playlists, seeking, volume, and repeat;
- recursive media folders, type-specific catalogs, and filename search;
- presentations, structured songs, themes, and service playlists;
- canonical folder, public Git HTTPS, ZIP, and legacy JSON Bible import with
  source metadata, safe updates, progress/cancellation, reference search, and
  per-output translations;
- Stage View with current/next slide, clock, timer, and messages;
- message, alert, lower-third, countdown, and stopwatch overlays;
- history, basic reports, backup, restore, and crash recovery;
- an early local HTTP/WebSocket API and web remote.

See the [roadmap](docs/ROADMAP.md) for the path to 1.0 and [IDEA.md](IDEA.md)
for the original product design.

## Install a development preview

1. Open the [Releases](https://github.com/sharkwedy/holyscreen/releases) page.
2. Download the asset for your system: `.exe`/`.zip` on Windows, `.dmg` on
   macOS, or `.AppImage`/`.deb`/`.tar.gz` on Linux.
3. Verify the download against the release's `SHA256SUMS` file.
4. Install or extract it, then start HolyScreen.

Development previews may be unsigned. Your operating system can ask you to
confirm that you trust the application. Back up important presentation data
before upgrading between previews.

## Build from source

Requirements: CMake 3.21+, a C++20 compiler, and Qt 6.8+ with Core, Gui, Quick,
Quick Controls, SQL, Multimedia, Network, Concurrent, HttpServer, WebSockets,
and Test. CMake downloads pinned libgit2 and miniz sources during configure.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target church-presenter_qmllint
```

Run the application:

```bash
open build/src/church-presenter.app                    # macOS
build/src/church-presenter.exe                         # Windows
build/src/church-presenter                             # Linux
```

Local data is stored in the operating system's application-data directory in
`presenter.db`. Set `HOLYSCREEN_DATA_DIR` to use a different directory in tests.

## Contributing and support

Contributions are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md), the
[Code of Conduct](CODE_OF_CONDUCT.md), and [governance](GOVERNANCE.md) before
opening a pull request. For help, see [SUPPORT.md](SUPPORT.md). Report security
problems through the private process in [SECURITY.md](SECURITY.md).

## License

HolyScreen is licensed under the [GNU General Public License v3.0](LICENSE).
Imported Bible translations and media remain subject to their owners' licenses.
See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

See the [Bible import guide](docs/BIBLE_IMPORT.md) for the canonical format,
supported sources, license confirmation, updates, and safety limits.
