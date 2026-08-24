# Testing HolyScreen

HolyScreen keeps one CTest registry and assigns every test to one primary RC
suite. A clean Release build remains the reference configuration.

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target presenter-ui_qmllint
```

Run one suite with `ctest --test-dir build -L <label> --output-on-failure`.

| Label | Scope |
|---|---|
| `unit` | isolated domain, application and adapter contracts |
| `integration` | SQLite plus real ephemeral HTTP/WebSocket/OBS servers |
| `qml` | Qt Quick component and interaction contracts |
| `golden` | deterministic output pixels, colors, blackout and safe areas |
| `e2e` | application startup and command-to-output workflows |
| `performance` | command, slide and Full HD frame budgets |
| `endurance` | short self-driven session that validates the endurance report |

Public internet tests are opt-in. Set `HOLYSCREEN_NETWORK_TESTS=1` only when
validating a known public Bible source. Normal CI and local suites remain fully
offline after dependencies are available.

Release validation additionally covers packages on clean systems, the physical
operator/two-output topology, mixed DPI, a phone PWA and a two-hour endurance
session. Record those results in the release validation report; automated tests
do not replace the physical checks.

The two-hour session runs through the executable, not through CTest. See
[`ENDURANCE.md`](ENDURANCE.md) for the options, the report schema and the
blocker thresholds.

## Synthetic media

Media validation never uses the operator library or any protected content.
`tools/make-synthetic-media.sh` (and `tools/make-synthetic-media.ps1` on
Windows) generates everything from ffmpeg sources: a 1080p60 H.264 reference
clip, the same clip with a deliberate audio dropout, a VP9/Opus clip for a
different decode path, WAV and AAC tones, images at 640x480, 1920x1080 and
3840x2160, and an unreadable file for the missing-codec path.

```sh
media_dir="$(tools/make-synthetic-media.sh)"
holyscreen --endurance --endurance-minutes=120 --endurance-media="$media_dir"
```

Nothing is committed: the default destination is a temporary directory and the
script prints its path on the last line.

The working report for the next candidate is
[`releases/1.0.0-rc.1-validation.md`](releases/1.0.0-rc.1-validation.md).
