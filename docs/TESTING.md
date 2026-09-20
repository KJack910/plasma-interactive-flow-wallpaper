# Strategia di verifica

## Verifica automatica

La suite CTest copre:

- limiti dello zoom;
- visibilità della mesh;
- geometria e deduplicazione della mesh;
- equivalenza della spline CPU/GPU;
- percorso GPU della spline;
- visibilità e pausa per output.

Comandi:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure
qmllint package/contents/ui/main.qml package/contents/ui/config.qml
bash -n install.sh uninstall.sh scripts/*.sh
```

## Verifica manuale post-installazione

1. Installare il package nella sessione Plasma.
2. Aprire la configurazione del wallpaper.
3. Verificare che la pagina impostazioni sia visibile.
4. Verificare hover, pressione e rilascio del mouse.
5. Verificare rotella, zoom minimo/massimo e sensibilità.
6. Verificare i preset mesh e particelle.
7. Su più monitor, abilitare `Multischermo` e `Diagnostica`.
8. Controllare la continuità della griglia sulla giunzione.
9. Coprire completamente un monitor con una finestra e verificare la pausa.
10. Usare `scripts/diagnose.sh` e controllare i log di `plasmashell`.

## Criteri di accettazione

Una release è pronta solo se:

- la configurazione CMake termina correttamente;
- tutti i test CTest terminano con esito positivo;
- `qmllint` non segnala errori;
- gli script Bash superano `bash -n`;
- il package Plasma viene installato e riconosciuto;
- il wallpaper resta visibile dopo il riavvio di `plasmashell`;
- nessun build artifact o backup locale è tracciato da Git;
- il rollback tramite `uninstall.sh` è eseguibile.

La verifica automatica non sostituisce la verifica visiva del renderer e del
comportamento multischermo.
