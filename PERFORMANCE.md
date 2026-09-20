# Valutazione OpenGL/Vulkan — 16 settembre 2026

## Sistema e carico misurato

- Plasma su due monitor, superficie virtuale 4480×1080; Qt 6.11.2.
- AMD Radeon RX 5700 XT, `amdgpu`/RADV (Mesa 26.2.2); Vulkan 1.4 disponibile.
- Preset realmente configurato su entrambi i monitor: qualità 4, 9 flussi
  (3+3+3), 30.000 particelle 3D, 60 FPS, superficie globale collegata.
- Campionamento di 6 secondi per fase, in sequenza attivo → pausa completa →
  attivo. Pausa applicata temporaneamente a entrambi gli output tramite il
  bridge D-Bus del wallpaper; in seguito lo script KWin è stato ricaricato per
  ristabilire lo stato di copertura reale. Nessuna impostazione utente modificata.

| Fase | Motore grafico di plasmashell | CPU di plasmashell | GPU totale | Potenza RX 5700 XT |
|---|---:|---:|---:|---:|
| Attivo A | 232 ms/s | 0,59 core | 34,1% | 41,7 W |
| Pausa | 0,006 ms/s | ~0 core | 5,7% | 33,9 W |
| Attivo B | 212 ms/s | 0,63 core | 26,6% | 41,3 W |

Il contatore `drm-engine-gfx` proviene dal file `fdinfo` DRM del processo
`plasmashell` (i suoi descrittori condividono lo stesso client ID, quindi non
sono sommati). CPU: differenza dei tick `utime+stime` divisa per `CLK_TCK=100`.
Occupazione GPU e potenza: medie di sette campioni a un secondo dai contatori
`amdgpu`. Il confronto attivo/pausa attribuisce la maggior parte della
differenza al wallpaper, ma `plasmashell` include anche pannelli e altre UI;
occupazione e potenza sono valori dell'intera GPU, non del solo XMB. La prova
è breve e non misura temperatura stabilizzata o energia su molte ore.

## Carico derivato dal codice

Alla qualità 4, con la superficie attuale, la griglia ha 1494×256 campioni e
762.448 indici per flusso intero. Senza il ritaglio per viewport, nove flussi
disegnati da due istanze produrrebbero circa 13,7 milioni di riferimenti a
vertici per frame, oppure 823 milioni/s a 60 FPS: è il limite di confronto,
non il carico tipico dopo il ritaglio descritto sotto. Ogni istanza ricalcola
inoltre una texture 1024×128 sulla CPU e la
carica ogni frame: complessivamente 15,7 milioni di texel/s, pari a 60 MiB/s
di soli dati float trasferiti. Sono stime di lavoro, non tempi misurati.

## Decisione

Non sostituire subito OpenGL con Vulkan. Il renderer usa
`QQuickFramebufferObject`, che Qt supporta solo con OpenGL; il passaggio
richiede un nuovo nodo scene graph/QRhi e shader convertiti. Inoltre il
backend della scena Qt Quick è deciso da plasmashell, non dalla singola istanza
del wallpaper. Il possibile risparmio di CPU da Vulkan non è quantificabile
senza un prototipo con la stessa scena; i dati attuali non mostrano una GPU
satura e indicano lavoro CPU procedurale e mesh ripetuta da affrontare prima.

## Riduzione dell'overscan elaborato (stima da geometria)

Il renderer conserva la sorgente larga quattro desktop virtuali per lo zoom
minimo di 0,40× e la navigazione fino ai bordi, ma per ogni monitor invia alla
GPU solo le colonne il cui intervallo prospettico può entrare nel viewport.
La riserva aggiuntiva parte da 128 pixel a schermo e cresce con la forza della
deformazione; non è più una frazione fissa dell'intero desktop. Gli estremi
vengono arrotondati a blocchi di 16 colonne per evitare upload a ogni minimo
movimento. La modalità che trasla tutta la scena col puntatore usa la stessa
selezione, includendo la traslazione nota nel calcolo inverso. Le particelle
e la texture spline non sono
ancora ritagliate: il risparmio sottostante riguarda i vertici della mesh,
non una riduzione già misurata di tempo GPU o watt.

