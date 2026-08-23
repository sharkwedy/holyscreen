# Changelog

Notable HolyScreen changes are documented here. The project follows Semantic
Versioning where practical before 1.0.

## [Unreleased]

- Added a shared `OutputRole` serialization covering `audience`, `stage`,
  `broadcast`, `confidence`, and `custom`, with a strict parser that no longer
  reduces every non-stage role to audience.
- Extracted output state, persistence, and view models from
  `ApplicationController` into `OutputStateModule`, keeping the controller as a
  compatible facade for QML.
- Split the output renderer per role: `OutputWindow.qml` is now only a host and
  visual router around `AudienceView`, `StageOutputView`, and `BroadcastView`.
- Moved the remote PWA to versioned resources in `src/remote/web/`, served
  byte for byte from the binary without touching the filesystem.
- Moved the QML files to the `presenter-ui` library and added a Qt Quick Test
  suite covering role routing and offscreen rendering of the output views.
- Removed every QML lint warning; the context property is now the single
  documented exception in `MainWindow.qml` and `OutputWindow.qml`.
- Fixed the identify overlay so it stays above video on every output.

## [0.11.0] - 2026-08-22

- Added canonical Bible folder/repository, public Git HTTPS, public ZIP, and
  legacy JSON imports with source/license/revision metadata, transactional
  idempotent updates, progress, cancellation, and license confirmation.
- Routed desktop and remote operations through shared CommandBus/EventBus
  modules, with a catalog allowlist and state revisions.
- Expanded undo/redo for blackout, overlays, output routing, themes, playlists,
  repeat mode, and presentation editing.
- Added transactional database migrations with backup/rollback coverage,
  autosave/recovery, and sanitized ZIP diagnostic exports.
- Replaced experimental remote routes with authenticated `/api/v1` HTTP and
  WebSocket contracts, an embedded responsive offline PWA, live state, QR
  access, and automatic reconnection.
- Added PBKDF2-HMAC-SHA256 credentials, hashed eight-hour sessions, revocation,
  same-origin checks, 64 KiB payload limits, brute-force blocking, and a rolling
  30-commands-per-second limit.
- Corrected the macOS bundle name, executable name, and application icon.
- Changed CI triggers to run on commits reaching `main`, or on an exact `CI`
  comment by an authorized collaborator in an open pull request.
- Expanded the local suite to 52 tests and added remote HTTP/WebSocket,
  authentication, QR, command bridge, diagnostics, and Bible import coverage.

## [0.10.3] - 2026-08-20

- Identified the repository used to publish release assets.

## [0.10.2] - 2026-08-20

- Aligned the Linux desktop icon name with the packaged application.

## [0.10.1] - 2026-08-20

- Removed unused Qt SQL drivers from Linux release packages.

## [0.10.0] - 2026-08-20

- Added the initial cross-platform preview and automated release packaging.

[Unreleased]: https://github.com/sharkwedy/holyscreen/compare/v0.11.0...HEAD
[0.11.0]: https://github.com/sharkwedy/holyscreen/releases/tag/v0.11.0
[0.10.3]: https://github.com/sharkwedy/holyscreen/releases/tag/v0.10.3
[0.10.2]: https://github.com/sharkwedy/holyscreen/releases/tag/v0.10.2
[0.10.1]: https://github.com/sharkwedy/holyscreen/releases/tag/v0.10.1
[0.10.0]: https://github.com/sharkwedy/holyscreen/releases/tag/v0.10.0
