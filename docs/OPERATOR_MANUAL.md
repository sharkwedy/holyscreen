# HolyScreen operator manual

[Português do Brasil](OPERATOR_MANUAL.pt-BR.md)

This manual covers the offline desktop operator workflow. Screen names and
available codecs can vary by operating system. The local web remote is an
optional companion; the desktop remains the source of truth.

## 1. First launch

The guided setup checks six areas: displays, audio, media library, Bible,
remote control and Broadcast. Complete only what the service needs. The setup
can be reopened later under **Settings > General > Guided setup** and never
blocks the operator window.

Before presenting content:

1. open **Settings > Screens**, identify the physical displays and enable only
   the intended outputs;
2. assign each output as Audience, Stage or Broadcast and confirm its preview;
3. select the audio output and test volume without a live microphone open;
4. add media folders and import an authorized Bible translation;
5. create a backup after the initial configuration.

Right-click a display name to give it an operator-friendly name. Display
routes are persisted by hardware identifier, but should be checked after a
GPU, dock or cable change.

The selected audio output is stored in the local profile. If that device is
disconnected, HolyScreen temporarily uses the default output and returns to
the saved choice when it reappears. Exported profiles retain the identifier,
so verify the selection after importing a profile on another computer.

## 2. Operator workspace

**Operation** is the default area. It contains the library, preview, current
playlist and playback controls. Splitter positions and the operator-window
size are saved automatically. Use **Settings > General > Restore layout** if a
panel becomes too small or is moved outside the useful area.

The other areas group Bible, Events, Integrations, Automations and Maintenance.
Status messages are non-blocking whenever continuing is safe. Destructive
operations such as restoring a database ask for confirmation.

## 3. Media library and playlist

Add one or more folders in the library manager. HolyScreen scans supported
audio, video and image files recursively; it does not copy or take ownership
of them. Search text is preserved while switching media categories.

- Double-click a catalog item to append it to the active playlist. If nothing
  is playing, the added item starts automatically.
- Double-click a playlist item to play it immediately.
- Drag playlist rows to reorder them.
- Use the context menu to reveal a source file in the file manager or add it
  to Favorites. Favorites appear at the top of the operator window.
- Save a playlist before replacing it when it will be reused.

Moving, renaming or deleting a source file outside HolyScreen makes the item
unavailable. Rescan the library after external file changes.

The main playback button changes between Play and Pause. The speaker button
mutes and restores the previous volume. Repeat supports off, one item and the
whole playlist.

## 4. Outputs and live content

An Audience output shows the public composition. A Stage output shows
operator-oriented content such as current/next slide, clock, timer and stage
messages. Broadcast uses its own safe-area and composition profile for OBS or
another capture tool.

Media is routed to every enabled compatible output by default. Disable media
for a specific output in **Settings > Screens** when that display should keep
its text or background. The output selector below the player can be collapsed
to recover vertical space.

Blackout hides public content without stopping the current presentation. Stop
ends the presentation. Verify which behavior is desired before using either
during a service.

## 5. Bible

Open the Bible area to browse books, chapters and verses. The quick Bible
search also opens when typing while no text field or editor has focus. Type a
reference such as `John 3:16` and press Enter to present it; press Escape to
cancel.

Only import translations that the church is authorized to use. HolyScreen can
import the canonical folder format, public Git HTTPS repositories, ZIP files
and the documented legacy JSON format. Translation metadata and per-output
selection are preserved. See [Bible import](BIBLE_IMPORT.md).

## 6. Events, overlays and Stage communication

Events organize a service order. Selecting an event does not present an item;
executing an event item does. History records accepted operational actions for
recovery and review.

The Live communication panel controls the audience message, central alert,
lower third, countdown and stopwatch. Stage communication appears only on
outputs assigned to Stage. Clear temporary overlays when their purpose is
complete.

## 7. Integrations and automations

Integrations support local HTTP, WebSocket, OBS, MIDI and OSC adapters.
External processes are disabled by default and require an explicit canonical
executable allowlist. Keep credentials in the operating-system secret store,
not in exported profiles or automation files.

Automations are offline rules composed of trigger, conditions and ordered
actions. Use Dry run before enabling a new rule. Loop, concurrency, duration
and output limits remain active even for trusted rules. See
[Integrations](INTEGRATIONS.md) and [Automations](AUTOMATIONS.md).

## 8. Local web remote

Enable the server only on a trusted local network. Set a strong local password,
scan the QR code and keep the server disabled when it is not needed. Never
forward its port on the router or expose it directly to the internet. Revoke
all sessions after a shared device is lost or replaced. See the complete
[Remote API and security guide](REMOTE_API.md).

## 9. Profiles, backup and recovery

Operator profiles transfer displays, appearance, media, library and
preferences. Passwords, tokens and protected media are never included. Import
is validated completely before anything is applied; use a newly exported
profile when moving between significantly different versions.

Create database backups before upgrades and major configuration changes. A
restore replaces operational data only after explicit confirmation. Keep at
least one copy outside the presentation computer.

## 10. Updates and shutdown

The update checker reads published releases from the official HolyScreen
GitHub repository. It only reports the available version and opens its release
page; it never downloads or installs a package. Verify every downloaded asset
against `SHA256SUMS` before installation.

Close the operator window normally. HolyScreen then saves layout state, stops
local services, closes output windows and exits completely. If an operating
system forces termination, inspect Maintenance diagnostics before the next
service.

For a short service checklist, use the [quick service guide](QUICK_SERVICE_GUIDE.md).
For recovery procedures, see [troubleshooting](TROUBLESHOOTING.md).
