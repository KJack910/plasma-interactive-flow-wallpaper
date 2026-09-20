# Audit di portabilità e generalità

Audit eseguito il 20 settembre 2026 sul progetto `xmb-native-plasma`.

## Risultato sintetico

Il sistema è generale all'interno del seguente perimetro:

```text
Linux + KDE Plasma 6 + Qt 6.6+ + OpenGL 3.3+ + sessione utente Plasma
```

Non è un wallpaper multipiattaforma in senso Windows/macOS. La parte grafica
C++ usa Qt, ma l'integrazione, l'installazione e il monitoraggio hardware usano
interfacce Linux/KDE specifiche.

## Matrice di compatibilità

| Area | Stato | Perimetro verificato |
|---|---|---|
| Compilazione C++ | Generale su Linux con Qt 6 | CMake 3.22+, Ninja, C++20 |
| Rendering | Dipendente dalla GPU | OpenGL 3.3+ |
| Plasma package | Specifico KDE | Plasma 6, `kpackagetool6` |
| Sessione | Specifica desktop | KDE Plasma, Wayland verificato |
| Multischermo | Generale nel modello logico | Layout rettangolari, anche sfalsati |
| CPU usage | Specifico Linux | `/proc/stat` |
| GPU usage | Specifico Linux/DRM | `gpu_busy_percent`, `gt_busy_percent`, `busy_time` |
| Pausa per output | Specifica KDE/KWin | QtDBus + script KWin |
| Dipendenze automatiche | Limitata | `pacman`/Arch attualmente |
| Windows/macOS | Non supportati | nessun package/installer previsto |
| Plasma 5 | Non supportato | API e strumenti Plasma 6 |

## Verifiche eseguite localmente

Sul sistema di sviluppo attuale sono stati verificati:

- CMake disponibile;
- Ninja disponibile;
- Qt 6 e `qmllint` disponibili;
- `kpackagetool6`, `plasmashell`, `systemctl` e `qdbus6` disponibili;
- configurazione CMake riuscita;
- compilazione Release riuscita senza lavoro pendente;
- 6 test CTest superati;
- lint QML superato senza output di errore;
- sintassi Bash verificata per gli script di installazione, rimozione,
  build e diagnostica.

Queste verifiche dimostrano la riproducibilità della build nel target Linux/KDE
locale. Non dimostrano una build su Debian, Fedora, Windows o macOS.

## Perché non è completamente generico

1. `/proc/stat` e `/sys/class/drm` non esistono su Windows/macOS.
2. `kpackagetool6`, `kwriteconfig6`, `qdbus6` e `plasmashell` appartengono allo
   stack KDE Plasma.
3. Il riavvio usa `systemctl --user` e presuppone una sessione systemd utente.
4. `scripts/install.sh --deps` usa pacman e nomi pacchetto Arch.
5. Il renderer è `QQuickFramebufferObject` OpenGL; non usa il backend Vulkan
   della singola wallpaper instance.
6. Le impostazioni e il package metadata sono specifici di `Plasma/Wallpaper`.

## Cosa è riutilizzabile fuori dal sistema di sviluppo

- il renderer C++/Qt e la maggior parte degli shader;
- i test matematici di mesh, spline, zoom e visibilità;
- il modello della superficie virtuale multischermo;
- la struttura CMake;
- il package Plasma, su altre distribuzioni Linux con Plasma 6;
- il fallback GPU DRM su driver che espongono i contatori previsti.

## Cosa serve per aumentare la generalità

Per supportare più distribuzioni Linux:

- aggiungere documentazione dei pacchetti equivalenti;
- separare la build dalla fase di installazione Plasma;
- aggiungere un installer senza `sudo pacman` obbligatorio;
- testare almeno una distribuzione Debian/Ubuntu e una Fedora/KDE;
- rendere opzionali le integrazioni KWin e SystemUsage.

Per supportare Windows/macOS sarebbe necessario un porting separato di package,
installazione, lettura CPU/GPU, lifecycle del desktop e integrazione del
wallpaper; non è un'estensione della sola build CMake.
