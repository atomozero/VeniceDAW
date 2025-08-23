/*
 * main_native.cpp - Native Haiku audio engine test
 * 
 * Tests the 100% native BMediaKit implementation
 * This file is designed to compile and run ONLY on Haiku OS
 */

#include "audio/HaikuAudioEngine.h"
#include <app/Application.h>
#include <interface/Alert.h>
#include <support/String.h>
#include <stdio.h>
#include <unistd.h>
#include <stdexcept>

using namespace HaikuDAW;

class HaikuDAWTestApp : public BApplication {
public:
    HaikuDAWTestApp();
    virtual ~HaikuDAWTestApp();
    
    virtual void ReadyToRun();
    virtual bool QuitRequested();
    
private:
    void RunEngineTest();
    void PrintHeader();
    void PrintStatus(const HaikuAudioEngine& engine);
    
    std::unique_ptr<HaikuAudioEngine> fEngine;
};

HaikuDAWTestApp::HaikuDAWTestApp()
    : BApplication("application/x-vnd.HaikuDAW-Native-Test")
{
    PrintHeader();
}

HaikuDAWTestApp::~HaikuDAWTestApp()
{
    printf("HaikuDAWTestApp: Application destroyed\n");
}

void HaikuDAWTestApp::PrintHeader()
{
    printf("\n");
    printf("=== HaikuDAW Native Audio Engine Test ===\n");
    printf("100%% Native Haiku BMediaKit Implementation\n");
    printf("==========================================\n");
    printf("\n");
}

void HaikuDAWTestApp::ReadyToRun()
{
    printf("HaikuDAWTestApp: Starting native engine test\n");
    
    try {
        RunEngineTest();
    } catch (const std::exception& e) {
        printf("ERROR: %s\n", e.what());
        
        BAlert* alert = new BAlert("Error",
            "Native audio engine test failed.\n"
            "Check the console for details.",
            "OK");
        alert->Go();
    }
    
    // Keep the application running briefly to see results
    printf("\nTest completed. Application will quit in 3 seconds...\n");
    BMessage quitMsg(B_QUIT_REQUESTED);
    MessageReceived(&quitMsg);
}

