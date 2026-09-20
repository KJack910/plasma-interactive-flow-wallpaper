# Pull request

## Summary

<!-- Describe the change in English and keep this section concise. -->

## Scope

- [ ] Renderer
- [ ] QML/configuration
- [ ] Multiscreen or KWin integration
- [ ] Build/install tooling
- [ ] Documentation only

## Verification

- [ ] `./scripts/check-prerequisites.sh`
- [ ] `cmake -S . -B build -G Ninja`
- [ ] `cmake --build build --parallel 1`
- [ ] `ctest --test-dir build --output-on-failure`
- [ ] `qmllint package/contents/ui/main.qml package/contents/ui/config.qml`
- [ ] `bash -n install.sh uninstall.sh scripts/*.sh`
- [ ] Manual Plasma verification, if runtime behavior changed

## Compatibility impact

<!-- State Linux/Plasma/Qt/GPU or configuration assumptions. -->

## Rollback notes

<!-- Explain how to revert or uninstall if this changes installation/runtime behavior. -->
