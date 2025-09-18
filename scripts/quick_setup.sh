#!/bin/bash

# quick_setup.sh - Setup rapido VeniceDAW Testing Framework
# Per uso immediato su Haiku

echo "🚀 VeniceDAW Quick Setup for Phase 2 Testing"
echo "============================================"

# Verifica sistema
if [ "$(uname)" != "Haiku" ]; then
    echo "❌ Questo script deve essere eseguito su Haiku nativo"
    echo "   Esegui su sistema Haiku fisico o emulato"
    exit 1
fi

echo "✅ Sistema Haiku confermato"

# Crea directory necessarie
mkdir -p reports/{memory_analysis,performance,thread_safety}
mkdir -p temp_build

# Build del framework di testing
echo ""
echo "🔧 Build Testing Framework..."
make clean
if make test-framework-quick; then
    echo "✅ Framework build completato!"
else
    echo "❌ Build fallito - verificare dipendenze"
    echo "   Prova: pkgman install haiku_devel"
    exit 1
fi

# Setup memoria debug
echo ""
echo "🧠 Setup memoria debug..."
if [ -f "scripts/memory_debug_setup.sh" ]; then
    chmod +x scripts/memory_debug_setup.sh
    ./scripts/memory_debug_setup.sh setup
else
    echo "⚠️  Script memory debug non trovato"
fi

# Test veloce funzionalità base
echo ""
echo "🧪 Test veloce framework..."
if [ -x "./VeniceDAWTestRunner" ]; then
    timeout 30s ./VeniceDAWTestRunner --quick-validation --json-output quick_test.json
    
    if [ -f "quick_test.json" ]; then
        echo "✅ Test framework funzionante!"
        echo "📊 Risultati salvati in quick_test.json"
    else
        echo "⚠️  Test completato ma senza output JSON"
    fi
else
    echo "❌ VeniceDAWTestRunner non trovato"
fi

echo ""
echo "🎉 Setup completato!"
echo ""
echo "📋 Prossimi passi:"
echo "   ./quick_test.sh              - Test rapido 5 minuti"
echo "   make test-framework-full     - Test completo 8+ ore"
echo "   make test-evaluate-phase2    - Valutazione finale Go/No-Go"
echo ""
echo "📚 Documentazione:"
echo "   README.md                   - Guida progetto"
echo ""
echo "💡 Per iniziare: ./quick_test.sh"