/*
 * test_file_loading.cpp - Test real audio file loading with BMediaFile
 * Tests actual WAV/MP3 file loading and playback on native Haiku
 */

#include "src/audio/SimpleHaikuEngine.h"
#include <Application.h>
#include <FilePanel.h>
#include <Path.h>
#include <Entry.h>
#include <iostream>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace HaikuDAW;

class FileLoadingTester : public BApplication {
public:
    FileLoadingTester() 
        : BApplication("application/x-vnd.VeniceDAW-FileTest"),
          fEngine(nullptr) {
        std::cout << "🎵 VeniceDAW File Loading Test" << std::endl;
        std::cout << "===============================" << std::endl;
    }
    
    ~FileLoadingTester() {
        if (fEngine) {
            fEngine->Stop();
            delete fEngine;
        }
    }
    
    void ReadyToRun() override {
        // Create audio engine
        fEngine = new SimpleHaikuEngine();
        
        // Test with known system sound files first
        TestSystemSounds();
        
        // Test with user files if available
        TestUserFiles();
        
        // Start playback if we have tracks
        if (fEngine->GetTrackCount() > 0) {
            std::cout << "\nStarting playback with " << fEngine->GetTrackCount() << " tracks..." << std::endl;
            status_t status = fEngine->Start();
            if (status == B_OK) {
                std::cout << "✅ Playback started successfully!" << std::endl;
                std::cout << "Playing for 5 seconds..." << std::endl;
                snooze(5000000); // Play for 5 seconds
            } else {
                std::cout << "❌ Failed to start playback: " << strerror(status) << std::endl;
            }
        }
        
        PostMessage(B_QUIT_REQUESTED);
    }
    
private:
    void TestSystemSounds() {
        std::cout << "\n📦 Testing with system sounds..." << std::endl;
        
        // Try to load common system sounds
        const char* systemSounds[] = {
            "/boot/system/data/sounds/Beep.wav",
            "/boot/system/data/sounds/Startup.wav",
            "/boot/system/data/sounds/InfoPopper/Important.wav",
            "/boot/system/data/sounds/InfoPopper/Information.wav"
        };
        
        for (const char* path : systemSounds) {
            BEntry entry(path);
            entry_ref ref;
            
            if (entry.Exists() && entry.GetRef(&ref) == B_OK) {
                std::cout << "Loading: " << path << std::endl;
                status_t status = fEngine->LoadAudioFileAsTrack(ref);
                
                if (status == B_OK) {
                    std::cout << "  ✅ Successfully loaded!" << std::endl;
                } else {
                    std::cout << "  ❌ Failed to load: " << strerror(status) << std::endl;
                }
            } else {
                std::cout << "  ⚠️  File not found: " << path << std::endl;
            }
        }
    }
    
    void TestUserFiles() {
        std::cout << "\n📁 Testing with user files..." << std::endl;
        
        // Test with vocal samples directory
        std::cout << "🎤 Testing vocal samples..." << std::endl;
        const char* vocalSamples[] = {
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/ooh.wav",
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/ooh-ha-ha.wav",
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/ooh mid.wav",
            "/boot/home/Desktop/3D Mixes/she-loves-it/she-loves-it-samples/vocal/wahauah.wav"
        };
        
        for (const char* path : vocalSamples) {
            BEntry entry(path);
            entry_ref ref;
            
            if (entry.Exists() && entry.GetRef(&ref) == B_OK) {
                std::cout << "Loading vocal sample: " << ref.name << std::endl;
                status_t status = fEngine->LoadAudioFileAsTrack(ref);
                
                if (status == B_OK) {
                    std::cout << "  ✅ Successfully loaded!" << std::endl;
                    
                    // Get track info
                    SimpleTrack* track = fEngine->GetTrack(fEngine->GetTrackCount() - 1);
                    if (track) {
                        float duration = (float)track->GetFileDuration() / track->GetFileSampleRate();
                        std::cout << "  Duration: " << duration << " seconds" << std::endl;
                        std::cout << "  Sample Rate: " << track->GetFileSampleRate() << " Hz" << std::endl;
                        
                        // Position vocal samples in a nice circle
                        float angle = (float)(fEngine->GetTrackCount() - 1) * 90.0f * M_PI / 180.0f;
                        float radius = 3.0f;
                        track->SetPosition(sinf(angle) * radius, cosf(angle) * radius, 0.0f);
                    }
                } else {
                    std::cout << "  ❌ Failed to load: " << strerror(status) << std::endl;
                }
            } else {
                std::cout << "  ⚠️  Vocal sample not found: " << path << std::endl;
            }
        }
        
        // Also check common user directories for test files
        const char* userPaths[] = {
            "/boot/home/Desktop/test.wav",
            "/boot/home/Desktop/test.mp3",
            "/boot/home/Music/test.wav",
            "/boot/home/test.wav"
        };
        
        for (const char* path : userPaths) {
            BEntry entry(path);
            entry_ref ref;
            
            if (entry.Exists() && entry.GetRef(&ref) == B_OK) {
                std::cout << "Loading user file: " << path << std::endl;
                status_t status = fEngine->LoadAudioFileAsTrack(ref);
                
                if (status == B_OK) {
                    std::cout << "  ✅ Successfully loaded!" << std::endl;
                    
                    // Get track info
                    SimpleTrack* track = fEngine->GetTrack(fEngine->GetTrackCount() - 1);
                    if (track) {
                        float duration = (float)track->GetFileDuration() / track->GetFileSampleRate();
                        std::cout << "  Duration: " << duration << " seconds" << std::endl;
                        std::cout << "  Sample Rate: " << track->GetFileSampleRate() << " Hz" << std::endl;
                    }
                } else {
                    std::cout << "  ❌ Failed to load: " << strerror(status) << std::endl;
                }
            }
        }
    }
    
    SimpleHaikuEngine* fEngine;
};

int main()
{
    std::cout << "VeniceDAW File Loading Test" << std::endl;
    std::cout << "Testing real audio file loading on native Haiku" << std::endl;
    std::cout << std::endl;
    
    FileLoadingTester app;
    app.Run();
    
    return 0;
}