# Language and Git conventions

## Repository language

English is the canonical language for the repository and GitHub collaboration:

- README and technical documentation;
- source comments and public identifiers;
- issues, pull requests and release notes;
- commit messages and tags;
- CI output and contribution instructions.

User-facing Plasma strings should remain clear and translatable. When Qt/KDE
translation catalogs are introduced, English remains the source language.

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
