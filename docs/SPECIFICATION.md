# Specifica del progetto

Versione di riferimento: 1.1.0
Plugin Plasma: `org.xmbflow.interactive`
Repository locale: `xmb-native-plasma`

## Obiettivo

Fornire un wallpaper nativo per KDE Plasma 6 ispirato all'estetica XMB di
PlayStation 3, con rendering interattivo di onde e particelle tramite Qt Quick
OpenGL e integrazione con Wayland.

Il progetto deve essere installabile per l'utente corrente senza modificare i
file di sistema e deve poter essere rimosso con uno script dedicato.

## Requisiti funzionali

1. Il wallpaper deve essere riconosciuto da Plasma come `Plasma/Wallpaper`.
2. Il renderer deve usare `QQuickFramebufferObject` e OpenGL.
3. La configurazione deve essere disponibile da Plasma tramite `config.qml`.
4. Le impostazioni devono includere almeno qualità mesh, FPS, particelle,
   velocità, luminosità, interazione mouse e pausa.
5. Il mouse deve deformare localmente il campo quando l'interazione è attiva.
6. Lo zoom deve usare il puntatore come pivot e rispettare i limiti definiti
   da `src/zoomconstraints.h`.
7. Il rendering multischermo collegato deve usare una superficie virtuale
   globale, non una fase indipendente per ogni monitor.
8. Il wallpaper deve poter sospendere il rendering quando è nascosto, coperto
   oppure messo in pausa manualmente.
9. Il componente `SystemUsage` deve aggiornare CPU/GPU ogni 300 ms quando è
   presente nella configurazione.
10. La rimozione deve disinstallare il package Plasma e lo script KWin del
    progetto senza cancellare le impostazioni generali di Plasma.

## Requisiti non funzionali

- C++20.
- CMake 3.22 o superiore.
- Qt 6.6 o superiore: Core, Gui, Quick, Qml, OpenGL e DBus.
- Build out-of-source con Ninja consigliata.
- Test automatici eseguibili con CTest.
- Nessuna dipendenza da Qt WebEngine, Chromium o HTML.
- Nessun segreto, configurazione personale o binario generato nel repository.

## Architettura

```text
Plasma WallpaperItem
├── package/contents/ui/main.qml
│   └── XmbRendererItem
│       └── QQuickFramebufferObject
│           ├── mesh e proiezione globale
│           ├── texture spline CPU/GPU
│           ├── shader delle onde
│           └── shader delle particelle
├── package/contents/ui/config.qml
│   └── impostazioni Plasma + SystemUsage
├── src/xmbnativeplugin.cpp
│   └── registrazione dei tipi QML nativi
└── kwin-script/contents/code/main.js
    └── rilevamento output coperti e pausa per monitor
```

## Contratto multischermo

Ogni istanza del wallpaper riceve l'origine e la dimensione del proprio output,
la dimensione del desktop virtuale e una dimensione di riferimento. Il renderer
calcola il campo nella stessa superficie logica globale e ritaglia il risultato
sul monitor locale. Questo mantiene continuità di fase su giunzioni orizzontali
e verticali, anche con monitor di dimensioni differenti.

La modalità diagnostica deve mostrare una griglia globale continua. Lo script
`scripts/multiscreen-diagnose.sh` stampa la topologia reale e un esempio di
mappatura; i valori DP-1/DP-2 presenti nello script sono solo il caso usato
nello sviluppo, non un requisito hardware.

## Contratto SystemUsage

- CPU: lettura dell'aggregato `cpu` da `/proc/stat`.
- Frequenza: timer Qt preciso da 300 ms.
- GPU: enumerazione di tutte le schede in `/sys/class/drm/card*`.
- Metriche dirette preferite: `gpu_busy_percent` e `gt_busy_percent`.
- Fallback: differenza dei contatori `engine/*/busy_time`.
- Più GPU: viene mostrata la scheda con il valore diretto più alto.
- Sistema non Linux o driver senza metriche DRM: il dato GPU non è garantito.

## Installazione e rollback

La build genera il plugin direttamente in:

```text
package/contents/ui/xmbnative/libxmbnativeplugin.so
```

Lo script di installazione:

1. verifica gli strumenti necessari;
2. compila il progetto;
3. rimuove il vecchio package con lo stesso ID, se presente;
4. installa il package Plasma per l'utente corrente;
5. riavvia `plasmashell`;
6. installa e ricarica lo script KWin di supporto.

L'installazione modifica lo stato della sessione Plasma corrente. Prima di
usarla su una configurazione importante è necessario creare un backup del
package installato. Per un rollback usare `uninstall.sh` e reinstallare la
versione precedente.

## Non-obiettivi

- Supporto a Plasma 5.
- Supporto a Windows o macOS.
- Compatibilità con desktop diversi da KDE Plasma.
- Cifratura o autenticazione: non applicabili a questo tipo di package.
- Misurazione energetica precisa per singolo wallpaper.
