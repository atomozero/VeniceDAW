# VeniceDAW 3DMix Viewer - Alpha 0.1
## "L'Archeologia Digitale Incontra l'Audio Spaziale"

### 🎭 La Storia (Più o Meno Vera)

Tutto è iniziato con un CD-ROM trovato in un mercatino dell'usato a Padova nel 2024.
Sì, proprio un CD. Etichettato a mano: "BeOS R5 - Demo Audio 2001". Sì, miracolosamente funzionava ancora.

All'interno: alcuni file `.3dmix` misteriosi di BeOS R5 (1995-2001), un sistema operativo che
la maggior parte delle persone considera "quella cosa che Apple NON ha comprato prima di Mac OS X".

### 🔬 Il Reverse Engineering (AKA: Perché Siamo Masochisti)

Dopo 23 Spritz al Cinar, 12 fette di Tiramisu, 3 notti insonni, e innumerevoli bestemmie in dialetto veneto, siamo riusciti a:

1. **Aprire i file con un hex editor** - Prima scoperta: non sono XML. Seconda scoperta: sono BMessage serializzati. Terza scoperta: il formato BMessage è Big Endian perché negli anni '90 PowerPC era cool.

2. **Capire che i sample rate mentono** - Il file dice 44100 Hz ma in realtà è 22050 Hz. Perché? Forse qualcuno in Be Inc. aveva bevuto troppo caffè quella mattina del 1998. O forse è una feature™.

3. **Scoprire che i byte sono swappati** - Perché i file sono stati salvati su PowerPC (big-endian) ma li leggiamo su Intel (little-endian). Il risultato? Rumore bianco glorioso. La soluzione? Swap di tutti i sample. Sì, TUTTI.

4. **Realizzare che le coordinate 3D usano float strani** - Non sono IEEE 754 standard. No, sono "Be Inc. Special Edition Float" con rappresentazione custom. Li abbiamo convertiti osservando pattern nei dati binari e pregando.

### 🎪 Cosa Funziona (Miracolosamente)

- ✅ Caricamento progetti `.3dmix` di BeOS R5 originali
- ✅ Visualizzazione 3D con OpenGL (anche se lenta, grazie llvmpipe)
- ✅ Audio mixing spaziale con ITD (Interaural Time Difference)
- ✅ Timeline con waveform rendering on-the-fly
- ✅ VU meter che effettivamente si muovono (dopo aver corretto la scala 3 volte)
- ✅ Controlli Mute/Solo per traccia

### 🐛 Cosa NON Funziona (Ma Ci Stiamo Lavorando)

- ⚠️ Performance su software rendering (è lento, ok? Usiamo llvmpipe, non NVIDIA RTX)
- ⚠️ La timeline potrebbe avere qualche traccia fuori posto (stiamo ancora capendo l'algoritmo di posizionamento originale)
- ⚠️ Il formato audio RAW senza header (chi salva audio RAW nel 1998? Ah sì, Be Inc.)
- ⚠️ Loop points (funzionano, ma forse non come nell'originale)

### 🚀 Come Usare

```bash
cd bin
./demo_3dmix_viewer /path/to/your/project.3dmix
```

**Controlli:**
- **Spazio**: Play/Pause
- **S**: Stop
- **T**: Apri Timeline window
- **+/-**: Zoom nella vista 3D
- **Mouse**: Ruota la vista 3D
- **R**: Reset camera

### 📜 Requisiti Tecnici

- **OS**: Haiku OS (duh! È un formato BeOS, mica Windows)
- **RAM**: Abbastanza da caricare 12 tracce audio RAW
- **GPU**: Qualsiasi cosa che supporti OpenGL (anche software rendering va bene, sarà lento ma funziona)
- **Pazienza**: Infinita

### 🤓 Note Tecniche per Nerd

Il formato `.3dmix` è un BMessage serializzato con questa struttura:
- Coordinate 3D in formato custom float
- Audio file paths relativi (buona fortuna a trovarli)
- Sample positions in formato "22050 Hz reference" (anche se il file dice 44100 Hz)
- Loop points in sample (non in secondi, ovviamente)
- Big-endian int16 stereo samples con header custom da 96 byte

Tutto questo è stato dedotto tramite:
- Hex dump analysis (molto)
- Pattern recognition (tantissimo)
- Trial and error (TROPPO)
- Preghiere alla divinità del reverse engineering

### 📞 Bug Report

Se qualcosa non funziona (e lo farà), apri un issue su GitHub includendo:
- Il file `.3dmix` che causa problemi (se puoi condividerlo)
- Log completo dell'errore
- Una descrizione del problema
- La tua bevanda preferita (per empatia)

### 🎩 Crediti

- **Be Inc.**: Per aver creato un sistema operativo fantastico che nessuno ha comprato
- **Haiku OS**: Per aver mantenuto vivo il sogno
- **Comunità Italiana di Haiku**: Per avermi sopportato durante questo progetto folle
- **Sviluppatori di Genio**: Per aver creato un IDE potentissimo per Haiku
- **Quello che ha venduto il CD**: Grazie, ignoto eroe
- **Spritz Cinar e Tiramisu**: Letteralmente tutto

### ⚖️ Licenza

Questo è un progetto di reverse engineering educativo. Non abbiamo accesso ai sorgenti originali di Be Inc. 3D Mixer.
Il codice è stato scritto da zero basandosi sull'analisi del formato file e documentazione frammentaria trovata su Internet Archive.

**Versione**: Alpha 0.1 "CD-ROM Edition"
**Data**: Novembre 2025
**Stato**: "Funziona sul mio PC™"

---

*Fatto con amore, Spritz Cinar, Tiramisu e molto hex editing in Veneto, Italia 🇮🇹*
