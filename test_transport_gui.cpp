/*
 * test_transport_gui.cpp - Test transport controls GUI with real audio
 * Phase 6.2 - Professional transport interface
 */

#include "src/gui/TransportWindow.h"
#include "src/audio/SimpleHaikuEngine.h"
#include <Application.h>
#include <Entry.h>
#include <Path.h>
#include <iostream>

using namespace VeniceDAW;
using namespace HaikuDAW;

class TransportTestApp : public BApplication {
public:
    TransportTestApp() 
        : BApplication("application/x-vnd.VeniceDAW-TransportTest"),
          fEngine(nullptr),
          fTransportWindow(nullptr) {
    }
    
    ~TransportTestApp() {
        if (fEngine) {
            fEngine->Stop();
            delete fEngine;
        }
    }
    
    void ReadyToRun() override {
        std::cout << "🎛️ VeniceDAW Transport GUI Test" << std::endl;
        std::cout << "=================================" << std::endl;
        
        // Create audio engine
        fEngine = new SimpleHaikuEngine();
        
        // Load vocal samples for testing
        LoadVocalSamples();
        
        // Create transport window
        fTransportWindow = new TransportWindow(fEngine);
        fTransportWindow->Show();
        
        std::cout << "✅ Transport GUI launched!" << std::endl;
        std::cout << "Use the GUI controls to:" << std::endl;
        std::cout << "  - Play/Pause audio" << std::endl;
        std::cout << "  - Stop and reset tracks" << std::endl;
        std::cout << "  - Control master volume" << std::endl;
        std::cout << "  - View waveform visualization" << std::endl;
        std::cout << "  - Monitor audio levels" << std::endl;
    }
    
private:
    void LoadVocalSamples() {
        const char* vocalSamples[] = {
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/ooh.wav",
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/ooh-ha-ha.wav", 
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/wahauah.wav"
        };
        
        int loaded = 0;
        for (const char* path : vocalSamples) {
            BEntry entry(path);
            entry_ref ref;
            
            if (entry.Exists() && entry.GetRef(&ref) == B_OK) {
                status_t status = fEngine->LoadAudioFileAsTrack(ref);
                if (status == B_OK) {
                    loaded++;
                    std::cout << "✅ Loaded: " << ref.name << std::endl;
                } else {
                    std::cout << "❌ Failed to load: " << ref.name << std::endl;
                }
            } else {
                std::cout << "⚠️  Not found: " << path << std::endl;
            }
        }
        
        std::cout << "Loaded " << loaded << " vocal samples" << std::endl;
        
        // Position tracks in 3D space
        for (int i = 0; i < fEngine->GetTrackCount(); i++) {
            SimpleTrack* track = fEngine->GetTrack(i);
            if (track) {
                float angle = i * 120.0f * M_PI / 180.0f; // 120 degrees apart
                float radius = 2.5f;
                track->SetPosition(sinf(angle) * radius, cosf(angle) * radius, 0.0f);
                track->SetVolume(0.8f);
            }
        }
    }
    
    SimpleHaikuEngine* fEngine;
    TransportWindow* fTransportWindow;
};

int main()
{
    std::cout << "VeniceDAW Phase 6.2 Transport GUI Test" << std::endl;
    std::cout << "Professional transport controls with waveform display" << std::endl;
    std::cout << std::endl;
    
    TransportTestApp app;
    app.Run();
    
    return 0;
}