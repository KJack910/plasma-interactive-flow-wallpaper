# XMB Interactive Flow — Native Plasma 6 / Wayland

Versione nativa del background XMB basata sulla v24 del progetto web. Non usa
Qt WebEngine, Chromium o HTML: wave e particelle vengono renderizzate da un
`QQuickFramebufferObject` C++ con OpenGL dentro `plasmashell`.

Documentazione:

- [Specifiche](docs/SPECIFICATION.md)
- [Installazione](docs/INSTALLATION.md)
- [Audit di portabilità](docs/PORTABILITY.md)
- [Verifica e test](docs/TESTING.md)
- [Contribuzione](CONTRIBUTING.md)

La valutazione misurata di prestazioni, consumo e fattibilità Vulkan è in
[PERFORMANCE.md](PERFORMANCE.md).

Lo zoom-in usa il puntatore per esplorare l'intero dominio virtuale a ogni
ingrandimento, dal minimo di 0,40× al massimo di 3,50×, su entrambi gli assi.
Shift+rotella sposta la camera verso il puntatore senza cambiare ingrandimento,
anche esattamente a 0,40×. Lo zoom-out normale mantiene il pivot sotto il
puntatore; se si tenta di andare oltre il minimo, la superficie viene
ricentrata automaticamente.
Nel multischermo il fuoco dello zoom-in usa il centro del monitor sotto il
puntatore, poi viene espresso nella camera globale: i flussi rimangono continui
alla giunzione e questa non diventa un falso bordo.
La mesh dei flussi viene disegnata solo per le colonne potenzialmente visibili
su ciascuno schermo, con un intervallo derivato dalla prospettiva e un margine
per la deformazione.

## Target

- CachyOS / Arch Linux
- KDE Plasma 6.7+
- Wayland
- Qt 6.6+
- GPU con OpenGL 3.3

## Installazione rapida

```bash
cd xmb-native-plasma
./scripts/install.sh --deps
```

Le esecuzioni successive non richiedono `--deps`:

```bash
./scripts/install.sh
```

Poi apri:

**Tasto destro desktop → Configura desktop e sfondo → XMB Interactive Flow**

## Impostazioni Plasma incluse

- Qualità mesh: 96 / 140 / 180 / 220 / 320 / 440×320 / 560×384 / 720×480
- FPS target: 15–240
- Numero particelle: 100–30000
- Profilo particellari: piccoli / grandi / misti
- Simulazione particellari: pseudo-2D / 3D volumetrica, con scie nella modalità 3D
- Velocità onda
- Velocità particelle
- Forza interazione mouse
- Luminosità
- Interazione mouse on/off
- Pausa quando il wallpaper non è visibile
- Pausa/ripresa automatica per monitor quando le finestre coprono l'intera area utile
- Blocco manuale completo del wallpaper e del suo rendering GPU, disattivabile per riprenderlo

Con il risparmio energetico per monitor, KWin osserva la copertura geometrica
dell'output, anche quando un'applicazione non dichiara correttamente fullscreen
o borderless e quando più finestre affiancate coprono insieme l'area utile.
Tramite QtDBus, ogni istanza
del wallpaper sospende il proprio timer e rimuove il framebuffer dalla scena;
gli altri monitor
continuano a usare lo stesso tempo globale. Alla riapertura, Qt Quick aggiorna
il framebuffer senza riavviare l'animazione o ricreare la texture. Su Wayland
non viene usata la lista delle finestre X11, che non è disponibile al wallpaper.
La pausa automatica degli schermi coperti e quella manuale sono impostazioni
indipendenti; se la pausa manuale è attiva, scoprire lo schermo non riavvia il
rendering finché non viene disattivata.

Il preset **Ultra** usa una griglia 220×220 con indici GPU a 32 bit, evitando
l'overflow `Uint16` che generava gli artefatti geometrici nella versione web.

## Mouse

Sulle zone vuote del desktop Wayland il renderer riceve il puntatore dal
`MouseArea` del wallpaper:

- hover: deformazione locale di wave + particelle;
- pressione sinistra: la deformazione scende gradualmente verso zero;
- rilascio: ritorna gradualmente, anche continuando a muovere il mouse.

Icone, pannelli e finestre stanno sopra il wallpaper e mantengono il proprio
input. Il plugin non tenta di aggirare le restrizioni globali di Wayland.

## Diagnostica

```bash
./scripts/diagnose.sh
```

Log completo di Plasma:

```bash
journalctl --user -u plasma-plasmashell.service -f
```

## Rimozione

```bash
./scripts/uninstall.sh
```

## Architettura

```text
Plasma WallpaperItem
   ├── config.qml + main.xml       impostazioni native Plasma
   ├── MouseArea                   input Wayland sul desktop esposto
   └── XmbRenderer                 QML type C++
         └── QQuickFramebufferObject
              ├── gradient pass
              ├── displacement texture 1024×128
              ├── dual wave mesh
              └── additive particles
```

