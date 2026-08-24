# Changelog

Notable HolyScreen changes are documented here. The project follows Semantic
Versioning where practical before 1.0.

## [Unreleased]

- Extracted live messages, alerts, lower thirds and timers into
  `LiveCommunicationDialog`, and removed inaccessible duplicate dialogs from
  `MainWindow`.
- Consolidated translation selection, licensed Bible imports, source updates
  and reference search in a `BibleSettingsFlow` backed only by `BibleContext`.
- Extracted folder management, media catalog filtering and playlist insertion
  from `MainWindow` into a Media-context-backed `MediaLibraryDialog`.
- Extracted the operator preview, transport controls and per-screen media
  routing into `PlaybackPanel`, connected directly to Media and Output contexts.
- Extracted the reorderable media playlist and save dialog into
  `PlaylistPanel`, and promoted the transport button styling to the reusable
  `PlayerButton` component.
- Extracted the searchable lyrics, audio, video and image library from the
  Dashboard into a dedicated `LibraryPanel` connected only to `MediaContext`.
- Completed `MediaContext` coverage for lyrics and direct file imports, and
  migrated the remaining operator QML consumers from the temporary root media
  aliases.
- Extracted the Dashboard Bible browser into a dedicated `BiblePanel` component
  with encapsulated selection and search state, without changing the operator
  workflow.
- Completed the `BibleContext` facade for translation selection, search,
  imports and slide rendering, and migrated operator and output QML away from
  the temporary root-controller Bible aliases.
- Migrated QML screen routing, identification and blackout consumers to
  `OutputContext`, with a contract guard against temporary root aliases.
- Migrated every operator QML media consumer to `MediaContext` and added a
  contract guard that prevents new uses of the temporary root-controller media
  aliases.
- Added persistent audio-output selection with hot-plug fallback to the system
  default, profile import/export support and a truthful onboarding audio gate.
- Extracted the operator header and favorites into a dedicated component, and
  connected the previously inert Agenda action to the service-playlist editor
  with destructive-history confirmation.
- Removed the hidden legacy operator workspace that was still instantiated
  behind the active modular dashboard, together with its stale shortcuts and
  duplicate bindings.
- Localized operator-facing C++ status, validation, automation, integration,
  media, Bible, backup and remote-control messages, and formatted authorized
  executable timestamps using the selected locale.
- Extended the translation contract to every operator and external-output QML
  surface, including Audience, Stage, Broadcast, previews, overlays, clocks and
  presentation layers.
- Localized the main operator window in pt-BR/en-US, including dialogs, media
  library, live controls, legacy panels, dynamic player states and accessible
  names for icon-only actions; expanded the translation contract to titles and
  tooltips.
- Localized every Settings tab in pt-BR/en-US, including profiles, shortcuts,
  displays, media, background, clock, remote control, and development options;
  also removed the Settings shortcut delegate's QML lint warnings.
- Localized Dashboard library, player, display selection, playlist, Bible, and
  media context actions in pt-BR/en-US, with accessible transport names.
- Localized the complete Automations editor and authorized-process dialogs in
  pt-BR/en-US, including weekday labels and accessible action names.
- Localized the complete Integrations editor in pt-BR/en-US, including dynamic
  field labels, secret-storage warnings, actions, history, and confirmations.
- Localized Stage output labels, output-window titles, and Broadcast profile
  controls in pt-BR/en-US with translation-contract coverage.
- Localized Bible navigation and the keyboard-driven quick reference search in
  pt-BR/en-US, including validation feedback and accessible field names.
- Localized the guided onboarding checklist, status messages, and actions in
  pt-BR/en-US and added the screen to the QML translation contract.
- Added complete pt-BR/en-US catalogs and a persistent language selector to
  the offline web remote, including dynamic state messages and a contract test
  for keys, placeholders and service-worker cache replacement.
- Added complete operator manuals, quick service checklists and troubleshooting
  guides in pt-BR and en-US, covering displays, codecs, audio, remote control,
  OBS, Bible data, backups and safe shutdown.
- Added a shared `sccache` compiler cache and the Ninja generator to the
  Windows, macOS and Linux CI jobs while preserving clean configure, complete
  CTest and QML lint runs.
- The update checker now reads published releases directly from the official
  `sharkwedy/holyscreen` GitHub Releases API, including pre-releases. It
  rejects drafts and untrusted URLs, applies timeout and response-size limits,
  compares SemVer versions and only accepts platform assets with valid GitHub
  SHA-256 metadata. It never downloads or installs an update automatically.
- Added bounded Output, Media, Bible, Event, Integration, Automation and
  Maintenance QML facades while preserving the compatible
  `ApplicationController` aliases. Event, history, maintenance and primary
  operator controls now consume the new contexts incrementally.
- Added versioned operator profiles with full validation before application,
  a recursive secret-field denylist and a 1 MiB limit. Remote credentials,
  tokens and protected media are never exported or imported.
- Added the re-openable guided setup for screens, audio, media library, Bible,
  remote control and Broadcast, plus persisted operator locale, demo-mode and
  editable shortcut preferences.
