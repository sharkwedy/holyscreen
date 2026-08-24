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

The contract covers every operator and output QML file. It rejects raw labels,
titles, placeholders, accessible names and tooltips while leaving technical
example URLs untranslated. Any new QML surface must be added to the `qmlFiles`
list in `TranslationCatalogTest.cpp` together with complete entries in both
catalogs. The embedded PWA has its own equivalent contract.

The embedded PWA chooses pt-BR by default, follows an English browser locale
on first use and persists an explicit pt-BR/en-US selection in local storage.
Its catalogs are embedded as JSON in `index.html`; `test_pwa_translation`
requires identical, non-empty key sets, matching placeholders and coverage of
every static or runtime translation key. The service worker cache version must
change whenever the embedded shell changes.
