# Versioning

## Policy

The project follows Semantic Versioning:

```text
MAJOR.MINOR.PATCH
```

- MAJOR: incompatible package, configuration or runtime behavior changes.
- MINOR: backward-compatible features or new configuration options.
- PATCH: backward-compatible fixes, documentation or packaging corrections.

## Current release

Current release: `1.1.0`

This release is the current stable Plasma 6 implementation with native OpenGL
rendering, cursor-pivot zoom, linked virtual-desktop rendering, per-output
pause support, system-usage readings and the associated regression tests.

The release number must match in:

- `CMakeLists.txt` (`project(... VERSION ...)`);
- `package/metadata.json` (`KPlugin.Version`);
- the Git tag (`v1.1.0` for this release).

Run the version consistency check with:

```bash
./scripts/check-version.sh
```

## Release workflow

1. Update the CMake and package versions together.
2. Add one concise entry to `CHANGELOG.md` under the new version.
3. Run the complete build, CTest, QML lint and shell syntax checks.
4. Update the README only when the user-facing scope or commands change; do
   not duplicate the complete changelog there.
5. Commit in English using a Conventional Commit message.
6. Create an annotated Git tag:

   ```bash
   git tag -a v1.2.0 -m "Release v1.2.0"
   git push origin main --follow-tags
   ```

7. Verify the GitHub Actions result before calling the release complete.

## Git history

Release tags are the authoritative milestones. Commit history records
implementation steps; it should not repeat the full release notes in every
commit or documentation page.

## Pre-release versions

Use SemVer suffixes such as `1.2.0-rc.1` for release candidates. Do not use
version names based on local machines, dates or backup directory names.
