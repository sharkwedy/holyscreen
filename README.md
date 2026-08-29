# HolyScreen

[![CI](https://github.com/sharkwedy/holyscreen/actions/workflows/ci.yml/badge.svg)](https://github.com/sharkwedy/holyscreen/actions/workflows/ci.yml)
[![Latest release](https://img.shields.io/github/v/release/sharkwedy/holyscreen?include_prereleases)](https://github.com/sharkwedy/holyscreen/releases)
[![Downloads](https://img.shields.io/github/downloads/sharkwedy/holyscreen/total)](https://github.com/sharkwedy/holyscreen/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

[Português do Brasil](README.pt-BR.md)

HolyScreen is an open-source desktop presentation engine for churches. Built
with C++20 and Qt 6/QML, it works offline and keeps screen discovery,
presentation state, persistence, and rendering separate.

> Status: stable `1.2.1` release.

![HolyScreen operator dashboard](.stitch/painel-principal.png)

Real HolyScreen capture using the public-domain Bíblia Livre (BLIVRE)
translation imported from [`damarals/biblias`](https://github.com/damarals/biblias).

## What works today

- a dedicated operator screen and up to five persistent external outputs;
- Audience, Stage and Broadcast output roles, monitor identification, blackout,
  safe areas, chroma/transparent capture profiles, and simulations;
- wallpaper, clock, text, lyrics, Bible, image, audio, and video presentation;
- unified media player with playlists, seeking, volume, and repeat;
- recursive media folders, type-specific catalogs, and filename search;
- presentations, structured songs, themes, and service playlists;
- canonical folder, public Git HTTPS, ZIP, and legacy JSON Bible import with
  source metadata, safe updates, progress/cancellation, reference search, and
  per-output translations;
- Stage View with current/next slide, clock, timer, and messages;
- message, alert, lower-third, countdown, and stopwatch overlays;
- history, basic reports, transactional migrations, backup, restore, crash
  recovery, autosave, undo/redo, and sanitized diagnostic exports;
- a password-protected local HTTP/WebSocket API v1 and responsive offline web
  remote for presentations, media, Bible, events, Stage View, overlays, and
  timers;
- HTTP, WebSocket, OBS v5, MIDI and OSC integrations with sanitized history and
  operating-system-backed secret storage when available;
- offline automations with guarded triggers, declarative conditions, bounded
  actions, dry-run, history and an authorized executable allowlist;
- guided setup, secret-free operator profiles, editable keyboard shortcuts,
  pt-BR/en-US UI and remote, and 100%, 150% and 200% interface scaling;
- a shared command/event architecture used by the desktop UI and remote.

See the [roadmap](docs/ROADMAP.md) for the path to 1.0 and [IDEA.md](IDEA.md)
for the original product design.

Operator documentation: [full manual](docs/OPERATOR_MANUAL.md), [quick service
guide](docs/QUICK_SERVICE_GUIDE.md), and [troubleshooting](docs/TROUBLESHOOTING.md).

## Install HolyScreen

1. Open the [Releases](https://github.com/sharkwedy/holyscreen/releases) page.
2. Download the asset for your system: `.exe`/`.zip` on Windows, `.dmg` on
   macOS, or `.AppImage`/`.deb`/`.tar.gz` on Linux.
3. Verify the download against the release's `SHA256SUMS` file.
4. Install or extract it, then start HolyScreen.

Release packages may be unsigned. Your operating system can ask you to
confirm that you trust the application. Back up important presentation data
before upgrading.

HolyScreen's update checker reads the official GitHub Releases API directly,
including published pre-releases, and only notifies the operator. It does not
download or install updates automatically. Always verify a manually downloaded
package against the release's `SHA256SUMS` file.

- Windows SmartScreen can require **More info → Run anyway**.
- On macOS, open **System Settings → Privacy & Security** after the first
  blocked launch, or Control-click the app and choose **Open**.
- On Linux, an AppImage may need `chmod +x HolyScreen-*.AppImage`.

## Local web remote

Open **Settings → Remote**, choose the IPv4 interface and port, define a fixed
password, and enable the server. Scan the displayed QR code from a device on
the same trusted local network. The server is disabled by default and must not
be exposed directly to the internet.

Only a salted PBKDF2-HMAC-SHA256 password hash is persisted. Sessions expire,
can be revoked, and are protected by login and command rate limits. The web app
has no CDN or runtime internet dependency. See the [Remote API and security
guide](docs/REMOTE_API.md).

## Build from source

Requirements: CMake 3.21+, a C++20 compiler, and Qt 6.8+ with Core, Gui, Quick,
Quick Controls, SQL, Multimedia, Network, Concurrent, HttpServer, WebSockets,
and Test. CMake downloads pinned libgit2, miniz, and QR generator sources during
configure.

```bash
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target presenter-ui_qmllint
```

See the [testing guide](docs/TESTING.md) for the unit, integration, QML,
golden, E2E and performance suites.

Run the application:

```bash
open build/src/HolyScreen.app                          # macOS
build/holyscreen.exe                                   # Windows
build/src/holyscreen                                   # Linux
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
