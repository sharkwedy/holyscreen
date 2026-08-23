# Operator profiles

HolyScreen operator profiles are portable JSON documents for moving a local
operator setup between installations. They do not contain media files, Bible
content or credentials.

## Contract

- `documentType`: `holyscreen.configuration`
- `schemaVersion`: `1`
- `profile`: the validated configuration object
- maximum document size: 1 MiB

The whole document is validated before any setting is applied. Unknown fields,
unsupported locales, invalid ports and values outside accepted ranges reject
the import. Keys named like password, token, credential, API key or secret are
rejected recursively at any depth.

The profile may contain locale, demo mode, presentation appearance, media
behavior, Bible translation selections, the local remote interface and port,
library paths, output routing, onboarding state and keyboard shortcuts. It
never enables the remote server, transfers its password or copies protected
media.

Import and export are available in **Settings > General > Operator profile**.
The guided setup can also be reopened from this section.
