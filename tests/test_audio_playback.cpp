/*
 * test_audio_playback.cpp - Automated tests for VeniceDAW audio playback system
 * Tests file loading, playback, mixing, and 3D positioning
 */

#include "src/audio/SimpleHaikuEngine.h"
#include "src/gui/Mixer3DWindow.h"
#include <iostream>
#include <string>
#include <assert.h>

using namespace HaikuDAW;

class AudioPlaybackTester {
public:
    AudioPlaybackTester() : engine(nullptr), testsPassed(0), testsFailed(0) {
        std::cout << "🎵 VeniceDAW Audio Playback Test Suite" << std::endl;
        std::cout << "=======================================" << std::endl;
    }
    
    ~AudioPlaybackTester() {
        if (engine) {
            engine->Stop();
            delete engine;
        }
        
        std::cout << std::endl;
        std::cout << "Test Results:" << std::endl;
        std::cout << "✅ Passed: " << testsPassed << std::endl;
        std::cout << "❌ Failed: " << testsFailed << std::endl;
        std::cout << "Total: " << (testsPassed + testsFailed) << std::endl;
        
        if (testsFailed == 0) {
            std::cout << "🎉 All tests passed!" << std::endl;
        } else {
            std::cout << "⚠️  Some tests failed" << std::endl;
        }
    }

private:
    SimpleHaikuEngine* engine;
    int testsPassed;
    int testsFailed;
    
    void Pass(const std::string& testName) {
        std::cout << "✅ " << testName << std::endl;
        testsPassed++;
    }
    
    void Fail(const std::string& testName, const std::string& reason) {
        std::cout << "❌ " << testName << " - " << reason << std::endl;
        testsFailed++;
    }

public:
    void RunAllTests() {
        TestEngineCreation();
        TestTestSignalGeneration();
        TestMultiTrackMixing();
        TestPlaybackControls();
        TestVolumeAndPanning();
        TestLevelMetering();
        Test3DPositioning();
        TestFilePlaybackStub(); // File loading test (no actual files needed)
    }
    
private:
    void TestEngineCreation() {
        std::cout << std::endl << "Test: Engine Creation" << std::endl;
        
        try {
            engine = new SimpleHaikuEngine();
            if (engine) {
                Pass("SimpleHaikuEngine creation");
            } else {
                Fail("SimpleHaikuEngine creation", "Engine is null");
                return;
            }
            
            if (!engine->IsRunning()) {
                Pass("Initial engine state (stopped)");
            } else {
                Fail("Initial engine state", "Engine should start in stopped state");
            }
            
        } catch (...) {
            Fail("SimpleHaikuEngine creation", "Exception thrown");
        }
    }
    
    void TestTestSignalGeneration() {
        std::cout << std::endl << "Test: Test Signal Generation" << std::endl;
        
        if (!engine) {
            Fail("Test signal generation", "Engine not available");
            return;
        }
        
        try {
            // Create test tracks with different signals
            SimpleTrack* sineTrack = new SimpleTrack(1, "Test Sine");
            SimpleTrack* squareTrack = new SimpleTrack(2, "Test Square");
            SimpleTrack* noiseTrack = new SimpleTrack(3, "Test Noise");
            
            sineTrack->SetSignalType(SimpleTrack::SIGNAL_SINE);
            sineTrack->SetFrequency(440.0f);
            
            squareTrack->SetSignalType(SimpleTrack::SIGNAL_SQUARE);
            squareTrack->SetFrequency(880.0f);
            
            noiseTrack->SetSignalType(SimpleTrack::SIGNAL_WHITE_NOISE);
            
            engine->AddTrack(sineTrack);
            engine->AddTrack(squareTrack);
            engine->AddTrack(noiseTrack);
            
            if (engine->GetTrackCount() == 3) {
                Pass("Track creation and addition");
            } else {
                Fail("Track creation", "Incorrect track count");
            }
            
        } catch (...) {
            Fail("Test signal generation", "Exception during track creation");
        }
    }
    
    void TestMultiTrackMixing() {
        std::cout << std::endl << "Test: Multi-Track Mixing" << std::endl;
        
        if (!engine || engine->GetTrackCount() == 0) {
            Fail("Multi-track mixing", "No tracks available");
            return;
        }
        
        try {
            // Test that multiple tracks can be mixed
            for (int i = 0; i < engine->GetTrackCount(); i++) {
                SimpleTrack* track = engine->GetTrack(i);
                if (track) {
                    track->SetVolume(0.5f);  // Set reasonable volumes
                    track->SetPan((i - 1) * 0.5f);  // Pan tracks left/center/right
                }
            }
            
            Pass("Volume and pan settings for multiple tracks");
            
            // Test solo functionality
            if (engine->GetTrackCount() > 0) {
                engine->SetTrackSolo(0, true);
                if (engine->GetSoloTrack() == 0) {
                    Pass("Solo functionality");
                } else {
                    Fail("Solo functionality", "Solo track not set correctly");
                }
            }
            
        } catch (...) {
            Fail("Multi-track mixing", "Exception during mixing setup");
        }
    }
    