- Persisted the operator window and splitter layout and added a Restore layout
  action.
- Extracted Events/History and Maintenance/Diagnostics from the monolithic
  operator window into focused QML areas with accessible control names.
- Added Qt Linguist catalogs for pt-BR and en-US, locale loading before QML,
  restart guidance and a unit contract that rejects missing, unfinished or
  placeholder-incompatible translations in migrated components.

- Added the offline automation domain: triggers, declarative conditions with
  `all`/`any` grouping, ordered actions, and runs, with no scripting language.
- Added AutomationEngine with correlation propagation, reentrancy blocking,
  chain depth, action, concurrency, debounce and time-budget limits, automatic
  disabling after consecutive failures, a global switch, and a dry-run that
  never reaches the network, MIDI, OSC, OBS or a process.
- Added the authorized executable allowlist: disabled by default, canonical
  paths that resolve symlinks, absolute paths only, list arguments, validated
  working directory, minimal environment, timeout and output caps, plus a
  process runner that never goes through a shell.
- Added migration 5 with `automations`, `automation_conditions`,
  `automation_actions`, `automation_runs` and `authorized_executables`, and the
  SQLite repository with ordered conditions and actions and run pruning.
- Documented the automation model in `docs/AUTOMATIONS.md`.
- Wired the automation engine into the application: domain events become
  triggers through `TriggerTranslator`, actions run through the CommandBus, the
  integration engine and the authorized process runner, and every run is
  recorded and pruned.
- Added the **Automações** operator area with the QUANDO → SE → ENTÃO editor,
  action reordering, inline validation, dry run, resume, confirmed deletion,
  run history, a global pause switch and the authorized executables list.
- Refused automations that reference commands outside the catalog, unknown
  integrations or unauthorized executables before saving them.
- Added a local-time scheduler with an injectable clock, one occurrence per
  local minute and trigger filters for `time` and `daysOfWeek`, including
  timezone-offset-aware daylight-saving transitions.
- Connected accepted remote commands and timer start/finish facts to the
  automation engine, including natural countdown expiration and correlation
  propagation; starting a song now emits both presentation and song triggers.
- Added versioned JSON automation import/export. Imports are fully validated,
  never overwrite existing IDs, contain no secrets and disable definitions
  that reference missing integrations or unauthorized executables.
- Added Qt Quick coverage for the automation editor and a cross-platform
  process helper covering execution, timeout, cancellation and output caps on
  Windows, macOS and Linux.

## [0.12.0] - 2026-08-23

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
- Added the Broadcast output: transparent or chroma background, 16:9 and 9:16
  composition presets, independent safe areas, per-overlay switches, and a
  preview in the operator, all persisted per output by migration 3
  (`output_broadcast_profiles`).
- Added the `output.broadcast-profile.set` command with partial updates,
  validation, and undo/redo.
- Warned in the settings when the platform cannot guarantee window
  transparency, instead of silently rendering a black background.
- Added the `presenter-integrations` library with the integration domain,
  ports, and an engine that selects the adapter by type, validates before
  persisting, applies timeouts and limited retries, cancels on shutdown, and
  publishes sanitized results.
- Added migration 4 with `integration_definitions` and
  `integration_call_history`, indexed by integration and date and pruned by a
  configurable retention.
- Added `ISecretStore` with the macOS Keychain, the Windows Credential Manager,
  and the Linux Secret Service, falling back to an in-memory store that
  declares itself non-persistent instead of writing plaintext.
- Documented the integration contracts in `docs/INTEGRATIONS.md`.
- Added the outbound HTTP adapter: `http`/`https` only, templated URL, headers
  and body, header secrets resolved from the vault, validated TLS, redirect,
  timeout and response-size limits, an allowlisted response metadata, and a
  `HEAD` connection test that never triggers the webhook.
- Added the WebSocket client adapter with `ws`/`wss`, templated text or JSON
  messages, limited backoff reconnection, observable state, and message and
  queue caps.
- Added the MIDI adapter over RtMidi 6.0.0 (pinned by checksum) with output
  port listing, Note On/Off, Control Change and Program Change, validated
  channel and values, and hot-plug handling without crashing.
- Added the OSC over UDP adapter with an OSC 1.0 encoder covered by known-byte
  tests, int32/float32/string/bool arguments, a datagram limit, and no listener
  opened by default.
- Added the OBS WebSocket v5 adapter with the challenge-response handshake,
  correlated request IDs, per-request timeouts, scene, recording, streaming and
  input operations, and a `GetVersion` connection test, covered by a fake
  conformance server.
- Added the **Integrações** operator area outside the settings dialog, with a
  searchable list, per-adapter editor, enable switch, duplication, confirmed
  deletion, connection test, non-modal status and sanitized history.
- Added the desktop-only `integration.test` and `integration.execute` commands;
  the remote catalog does not expose them.
- Added the integration summary and the secret store backend to the diagnostic
  export, without any configuration value.

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
