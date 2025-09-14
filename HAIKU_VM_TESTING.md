# VeniceDAW Testing su Haiku VM
## Guida Completa per Testing 100% Nativo

### 🚀 Setup della VM Haiku

**Connessione SSH alla VM:**
```bash
# Da PowerShell Windows
ssh -p 2222 user@localhost
# Password: sapone
```

### 📁 Preparazione del Codice

**1. Trasferimento codice alla VM Haiku:**
```bash
# Da Windows, copia la directory HaikuMix nella VM
# Opzione A: Via SCP
scp -P 2222 -r HaikuMix user@localhost:/boot/home/

# Opzione B: Via shared folder QEMU (se configurato)
# Opzione C: Git clone direttamente nella VM
```

**2. Nella VM Haiku:**
```bash
cd /boot/home/HaikuMix
ls -la  # Verifica che tutti i files ci siano
```

### 🧪 Esecuzione Testing Framework

**1. Build del Framework Nativo:**
```bash
# Su Haiku nativo - questo compilerà con le vere BeAPI
make test-framework
```

**Output atteso:**
```
🧪 Building VeniceDAW Testing Framework...
✅ Building on native Haiku with real BeAPI
✅ Testing Framework built!
```

**2. Validazione Rapida (5 minuti):**
```bash
make test-framework-quick
```

**3. Setup Memory Debugging:**
```bash
make setup-memory-debug
```

**4. Validazione Completa (8+ ore):**
```bash
make test-framework-full
```

### 🎯 Test Individuali

**Test Thread Safety BeAPI:**
```bash
make test-thread-safety
```

**Test Performance Station 8-Track:**
```bash
make test-performance-scaling
```

**Test Memory Stress con malloc_debug:**
```bash
make test-memory-stress
```

**Test GUI Automation con hey:**
```bash
make test-gui-automation
```

**Valutazione Phase 2 Go/No-Go:**
```bash
make test-evaluate-phase2
```

### 📊 Verifica Risultati

**1. JSON Report:**
```bash
cat phase2_evaluation.json
```

**2. Report HTML:**
```bash
# Apri con WebPositive browser di Haiku
WebPositive phase2_evaluation.html
```

**3. Logs Memory Debug:**
```bash
ls reports/memory_analysis/
cat reports/memory_analysis/memory_debug_*.log
```

### 🔧 Risoluzione Problemi

**Se mancano headers BeAPI:**
```bash
# Verifica presenza headers
ls /boot/system/headers/be/
ls /boot/system/headers/os/

# Se mancano, installa development package
pkgman install haiku_devel
```

**Se manca libroot_debug.so:**
```bash
ls /boot/system/lib/libroot_debug.so
# Se manca:
pkgman install haiku_devel
```

**Se manca hey tool:**
```bash
which hey
# Se manca:
pkgman install hey
```

### 📈 Interpretazione Risultati

**Go/No-Go Thresholds:**

✅ **READY** - Tutti i gate superati:
- Memory: 0 leaks, <1MB/ora growth, <25% fragmentation
- Performance: >60 FPS, <70% CPU 8-track, <100ms response  
- Audio: <12ms latency, <0.001% dropout, <1ms jitter
- Reliability: >72h MTBF, <0.01% crash rate

⚠️ **CONDITIONAL** - Problemi minori (1-7 giorni fix)

❌ **NOT READY** - Problemi significativi (2+ settimane)

### 🎛️ Test Performance Station Specifici

**Validazione Scaling Lineare:**
```bash
# Testa da 1 a 8 tracce
./VeniceDAWTestRunner --performance-scaling --verbose
```

**Output atteso per sistema sano:**
```
🎛️ Testing 1 track... CPU: 8%, Memory: 6MB, FPS: 62 ✅ MET
🎛️ Testing 2 tracks... CPU: 16%, Memory: 12MB, FPS: 61 ✅ MET  
🎛️ Testing 3 tracks... CPU: 24%, Memory: 18MB, FPS: 61 ✅ MET
🎛️ Testing 4 tracks... CPU: 32%, Memory: 24MB, FPS: 60 ✅ MET
🎛️ Testing 5 tracks... CPU: 40%, Memory: 30MB, FPS: 60 ✅ MET
🎛️ Testing 6 tracks... CPU: 48%, Memory: 36MB, FPS: 60 ✅ MET
🎛️ Testing 7 tracks... CPU: 56%, Memory: 42MB, FPS: 60 ✅ MET
🎛️ Testing 8 tracks... CPU: 64%, Memory: 48MB, FPS: 60 ✅ MET

📊 Linear scaling: ✅ CPU coefficient: 8%/track, Memory: 6MB/track
🎯 Phase 2 Ready: ✅ READY (8/8 tracks sustainable)
```

### 🔒 Test Thread Safety BeAPI

**Validazione Lock-Free Communication:**
```bash
./VeniceDAWTestRunner --thread-safety --verbose
```

**Test specifici eseguiti:**
- BLooper message handling sotto carico (5000 messaggi)
- BWindow resize thread safety (100 resize operations)  
- Lock-free parameter updates GUI→Audio (10000 updates)
- B_FOLLOW_ALL behavior validation per TSoundView

### 🧠 Memory Debug con malloc_debug

**Setup automatico ambiente:**
```bash
./scripts/memory_debug_setup.sh setup
```

**Test 8 ore automatico:**
```bash
./scripts/memory_debug_setup.sh stress
```

**Analisi manuale leak:**
```bash
./scripts/memory_debug_setup.sh analyze reports/memory_analysis/stress_test_*.log
```

### 📋 Checklist Pre-Testing

Prima di eseguire i test, verifica:

**✅ Sistema Haiku:**
- [ ] Haiku R1 Beta 4+ nativo
- [ ] Headers BeAPI in /boot/system/headers/be/
- [ ] libroot_debug.so presente
- [ ] hey tool installato
- [ ] Almeno 4GB RAM libera
- [ ] CPU dual-core o superiore

**✅ Build Environment:**
- [ ] g++ supporta C++17
- [ ] OpenGL/Mesa funzionante  
- [ ] 2GB spazio disco per logs
- [ ] Permessi scrittura in directory progetto

### 🚨 Attenzioni Importanti

**⚠️ Durante test lunghi:**
- Non chiudere la finestra terminale
- Non sospendere la VM
- Assicurati alimentazione stabile
- Monitora spazio disco

**⚠️ Test 8-ore stress:**
- Programma durante notte/weekend
- Salva lavoro importante prima
- Sistema potrebbe essere rallentato
- Genera ~500MB di logs

### 🎉 Prossimi Passi dopo READY

Quando il sistema restituisce **✅ READY**:

1. **Documenta configurazione** hardware/software
2. **Salva baseline** performance per regressioni future
3. **Procedi con Phase 2** sviluppo GUI avanzate
4. **Setup CI/CD** per validazione continua
5. **Condividi risultati** con community Haiku

---

**💡 Suggerimento**: Esegui prima `make test-framework-quick` per validazione iniziale, poi `make test-framework-full` per certificazione completa Phase 2.