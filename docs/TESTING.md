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

Public internet tests are opt-in. Set `HOLYSCREEN_NETWORK_TESTS=1` only when
validating a known public Bible source. Normal CI and local suites remain fully
offline after dependencies are available.

Release validation additionally covers packages on clean systems, the physical
operator/two-output topology, mixed DPI, a phone PWA and a two-hour endurance
session. Record those results in the release validation report; automated tests
do not replace the physical checks.