## Nota sulla fedeltà v24

Gli shader di wave e particelle sono portati dalla pipeline WebGL v24. La
texture carrier della spline viene ricostruita nativamente con una sintesi
procedurale equivalente a bassa frequenza; il movimento principale, le due
wave, i filamenti, Fresnel, prospettiva, particelle e interazione sono eseguiti
dagli stessi modelli GLSL della v24.


## v1.0.1 - Qt 6 QML registration fix

`XmbRendererItem` is intentionally **not** declared `final`. `qmlRegisterType<T>()`
creates an internal `QQmlElement<T>` subclass, so marking `T` final prevents Qt 6
from compiling the QML registration wrapper. The project uses manual registration
from `XmbNativePlugin::registerTypes()`, therefore `QML_NAMED_ELEMENT` is not used.


- v1.1.0 aggiunge zoom con rotella, interazione del mouse non invertita e flussi multipli configurabili sopra/sotto.


- v1.1.0: mouse effect restored to v1.0.2 behavior; wheel zoom and multi-flow controls are retained.


### v1.1.0 flow zones
Upper, center and lower flow counts are now independent. Upper/lower bands are placed in the actual upper/lower thirds of the screen; center bands remain around the center. Mouse deformation logic remains the v1.0.2/v1.0.4 behavior.


## v1.1.0

- particles are assigned to and visually follow the active upper/center/lower flow lanes;
- wheel zoom is anchored to the cursor position instead of the screen center;
- maximum zoom increased from 1.85x to 3.50x;
- optional horizontal multi-monitor linking uses the virtual desktop X span plus a shared animation clock, so flow phase and particle motion continue across monitor seams;
- mouse deformation direction/press behavior remains the restored v1.0.2 behavior.


### v1.1.0

- same mouse deformation formula, but its source offset points toward the cursor;
- wheel zoom step reduced and interpolation slowed; configurable wheel sensitivity added;
- multiscreen X mapping now uses the actual global bounds of each wallpaper item, with a QScreen fallback, to remove phase disagreement at monitor seams.


## v1.1.0

- restores the v1.0.5/v1.0.2 interaction field coefficients, falloff, depth response, click easing and flow contribution; only the radial component is directed toward the cursor;
- replaces Wayland `mapToGlobal()` multiscreen slicing with `QScreen::geometry()` logical output coordinates;
- separates the virtual-desktop texture/particle slice from an extended horizontal carrier phase, so the wave does not restart or get compressed at monitor boundaries;
- keeps shared animation time, cursor-pivot zoom, particle-to-flow binding and zoom sensitivity from v1.0.7.


## v1.1.0 — mouse effect rollback

The mouse deformation itself is restored to the v1.0.5/v1.0.2 implementation:
same `offset`, radius, falloff, flow term, displacement, depth response and
press/release easing. No direction correction is applied inside the deformation
shader.

Dynamic zoom and multiscreen code remain separate from that interaction block.
The pointer is converted into the legacy interaction coordinate system so
cursor-pivot zoom does not alter the original deformation behavior.


## v1.1.0 — Multiscreen Foundation

This release freezes the interaction/zoom visual logic and replaces the previous multiscreen mapping with a true logical-pixel world domain.

For the supplied setup:

- DP-1: `0,0 1920x1080` -> world X `[0,1920]`
- DP-2: `1920,0 2560x1080` -> world X `[1920,4480]`
- virtual desktop: `4480x1080`
- exact shared seam: `worldX = 1920`
- phase reference width: `1920`

At the seam, DP-1 evaluates `worldX = 0 + 1*1920 = 1920`; DP-2 evaluates `worldX = 1920 + 0*2560 = 1920`. The carrier, spline lookup and particle field therefore see the same world coordinate rather than two synchronized-but-independent local phases.

### Diagnostic mode

Enable **Multischermo** and **Diagnostica**. Diagnostic mode temporarily renders at zoom 1x with pointer deformation suppressed and overlays a global 2D grid. This does not overwrite your saved zoom or mouse settings. The strong line at X=1920 must meet exactly at the DP-1/DP-2 boundary.

Run `./scripts/multiscreen-diagnose.sh` to print the KScreen topology and the expected world mapping.

### Superficie virtuale globale

Il rendering multischermo usa ora una singola superficie logica 2D. Ogni
istanza del wallpaper ricostruisce lo stesso campo globale e ritaglia soltanto
il rettangolo del proprio output. Prospettiva, zoom, deformazione, spline e
particelle vengono calcolati prima del ritaglio: non esistono più una
proiezione o un pivot separati per monitor.

La mappatura include sia X sia Y, quindi rimane continua anche con monitor di
dimensioni diverse o sfalsati verticalmente. La densità orizzontale della mesh
cresce automaticamente con la larghezza del desktop virtuale per mantenere la
qualità del singolo monitor.

L’impostazione **Segui puntatore** offre tre modalità: disattivata, solo
interazione locale, oppure traslazione del flusso globale insieme
all’interazione.
