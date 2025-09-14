#!/bin/bash

# setup_haiku_vm.sh - Setup automatico ambiente testing VeniceDAW su Haiku VM
# Da eseguire SOLO su sistema Haiku nativo

echo "🚀 VeniceDAW Haiku VM Testing Setup"
echo "=================================="

# Verifica sistema Haiku
if [ "$(uname)" != "Haiku" ]; then
    echo "❌ ERRORE: Questo script deve essere eseguito SOLO su Haiku nativo!"
    echo "   Sistema attuale: $(uname)"
    echo "   Connettiti alla VM Haiku: ssh -p 2222 user@localhost"
    exit 1
fi

echo "✅ Sistema Haiku rilevato: $(uname -a)"

# Verifica directory progetto
if [ ! -f "Makefile" ] || [ ! -d "src/testing" ]; then
    echo "❌ ERRORE: Esegui questo script dalla directory root di VeniceDAW"
    echo "   Percorso attuale: $(pwd)"
    echo "   Files necessari: Makefile, src/testing/"
    exit 1
fi

echo "✅ Directory progetto VeniceDAW confermata"

# Funzione per verificare e installare pacchetto
check_and_install() {
    local package=$1
    local command=$2
    
    if ! command -v "$command" &> /dev/null; then
        echo "⚠️  $command non trovato, installing $package..."
        pkgman install -y "$package"
        if [ $? -eq 0 ]; then
            echo "✅ $package installato con successo"
        else
            echo "❌ Errore installazione $package"
            return 1
        fi
    else
        echo "✅ $command già disponibile"
    fi
}

echo ""
echo "🔧 Verifica dipendenze sistema..."

# Verifica headers sviluppo Haiku
if [ ! -d "/boot/system/headers/be" ]; then
    echo "⚠️  Headers BeAPI non trovati, installing haiku_devel..."
    pkgman install -y haiku_devel
    if [ $? -eq 0 ]; then
        echo "✅ haiku_devel installato con successo"
    else
        echo "❌ Errore installazione haiku_devel"
        exit 1
    fi
else
    echo "✅ Headers BeAPI disponibili in /boot/system/headers/be/"
fi

# Verifica libroot_debug.so
if [ ! -f "/boot/system/lib/libroot_debug.so" ]; then
    echo "⚠️  libroot_debug.so non trovato"
    echo "   Questo è necessario per memory leak detection"
    echo "   Prova: pkgman install haiku_devel"
else
    echo "✅ libroot_debug.so disponibile per memory debugging"
fi

# Verifica hey tool
check_and_install "hey" "hey"

# Verifica gcc/g++
if ! command -v g++ &> /dev/null; then
    echo "⚠️  g++ non trovato, installing gcc..."
    pkgman install -y gcc
else
    echo "✅ g++ disponibile: $(g++ --version | head -1)"
fi

# Verifica supporto C++17
echo "🔍 Testing supporto C++17..."
cat > test_cpp17.cpp << 'EOF'
#include <iostream>
#include <memory>
int main() {
    auto ptr = std::make_unique<int>(42);
    std::cout << "C++17 works: " << *ptr << std::endl;
    return 0;
}
EOF

if g++ -std=c++17 test_cpp17.cpp -o test_cpp17 2>/dev/null; then
    ./test_cpp17
    rm -f test_cpp17.cpp test_cpp17
    echo "✅ Supporto C++17 confermato"
else
    echo "❌ Errore: g++ non supporta C++17"
    rm -f test_cpp17.cpp test_cpp17
    exit 1
fi

# Crea directories per reports
mkdir -p reports/memory_analysis
mkdir -p reports/performance
mkdir -p reports/thread_safety
echo "✅ Directory reports create"

# Test build framework
echo ""
echo "🧪 Test build framework..."
make clean
if make test-framework; then
    echo "✅ Framework compilato con successo!"
else
    echo "❌ Errore compilazione framework"
    echo ""
    echo "🔍 Possibili cause:"
    echo "   - Headers BeAPI mancanti: pkgman install haiku_devel"
    echo "   - Librerie mancanti: verificare LIBS nel Makefile"
    echo "   - Errori sintassi: verificare logs compilazione"
    exit 1
fi

# Verifica eseguibile
if [ -x "./VeniceDAWTestRunner" ]; then
    echo "✅ VeniceDAWTestRunner eseguibile creato"
    
    # Test rapido funzionalità base
    echo "🧪 Test rapido funzionalità..."
    timeout 10s ./VeniceDAWTestRunner --quick --json-output test_setup.json 2>/dev/null
    
    if [ -f "test_setup.json" ]; then
        echo "✅ Test setup completato con successo"
        rm -f test_setup.json
    else
        echo "⚠️  Test setup non riuscito (normale se mancano componenti VeniceDAW)"
    fi
else
    echo "❌ VeniceDAWTestRunner non eseguibile"
fi

# Setup memoria debug
echo ""
echo "🧠 Setup memoria debug environment..."
chmod +x scripts/memory_debug_setup.sh
./scripts/memory_debug_setup.sh setup

# Informazioni sistema
echo ""
echo "📊 Informazioni sistema Haiku:"
echo "   OS: $(uname -a)"
echo "   CPU: $(cat /boot/system/apps/ActivityMonitor 2>/dev/null || echo "Info CPU non disponibile")"
echo "   Memoria: $(free -h 2>/dev/null || echo "Info memoria non disponibile")"
echo "   Spazio disco: $(df -h . | tail -1)"

# Creazione script di convenience
cat > quick_test.sh << 'EOF'
#!/bin/bash
# Quick test VeniceDAW Phase 2 readiness
echo "🚀 VeniceDAW Quick Phase 2 Test"
echo "Durata stimata: 5 minuti"
echo "Started: $(date)"

make test-framework-quick

echo ""
echo "Results:"
if [ -f "quick_validation.json" ]; then
    echo "📄 JSON: quick_validation.json"
    grep -A 5 "phase2_readiness" quick_validation.json | head -10
fi

echo "Completed: $(date)"
EOF

chmod +x quick_test.sh

echo ""
echo "🎉 Setup completato con successo!"
echo ""
echo "📋 Comandi disponibili:"
echo "   ./quick_test.sh                    - Test veloce (5 min)"
echo "   make test-framework-quick          - Validazione rapida"
echo "   make test-framework-full           - Validazione completa (8+ ore)" 
echo "   make test-memory-stress            - Test memoria 8 ore"
echo "   make test-performance-scaling      - Test scaling Performance Station"
echo "   make test-thread-safety            - Test thread safety BeAPI"
echo "   make test-gui-automation           - Test automazione GUI"
echo "   make test-evaluate-phase2          - Valutazione Go/No-Go finale"
echo ""
echo "📚 Documentazione:"
echo "   cat HAIKU_VM_TESTING.md           - Guida completa testing"
echo "   cat docs/PHASE2_TESTING_SYSTEM.md - Documentazione sistema"
echo ""
echo "🚀 Per iniziare subito:"
echo "   ./quick_test.sh"
echo ""
echo "💡 Suggerimento: Esegui prima quick test, poi full validation per certificazione Phase 2"