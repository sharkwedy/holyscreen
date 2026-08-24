# Internationalization

HolyScreen uses Qt Linguist catalogs embedded under `/i18n`. Brazilian
Portuguese (`pt-BR`) is the default locale and English (`en-US`) is the first
complete translated locale targeted for 1.0.

## Workflow

Visible QML strings in migrated components must use `qsTr()`. C++ strings
shown to the operator must use `tr()` or `QCoreApplication::translate()`.
Command IDs, JSON fields, API paths, file paths and schema identifiers remain
untranslated.

Update the catalogs with the pinned Qt `lupdate` executable, finish both
translations and preserve every `%1`, `%2` or later placeholder. The
`test_translation_catalog` unit test verifies that migrated QML contains no
raw visible literal, both catalogs contain each source message, no translation
is unfinished and placeholders match.

Changing the locale in Settings persists the selection. HolyScreen loads the
matching catalog before constructing the application controller and QML
engine, so the interface shows a restart notice after the selection changes.

## Migração incremental

The contract currently covers Events, Maintenance, guided onboarding, Bible
navigation, quick Bible search, Integrations, Broadcast settings, Stage labels
and output window titles. Add a QML file to the `qmlFiles` list in
`TranslationCatalogTest.cpp` as soon as all of its visible strings are wrapped
and both catalogs are complete. The 0.14.0 checkpoint requires every operator,
output and PWA surface to be covered.

The embedded PWA chooses pt-BR by default, follows an English browser locale
on first use and persists an explicit pt-BR/en-US selection in local storage.
Its catalogs are embedded as JSON in `index.html`; `test_pwa_translation`
requires identical, non-empty key sets, matching placeholders and coverage of
every static or runtime translation key. The service worker cache version must
change whenever the embedded shell changes.
