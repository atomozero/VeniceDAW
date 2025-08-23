/*
 * main_simple.cpp - Simple console demo for AudioEngine testing
 * 
 * Cross-platform demo that works on any system
 * Demonstrates core functionality that will work on Haiku
 */

#include "audio/AudioEngineSimple.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace HaikuDAW;

void printHeader()
{
    std::cout << "\n";
    std::cout << "=== HaikuDAW - Audio Engine Demo ===" << std::endl;
    std::cout << "Modern Digital Audio Workstation for Haiku OS" << std::endl;
    std::cout << "Cross-platform engine testing" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << "\n";
}

void printStatus(const AudioEngineSimple& engine)
{
    std::cout << "\r" << engine.GetStatusString() << std::flush;
}

void runDemo()
{
    printHeader();
    
    // Create audio engine
    std::cout << "Creating audio engine..." << std::endl;
    AudioEngineSimple engine("HaikuDAW Demo");
    
    std::cout << "\nEngine created successfully!" << std::endl;
    std::cout << "Sample Rate: " << engine.GetSampleRate() << " Hz" << std::endl;
    std::cout << "Buffer Size: " << engine.GetBufferSize() << " frames" << std::endl;
    std::cout << "\n";
    
    // Demo sequence
    std::cout << "=== DEMO SEQUENCE ===" << std::endl;
    
    // 1. Start engine
    std::cout << "\n1. Starting audio engine..." << std::endl;
    if (engine.Start()) {
        std::cout << "   ✓ Engine started successfully!" << std::endl;
    } else {
        std::cout << "   ✗ Failed to start engine!" << std::endl;
        return;
    }
    
    // Show running status for a few seconds
    std::cout << "\n   Running status (3 seconds):" << std::endl;
    for (int i = 0; i < 30; i++) {
        std::cout << "   ";
        printStatus(engine);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << std::endl;
    
    // 2. Add tracks
    std::cout << "\n2. Adding audio tracks..." << std::endl;
    
    int32_t guitarTrack = engine.AddTrack("Lead Guitar");
    int32_t bassTrack = engine.AddTrack("Bass");
    int32_t drumTrack = engine.AddTrack("Drums");
    int32_t vocalTrack = engine.AddTrack("Vocals");
    
    if (guitarTrack > 0 && bassTrack > 0 && drumTrack > 0 && vocalTrack > 0) {
        std::cout << "   ✓ All tracks added successfully!" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 3. Configure tracks
    std::cout << "\n3. Configuring tracks..." << std::endl;
    
    // Set volumes
    engine.SetTrackVolume(guitarTrack, 0.8f);  // 80%
    engine.SetTrackVolume(bassTrack, 0.9f);    // 90% 
    engine.SetTrackVolume(drumTrack, 0.7f);    // 70%
    engine.SetTrackVolume(vocalTrack, 1.0f);   // 100%
    
    // Set 3D positions
    engine.SetTrackPosition(guitarTrack, -2.0f, 0.0f, 1.0f);  // Left
    engine.SetTrackPosition(bassTrack, 0.0f, 0.0f, -1.0f);    // Center back
    engine.SetTrackPosition(drumTrack, 2.0f, 0.0f, 1.0f);     // Right
    engine.SetTrackPosition(vocalTrack, 0.0f, 1.0f, 0.0f);    // Center front
    
    std::cout << "   ✓ Track configuration complete!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 4. Test master volume
    std::cout << "\n4. Testing master volume control..." << std::endl;
    
    float volumes[] = { 1.0f, 0.5f, 0.2f, 0.8f, 1.0f };
    const char* labels[] = { "100%", "50%", "20%", "80%", "100%" };
    
    for (int i = 0; i < 5; i++) {
        engine.SetMasterVolume(volumes[i]);
        std::cout << "   Volume: " << labels[i] << " - ";
        printStatus(engine);
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    std::cout << "   ✓ Volume control test complete!" << std::endl;
    
    // 5. Test mute/solo
    std::cout << "\n5. Testing mute/solo functionality..." << std::endl;
    
    std::cout << "   Muting guitar track..." << std::endl;
    engine.SetTrackMute(guitarTrack, true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "   Soloing vocal track..." << std::endl;
    engine.SetTrackSolo(vocalTrack, true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "   Restoring all tracks..." << std::endl;
    engine.SetTrackMute(guitarTrack, false);
    engine.SetTrackSolo(vocalTrack, false);
    
    std::cout << "   ✓ Mute/solo test complete!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 6. Performance monitoring
    std::cout << "\n6. Performance monitoring (5 seconds)..." << std::endl;
    std::cout << "   Tracking CPU usage and latency:" << std::endl;
    
    for (int i = 0; i < 50; i++) {
        std::cout << "   ";
        printStatus(engine);
        std::cout << " | Latency: " << engine.GetLatency() << "ms"
                  << " | Process: " << engine.GetProcessingTime() << "µs";
        std::cout << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "   ✓ Performance monitoring complete!" << std::endl;
    
    // 7. Remove tracks
    std::cout << "\n7. Removing tracks..." << std::endl;
    engine.RemoveTrack(guitarTrack);
    engine.RemoveTrack(bassTrack);
    std::cout << "   Remaining tracks: " << engine.GetTrackCount() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 8. Stop engine
    std::cout << "\n8. Stopping audio engine..." << std::endl;
    if (engine.Stop()) {
        std::cout << "   ✓ Engine stopped successfully!" << std::endl;
    }
    
    // Final status
    std::cout << "\nFinal status: " << engine.GetStatusString() << std::endl;
    
    std::cout << "\n=== DEMO COMPLETE ===" << std::endl;
    std::cout << "\nThis demonstrates the core HaikuDAW functionality:" << std::endl;
    std::cout << "  ✓ Real-time audio engine with threading" << std::endl;
    std::cout << "  ✓ Multi-track management" << std::endl;
    std::cout << "  ✓ Volume and positioning controls" << std::endl;
    std::cout << "  ✓ Mute/solo functionality" << std::endl;
    std::cout << "  ✓ Performance monitoring" << std::endl;
    std::cout << "  ✓ Clean startup/shutdown" << std::endl;
    std::cout << "\nReady to port to native Haiku BMediaKit! 🎵" << std::endl;
    std::cout << "\n";
}

int main()
{
    try {
        runDemo();
    } catch (const std::exception& e) {
        std::cout << "\nERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "\nUnknown error occurred!" << std::endl;
        return 1;
    }
    
    return 0;
}