Con 1494 colonne, due output 1920+2560 su 4480 pixel virtuali, camera
centrata e interazione ordinaria, le colonne inviate per output sono circa
529+673 a 0,40×, 225+289 a 1× e 81+97 a 3,50×, contro 1494+1494 senza
ritaglio: rispettivamente 60%, 83% e 94% in meno. Sono conteggi geometrici,
non tempi GPU; con interazione più forte la riserva cresce e il risparmio cala.
Il solo intervallo matematicamente visibile è più stretto, ma non sarebbe una
scelta sicura per prospettiva, deformazione e rasterizzazione. Le particelle
non sono incluse in questi conteggi.

Per ridurre altro carico senza abbassare il dettaglio visivo, il candidato
principale è `generateSplineTexture()`: i due renderer collegati valutano gli
stessi 131.072 texel per frame, inclusa una componente hash che non dipende
dal tempo. Si può precomputare la componente statica una volta, poi calcolare
la parte animata una sola volta per timestamp/superficie globale e farla usare
a entrambi gli output. Questo dimezzerebbe il calcolo procedurale duplicato
su due schermi, ma non gli upload finché ogni contesto mantiene una propria
texture. Una texture GPU condivisa richiede prima di verificare la condivisione
dei contesti OpenGL e la gestione della sua durata. Ridurre la risoluzione della
spline o della mesh non è il primo passo: può alterare filamenti e movimento.
Servono misure CPU e GPU a parità di configurazione prima/dopo, non dedurre i
watt dai soli conteggi di vertici.

Ordine consigliato:

1. Misurare il risparmio reale della nuova selezione di colonne per viewport.
2. Generare/condividere la texture spline una volta per frame globale oppure
   spostarne il calcolo in GPU; misurare CPU, GPU e watt.
3. Solo se resta un limite di driver/draw-call CPU, creare un prototipo QRhi
   OpenGL/Vulkan con gli stessi shader, FPS e monitor. Confrontare tempi CPU
   per frame, tempo GPU, FPS percentili e watt a parità di output visivo.

Riferimenti Qt: [QQuickFramebufferObject](https://doc.qt.io/qt-6/qquickframebufferobject.html),
[QRhi](https://doc.qt.io/qt-6/qrhi.html),
[selezione del backend Qt Quick](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html).

## Aggiornamento 17 settembre 2026 — preset Extreme+, spline e deduplicazione

Configurazione misurata: quality=5 (nuovo preset "Extreme+", griglia base
440×320, resX estesa 2054), 30.000 particelle 3D, 9 flussi, 60 FPS, due
output 1920+2560 su 4480×1080 collegati.

1. **Spline** (`src/splinetexture.h`): la componente hash statica della
   texture 1024×128 è generata una sola volta per processo; ogni frame
   calcola solo i termini dinamici. Equivalenza bit-level verificata su
   8.519.680 texel (test `xmb_splineequivalence`); benchmark: **−35…−36%**
   sul calcolo CPU della spline (≈3,0 ms → ≈1,95 ms per frame).
2. **Deduplicazione mesh** (`src/meshgeometry.h`): ogni vertice della griglia
   è memorizzato una sola volta; gli indici ricostruiscono gli stessi strip
   e raccordi. Test `xmb_meshgeometry`: 20,9 milioni di coordinate
   indicizzate identiche. VBO per output a Extreme+: **10.483.616 →
   5.258.240 byte (−49,8%)**. Triangoli, ritaglio, prospettiva e zoom
   invariati.
3. **Misura post-installazione** (plasmashell riavviato, nuove librerie
   caricate): engine GFX del processo `plasmashell` da `fdinfo` DRM
   (un solo fd; i tre fd condividono lo stesso client-id e vanno contati
   una volta): **12,3%** stabile su tre campioni da 6–10 s. Il 35–40%
   citato in precedenza era GPU totale (incluse KWin e altre app), non
   il solo engine gfx di plasmashell: i due numeri non sono confrontabili
   direttamente. A parità di metrica (gfx% di plasmashell), la riduzione
   di vertici e calcolo CPU è inclusa in questa cifra.