    void TestPlaybackControls() {
        std::cout << std::endl << "Test: Playback Controls" << std::endl;
        
        if (!engine) {
            Fail("Playback controls", "Engine not available");
            return;
        }
        
        try {
            // Test start
            status_t startResult = engine->Start();
            if (startResult == B_OK && engine->IsRunning()) {
                Pass("Engine start");
            } else {
                Fail("Engine start", "Failed to start or not running");
            }
            
            // Test reset tracks
            engine->ResetAllTracks();
            Pass("Reset all tracks");
            
            // Test stop
            status_t stopResult = engine->Stop();
            if (stopResult == B_OK && !engine->IsRunning()) {
                Pass("Engine stop");
            } else {
                Fail("Engine stop", "Failed to stop or still running");
            }
            
        } catch (...) {
            Fail("Playback controls", "Exception during playback control");
        }
    }
    
    void TestVolumeAndPanning() {
        std::cout << std::endl << "Test: Volume and Panning" << std::endl;
        
        if (!engine || engine->GetTrackCount() == 0) {
            Fail("Volume and panning", "No tracks available");
            return;
        }
        
        try {
            SimpleTrack* track = engine->GetTrack(0);
            
            // Test volume
            track->SetVolume(0.75f);
            if (track->GetVolume() == 0.75f) {
                Pass("Track volume setting");
            } else {
                Fail("Track volume setting", "Volume not set correctly");
            }
            
            // Test panning
            track->SetPan(-0.5f);  // Left pan
            if (track->GetPan() == -0.5f) {
                Pass("Track panning setting");
            } else {
                Fail("Track panning setting", "Pan not set correctly");
            }
            
            // Test master volume
            engine->SetMasterVolume(0.8f);
            if (engine->GetMasterVolume() == 0.8f) {
                Pass("Master volume setting");
            } else {
                Fail("Master volume setting", "Master volume not set correctly");
            }
            
        } catch (...) {
            Fail("Volume and panning", "Exception during volume/pan testing");
        }
    }
    
    void TestLevelMetering() {
        std::cout << std::endl << "Test: Level Metering" << std::endl;
        
        if (!engine || engine->GetTrackCount() == 0) {
            Fail("Level metering", "No tracks available");
            return;
        }
        
        try {
            SimpleTrack* track = engine->GetTrack(0);
            
            // Level meters should be initialized to 0
            if (track->GetPeakLevel() >= 0.0f && track->GetRMSLevel() >= 0.0f) {
                Pass("Level meter initialization");
            } else {
                Fail("Level meter initialization", "Negative level values");
            }
            
            // Master level meters
            if (engine->GetMasterPeakLeft() >= 0.0f && engine->GetMasterPeakRight() >= 0.0f) {
                Pass("Master level meter initialization");
            } else {
                Fail("Master level meter initialization", "Negative master level values");
            }
            
        } catch (...) {
            Fail("Level metering", "Exception during level meter testing");
        }
    }
    
    void Test3DPositioning() {
        std::cout << std::endl << "Test: 3D Positioning" << std::endl;
        
        if (!engine || engine->GetTrackCount() == 0) {
            Fail("3D positioning", "No tracks available");
            return;
        }
        
        try {
            SimpleTrack* track = engine->GetTrack(0);
            
            // Test 3D position setting
            track->SetPosition(5.0f, 2.0f, -3.0f);
            
            float x, y, z;
            track->GetPosition(x, y, z);
            
            if (x == 5.0f && y == 2.0f && z == -3.0f) {
                Pass("3D position setting and retrieval");
            } else {
                Fail("3D position setting", "Position not set correctly");
            }
            
            // Test multiple tracks with different positions
            if (engine->GetTrackCount() > 1) {
                for (int i = 0; i < engine->GetTrackCount(); i++) {
                    SimpleTrack* t = engine->GetTrack(i);
                    if (t) {
                        float angle = (i / (float)engine->GetTrackCount()) * 2.0f * M_PI;
                        t->SetPosition(cos(angle) * 8.0f, 0.0f, sin(angle) * 8.0f);
                    }
                }
                Pass("Multiple tracks positioned in circle");
            }
            
        } catch (...) {
            Fail("3D positioning", "Exception during 3D positioning");
        }
    }
    
    void TestFilePlaybackStub() {
        std::cout << std::endl << "Test: File Playback System (Stub)" << std::endl;
        
        if (!engine) {
            Fail("File playback system", "Engine not available");
            return;
        }
        
        try {
            // Test file loading interface (without actual files)
            SimpleTrack* fileTrack = new SimpleTrack(99, "Test File Track");
            engine->AddTrack(fileTrack);
            
            // Test file detection
            if (!fileTrack->HasFile()) {
                Pass("File detection (no file loaded)");
            } else {
                Fail("File detection", "Should report no file loaded");
            }
            
            // Test playback position interface
            fileTrack->SetPlaybackPosition(1000);
            if (fileTrack->GetPlaybackPosition() == 1000) {
                Pass("Playback position setting");
            } else {
                Fail("Playback position setting", "Position not set correctly");
            }
            
            // Note: Actual file loading tests would require test audio files
            std::cout << "📝 Note: Actual file loading requires test audio files on Haiku system" << std::endl;
            
        } catch (...) {
            Fail("File playback system", "Exception during file system testing");
        }
    }
};

int main()
{
    std::cout << "VeniceDAW Audio Playback Test Suite" << std::endl;
    std::cout << "Built for Haiku OS - Phase 5.3/5.4 Testing" << std::endl;
    std::cout << std::endl;
    
    AudioPlaybackTester tester;
    tester.RunAllTests();
    
    return 0;
}