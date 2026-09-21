# Language and Git conventions

## Repository language

English is the canonical language for the repository and GitHub collaboration:

- README and technical documentation;
- source comments and public identifiers;
- issues, pull requests and release notes;
- commit messages and tags;
- CI output and contribution instructions.

User-facing Plasma strings use English as the source language and are localized
through KDE's `i18n()` helper. The wallpaper currently ships an Italian catalog
under `package/contents/locale/it/LC_MESSAGES/`; English is the fallback when no
translation is available.

The settings page follows the active Plasma/system locale. It does not add a
second per-wallpaper language selector, so locale changes should be followed by
reloading the wallpaper configuration page or restarting `plasmashell`.

Translation sources and build files live in:

```text
package/translate/it.po
package/translate/build.sh
package/contents/locale/it/LC_MESSAGES/plasma_wallpaper_org.xmbflow.interactive.mo
```

Run `package/translate/build.sh` after changing the catalog. Keep message IDs in
English and preserve `%1` placeholders exactly.

## Localized documentation

Translations may be added without duplicating the canonical document structure:

```text
docs/INSTALLATION.md       canonical English document
docs/INSTALLATION.it.md    Italian translation
docs/INSTALLATION.de.md    German translation
```

A localized document must link back to its English source and should be updated
when the source document changes.

## Git conventions

- Use English Conventional Commit messages, for example:
  `fix: preserve multiscreen phase across output seams`.
- Use `main` as the default branch.
- Use English tag annotations such as `Release v1.2.0`.
- Keep generated artifacts, local logs, backups and credentials out of Git.
- Keep line endings normalized through `.gitattributes`.
- Do not put secrets in commit messages, issues, pull requests or CI logs.

The repository is already configured with English GitHub Actions and contribution
metadata. See `CONTRIBUTING.md` for the development workflow.