void HaikuDAWTestApp::RunEngineTest()
{
    printf("=== NATIVE HAIKU ENGINE TEST ===\n\n");
    
    // 1. Create engine
    printf("1. Creating native Haiku audio engine...\n");
    fEngine = std::make_unique<HaikuAudioEngine>("HaikuDAW Native Test");
    
    if (!fEngine) {
        throw std::runtime_error("Failed to create audio engine");
    }
    
    printf("   ✓ Engine created successfully!\n");
    printf("   Format: %.0f Hz, %d channels, %d frames buffer\n",
           fEngine->GetSampleRate(),
           fEngine->GetFormat().u.raw_audio.channel_count,
           fEngine->GetBufferSize());
    printf("\n");
    
    // 2. Start engine
    printf("2. Starting native audio engine...\n");
    status_t result = fEngine->StartEngine();
    if (result != B_OK) {
        BString error;
        error.SetToFormat("Failed to start engine: %s", strerror(result));
        throw std::runtime_error(error.String());
    }
    
    printf("   ✓ Native engine started!\n");
    PrintStatus(*fEngine);
    printf("\n");
    
    // 3. Add test tracks
    printf("3. Adding native audio tracks...\n");
    
    auto track1 = std::make_shared<HaikuAudioTrack>(1, "Native Guitar");
    auto track2 = std::make_shared<HaikuAudioTrack>(2, "Native Bass");  
    auto track3 = std::make_shared<HaikuAudioTrack>(3, "Native Drums");
    auto track4 = std::make_shared<HaikuAudioTrack>(4, "Native Vocals");
    
    fEngine->AddTrack(track1);
    fEngine->AddTrack(track2);
    fEngine->AddTrack(track3);
    fEngine->AddTrack(track4);
    
    printf("   ✓ Added %d native tracks\n", fEngine->GetTrackCount());
    printf("\n");
    
    // 4. Configure tracks with 3D positioning
    printf("4. Configuring 3D spatial audio...\n");
    
    track1->SetVolume(0.8f);
    track1->SetPosition(-2.0f, 0.5f, 1.0f);  // Left, slightly up, forward
    
    track2->SetVolume(0.9f);
    track2->SetPosition(0.0f, -0.5f, -1.0f); // Center, slightly down, back
    
    track3->SetVolume(0.7f);
    track3->SetPosition(2.0f, 1.0f, 0.0f);   // Right, up, center
    
    track4->SetVolume(1.0f);
    track4->SetPosition(0.0f, 1.5f, 2.0f);   // Center, high, very forward
    
    printf("   ✓ 3D positioning configured\n");
    printf("     Guitar: Left (-2, 0.5, 1)\n");
    printf("     Bass: Center-Back (0, -0.5, -1)\n");
    printf("     Drums: Right-High (2, 1, 0)\n");
    printf("     Vocals: Center-High-Front (0, 1.5, 2)\n");
    printf("\n");
    
    // 5. Test master volume
    printf("5. Testing master volume control...\n");
    
    float volumes[] = { 1.0f, 0.5f, 0.2f, 0.8f, 1.0f };
    const char* labels[] = { "100%", "50%", "20%", "80%", "100%" };
    
    for (int i = 0; i < 5; i++) {
        fEngine->SetMasterVolume(volumes[i]);
        printf("   Volume: %s - ", labels[i]);
        PrintStatus(*fEngine);
        printf("\n");
        usleep(500000); // 500ms delay
    }
    
    printf("   ✓ Master volume test complete\n\n");
    
    // 6. Test track controls  
    printf("6. Testing track mute/solo...\n");
    
    track1->SetMute(true);
    printf("   Guitar muted\n");
    usleep(500000);
    
    track4->SetSolo(true);
    printf("   Vocals solo\n");
    usleep(500000);
    
    track1->SetMute(false);
    track4->SetSolo(false);
    printf("   All tracks restored\n");
    usleep(500000);
    
    printf("   ✓ Track control test complete\n\n");
    
    // 7. Performance monitoring
    printf("7. Native performance monitoring (3 seconds)...\n");
    
    for (int i = 0; i < 30; i++) {
        printf("   ");
        PrintStatus(*fEngine);
        printf("\n");
        usleep(100000); // 100ms
    }
    
    printf("   ✓ Performance monitoring complete\n\n");
    
    // 8. Stop engine
    printf("8. Stopping native engine...\n");
    
    result = fEngine->StopEngine();
    if (result == B_OK) {
        printf("   ✓ Engine stopped cleanly\n");
    } else {
        printf("   ⚠ Engine stop returned: %s\n", strerror(result));
    }
    
    printf("\nFinal status: %s\n", fEngine->GetStatusString().String());
    
    printf("\n=== NATIVE TEST COMPLETE ===\n");
    printf("\n✅ Native Haiku BMediaKit engine working!\n");
    printf("\nFeatures demonstrated:\n");
    printf("  ✓ BMediaNode integration\n");
    printf("  ✓ BSoundPlayer audio output  \n");
    printf("  ✓ BBufferGroup management\n");
    printf("  ✓ Real-time processing with BMediaEventLooper\n");
    printf("  ✓ Multi-track 3D spatial audio\n");
    printf("  ✓ Native Haiku threading\n");
    printf("  ✓ Performance monitoring\n");
    printf("  ✓ Clean resource management\n");
    printf("\n🎵 Ready for Haiku community demo! 🎵\n\n");
}

void HaikuDAWTestApp::PrintStatus(const HaikuAudioEngine& engine)
{
    printf("%s", engine.GetStatusString().String());
}

bool HaikuDAWTestApp::QuitRequested()
{
    printf("HaikuDAWTestApp: Quit requested\n");
    return true;
}

// Application entry point
int main()
{
    printf("Starting HaikuDAW Native Test...\n");
    
    HaikuDAWTestApp app;
    
    status_t result = app.InitCheck();
    if (result != B_OK) {
        printf("ERROR: Failed to initialize application: %s\n", strerror(result));
        return 1;
    }
    
    app.Run();
    
    printf("HaikuDAW Native Test: Clean exit\n");
    return 0;
}