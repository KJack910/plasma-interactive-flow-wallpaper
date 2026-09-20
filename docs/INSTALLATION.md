# Installazione

## Prerequisiti

Ambiente supportato:

- Linux con sessione KDE Plasma 6;
- Wayland consigliato e verificato;
- CMake 3.22+;
- Ninja;
- compilatore C++20;
- Qt 6.6+ con Core, Gui, Quick, Qml, OpenGL e DBus;
- `kpackagetool6`, `kwriteconfig6`, `qdbus6` e `plasmashell`;
- OpenGL 3.3 o superiore per il renderer.

Per la pausa automatica sui monitor coperti è necessario anche il supporto allo
script KWin e il comando `kscreen-doctor` per la diagnostica dei display.

## Arch Linux, CachyOS e derivate

Lo script può installare le dipendenze di compilazione:

```bash
./scripts/install.sh --deps
```

Senza installare dipendenze:

```bash
./scripts/build.sh
```

## Build manuale

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
```

La libreria viene prodotta in:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

## Test e lint

```bash
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
```

## Installazione nella sessione Plasma corrente

Dopo build e test:

```bash
./install.sh
```

Oppure:

```bash
./scripts/install.sh
```

Lo script installa il package per l'utente corrente, riavvia `plasmashell` e
installa lo script KWin `xmbfullscreenbridge`.

Aprire poi:

```text
Tasto destro sul desktop → Configura desktop e sfondo → XMB Interactive Flow
```

## Diagnostica

```bash
./scripts/diagnose.sh
./scripts/multiscreen-diagnose.sh
```

Per i log di Plasma:

```bash
journalctl --user -u plasma-plasmashell.service -f
```

## Rimozione

```bash
./uninstall.sh
```

La rimozione disinstalla il package e disabilita lo script KWin del progetto.

## Altre distribuzioni

La compilazione è portabile verso altre distribuzioni Linux se sono disponibili
le dipendenze equivalenti. Lo script `--deps`, però, è attualmente specifico
per pacman/Arch e non tenta di indovinare i nomi dei pacchetti per Debian,
Fedora o altre distribuzioni.

Su queste distribuzioni installare manualmente:

- CMake e Ninja;
- compilatore C++20;
- Qt 6 Core, Gui, Quick, Qml, OpenGL e DBus;
- strumenti KDE Plasma 6 per KPackage, KWin e plasmashell.

Dopo l'installazione manuale, eseguire la build e i test con i comandi sopra.
La fase di installazione resta legata alla presenza di una sessione Plasma 6
attiva e ai comandi KDE indicati.
