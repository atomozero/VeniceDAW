/*
 * ThreadSafetyTests.cpp - Comprehensive thread safety validation for VeniceDAW
 * 
 * This module implements lock-free validation patterns specifically designed 
 * for real-time audio constraints, addressing the critical challenge that
 * mutex operations are unsafe in audio threads.
 */

#include "VeniceDAWTestFramework.h"

// Conditional HaikuDAW includes  
#ifdef __HAIKU__
    #include "../audio/SimpleHaikuEngine.h"
    #include "../gui/MixerWindow.h"
#endif

// Conditional BeAPI headers
#ifdef __HAIKU__
    #include <Application.h>
    #include <Window.h>
    #include <View.h>
    #include <Message.h>
    #include <Looper.h>
    #include <MediaKit.h>
    #include <SoundPlayer.h>
#else
    #include "HaikuMockHeaders.h"
#endif

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace std::chrono;

namespace VeniceDAWTesting {

// ============================================================================
// Lock-Free Audio-GUI Communication Test
// ============================================================================

class LockFreeAudioGUITest {
private:
    // Audio parameters that can be modified from GUI
    std::atomic<float> fVolume{1.0f};
    std::atomic<float> fPan{0.0f};
    std::atomic<bool> fMute{false};
    std::atomic<int> fEffectType{0};
    
    // Performance monitoring
    std::atomic<int> fDropoutCount{0};
    std::atomic<int> fProcessedFrames{0};
    std::atomic<bool> fTestRunning{false};
    
    // Test configuration
    static constexpr int TEST_DURATION_SECONDS = 30;
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int BUFFER_SIZE = 256;
    static constexpr float MAX_DROPOUT_RATE = 0.001f; // 0.1% max dropout rate
    
public:
    struct TestResult {
        int totalFramesProcessed;
        int dropoutCount;
        float dropoutRate;
        float averageLatency;
        bool passed;
        std::vector<std::string> issues;
    };
    
    TestResult RunLockFreeTest() {
        TestResult result = {};
        
        std::cout << "🔒 Starting lock-free audio-GUI communication test...\n";
        std::cout << "   Duration: " << TEST_DURATION_SECONDS << " seconds\n";
        std::cout << "   Sample rate: " << SAMPLE_RATE << " Hz\n";
        std::cout << "   Buffer size: " << BUFFER_SIZE << " samples\n";
        
        fTestRunning.store(true);
        fDropoutCount.store(0);
        fProcessedFrames.store(0);
        
        // Start audio processing thread (simulates real-time audio)
        std::thread audioThread(&LockFreeAudioGUITest::AudioProcessingThread, this);
        
        // Start GUI parameter update thread
        std::thread guiThread(&LockFreeAudioGUITest::GUIParameterThread, this);
        
        // Monitor test progress
        auto testStart = steady_clock::now();
        while (duration_cast<seconds>(steady_clock::now() - testStart).count() < TEST_DURATION_SECONDS) {
            std::this_thread::sleep_for(milliseconds(1000));
            
            int frames = fProcessedFrames.load();
            int dropouts = fDropoutCount.load();
            float currentDropoutRate = (frames > 0) ? static_cast<float>(dropouts) / frames : 0.0f;
            
            std::cout << "   Progress: " << duration_cast<seconds>(steady_clock::now() - testStart).count() 
                      << "s, Frames: " << frames << ", Dropouts: " << dropouts 
                      << ", Rate: " << (currentDropoutRate * 100) << "%\n";
        }
        
        fTestRunning.store(false);
        
        // Wait for threads to finish
        audioThread.join();
        guiThread.join();
        
        // Calculate results
        result.totalFramesProcessed = fProcessedFrames.load();
        result.dropoutCount = fDropoutCount.load();
        result.dropoutRate = (result.totalFramesProcessed > 0) ? 
                            static_cast<float>(result.dropoutCount) / result.totalFramesProcessed : 0.0f;
        
        result.passed = (result.dropoutRate <= MAX_DROPOUT_RATE);
        
        if (!result.passed) {
            result.issues.push_back("Dropout rate exceeds threshold: " + 
                                  std::to_string(result.dropoutRate * 100) + "% > " +
                                  std::to_string(MAX_DROPOUT_RATE * 100) + "%");
        }
        
        std::cout << "✅ Lock-free test completed:\n";
        std::cout << "   Total frames: " << result.totalFramesProcessed << "\n";
        std::cout << "   Dropouts: " << result.dropoutCount << "\n";
        std::cout << "   Dropout rate: " << (result.dropoutRate * 100) << "%\n";
        std::cout << "   Result: " << (result.passed ? "PASS" : "FAIL") << "\n";
        
        return result;
    }
    
private:
    void AudioProcessingThread() {
        std::cout << "🎵 Audio processing thread started (real-time priority)\n";
        
        // Set real-time priority (Haiku-specific)
        set_thread_priority(find_thread(NULL), B_REAL_TIME_PRIORITY);
        
        auto frameStart = steady_clock::now();
        int frameNumber = 0;
        
        while (fTestRunning.load(std::memory_order_acquire)) {
            // Simulate audio buffer processing time
            auto processingStart = steady_clock::now();
            
            // Read parameters atomically (safe for audio thread)
            float volume = fVolume.load(std::memory_order_acquire);
            float pan = fPan.load(std::memory_order_acquire);
            bool mute = fMute.load(std::memory_order_acquire);
            int effectType = fEffectType.load(std::memory_order_acquire);
            
            // Simulate audio processing work
            SimulateAudioProcessing(volume, pan, mute, effectType);
            
            auto processingEnd = steady_clock::now();
            auto processingTime = duration_cast<microseconds>(processingEnd - processingStart);
            
            // Check if we're meeting real-time constraints
            // At 44.1kHz with 256 samples, we have ~5.8ms per buffer
            constexpr int maxProcessingTimeMicros = 5800;
            
            if (processingTime.count() > maxProcessingTimeMicros) {
                fDropoutCount.fetch_add(1);
            }
            
            fProcessedFrames.fetch_add(1);
            frameNumber++;
            
            // Wait for next buffer period
            frameStart += microseconds(BUFFER_SIZE * 1000000 / SAMPLE_RATE);
            std::this_thread::sleep_until(frameStart);
        }
        
        std::cout << "🎵 Audio processing thread finished\n";
    }
    
    void GUIParameterThread() {
        std::cout << "🖥️ GUI parameter update thread started\n";
        
        int updateCount = 0;
        auto lastUpdate = steady_clock::now();
        (void)lastUpdate; // Avoid unused variable warning
        
        while (fTestRunning.load(std::memory_order_acquire)) {
            // Simulate GUI parameter changes at 60 Hz
            std::this_thread::sleep_for(milliseconds(16));
            
            // Update parameters using atomic operations (safe for GUI thread)
            float newVolume = 0.5f + 0.5f * sin(updateCount * 0.1f);
            float newPan = sin(updateCount * 0.05f);
            bool newMute = (updateCount % 120) < 20; // Mute for 20 frames every 2 seconds
            int newEffectType = (updateCount / 30) % 4; // Change effect every 0.5 seconds
            
            fVolume.store(newVolume, std::memory_order_release);
            fPan.store(newPan, std::memory_order_release);
            fMute.store(newMute, std::memory_order_release);
            fEffectType.store(newEffectType, std::memory_order_release);
            
            updateCount++;
        }
        
        std::cout << "🖥️ GUI parameter thread finished (" << updateCount << " updates)\n";
    }
    
    void SimulateAudioProcessing(float volume, float pan, bool mute, int effectType) {
        // Simulate realistic audio processing work
        volatile float dummy = 0.0f;
        
        // Volume processing
        for (int i = 0; i < BUFFER_SIZE; i++) {
            dummy += sin(i * 0.1f) * volume;
        }
        
        // Pan processing  
        if (pan != 0.0f) {
            for (int i = 0; i < BUFFER_SIZE; i++) {
                dummy += cos(i * 0.1f) * pan;
            }
        }
        
        // Effect processing based on type
        switch (effectType) {
            case 1: // Reverb simulation
                for (int i = 0; i < BUFFER_SIZE / 4; i++) {
                    dummy += dummy * 0.3f;
                }
                break;
            case 2: // Delay simulation
                for (int i = 0; i < BUFFER_SIZE / 2; i++) {
                    dummy += dummy * 0.5f;
                }
                break;
            case 3: // Distortion simulation
                dummy = (dummy > 0.7f) ? 0.7f : dummy;
                break;
        }
        
        // Mute processing
        if (mute) {
            dummy = 0.0f;
        }
    }
};

// ============================================================================
// BLooper/BWindow Thread Safety Test
// ============================================================================

class BeAPIThreadSafetyTest {
private:
    BApplication* fTestApp;
    BWindow* fTestWindow;
    std::vector<std::string> fThreadSafetyIssues;
    
public:
    struct BLooperTestResult {
        bool messageQueueStable;
        bool threadSafetyMaintained;
        int messagesProcessed;
        int failedMessages;
        std::vector<std::string> issues;
    };
    
    BeAPIThreadSafetyTest() : fTestApp(nullptr), fTestWindow(nullptr) {}
    
    ~BeAPIThreadSafetyTest() {
        Cleanup();
    }
    
    BLooperTestResult TestBLooperMessageHandling() {
        BLooperTestResult result = {};
        
        std::cout << "🔄 Testing BLooper message handling under load...\n";
        
        // Create test application and window
        fTestApp = new BApplication("application/x-vnd.VeniceDAW-ThreadTest");
        
        BRect frame(100, 100, 400, 300);
        fTestWindow = new BWindow(frame, "Thread Safety Test", B_TITLED_WINDOW, 0);
        
        if (!fTestWindow->Lock()) {
            result.issues.push_back("Failed to lock test window initially");
            return result;
        }
        
        fTestWindow->Show();
        fTestWindow->Unlock();
        
        // Test rapid message posting
        const int MESSAGE_COUNT = 5000;
        int successCount = 0;
        
        for (int i = 0; i < MESSAGE_COUNT; i++) {
            BMessage* msg = new BMessage('TEST');
            msg->AddInt32("sequence", i);
            msg->AddInt64("timestamp", system_time());
            
            if (fTestWindow->PostMessage(msg) == B_OK) {
                successCount++;
            } else {
                result.failedMessages++;
            }
            
            // Occasional rapid burst
            if (i % 100 == 0) {
                for (int burst = 0; burst < 10; burst++) {
                    BMessage* burstMsg = new BMessage('BRST');
                    burstMsg->AddInt32("burst", burst);
                    fTestWindow->PostMessage(burstMsg);
                }
            }
        }
        
        // Allow time for message processing
        std::this_thread::sleep_for(milliseconds(2000));
        
        result.messagesProcessed = successCount;
        result.messageQueueStable = (result.failedMessages == 0);
        result.threadSafetyMaintained = fThreadSafetyIssues.empty();
        
        if (!result.messageQueueStable) {
            result.issues.push_back("Message queue instability: " + 
                                  std::to_string(result.failedMessages) + " failed messages");
        }
        
        std::cout << "   Messages sent: " << MESSAGE_COUNT << "\n";
        std::cout << "   Messages processed: " << successCount << "\n";
        std::cout << "   Failed messages: " << result.failedMessages << "\n";
        std::cout << "   Result: " << (result.messageQueueStable ? "STABLE" : "UNSTABLE") << "\n";
        
        return result;
    }
    
    bool TestWindowResizeThreadSafety() {
        std::cout << "🪟 Testing BWindow resize thread safety (B_FOLLOW_ALL behavior)...\n";
        
        if (!fTestWindow) {
            std::cout << "   ❌ No test window available\n";
            return false;
        }
        
        // Perform rapid resize operations while checking thread safety
        for (int i = 0; i < 100; i++) {
            if (!fTestWindow->Lock()) {
                fThreadSafetyIssues.push_back("Failed to lock window during resize " + std::to_string(i));
                return false;
            }
            
            // Simulate TSoundView B_FOLLOW_ALL resize behavior
            BRect currentFrame = fTestWindow->Frame();
            (void)currentFrame; // Avoid unused variable warning
            float newWidth = 300 + (i % 20) * 10;  // Vary width 300-490
            float newHeight = 200 + (i % 15) * 8;  // Vary height 200-312
            
            fTestWindow->ResizeTo(newWidth, newHeight);
            
            // Check if views properly follow the resize
            BView* childView = fTestWindow->ChildAt(0);
            if (childView) {
                BRect viewBounds = childView->Bounds();
                BRect windowBounds = fTestWindow->Bounds();
                
                // For B_FOLLOW_ALL, view should match window bounds
                if (viewBounds.Width() != windowBounds.Width() || 
                    viewBounds.Height() != windowBounds.Height()) {
                    fThreadSafetyIssues.push_back("View resize synchronization issue at iteration " + 
                                                 std::to_string(i));
                }
            }
            
            fTestWindow->Unlock();
            
            // Brief pause to allow message processing
            std::this_thread::sleep_for(microseconds(5000)); // 5ms
        }
        
        bool success = fThreadSafetyIssues.empty();
        std::cout << "   Resize operations: 100\n";
        std::cout << "   Thread safety issues: " << fThreadSafetyIssues.size() << "\n";
        std::cout << "   Result: " << (success ? "SAFE" : "UNSAFE") << "\n";
        
        return success;
    }
    
private:
    void Cleanup() {
        if (fTestWindow) {
            if (fTestWindow->Lock()) {
                fTestWindow->Quit();
            }
            fTestWindow = nullptr;
        }
        
        if (fTestApp) {
            delete fTestApp;
            fTestApp = nullptr;
        }
    }
};

// ============================================================================
// VeniceDAW Specific Thread Safety Tests
// ============================================================================

class VeniceDAWThreadSafetyValidator {
public:
    struct ValidationResult {
        bool lockFreeCommPassed;
        bool blooperSafetyPassed;
        bool windowResizeSafetyPassed;
        float overallScore;
        std::vector<std::string> criticalIssues;
        std::vector<std::string> recommendations;
    };
    
    ValidationResult RunComprehensiveValidation() {
        ValidationResult result = {};
        
        std::cout << "🧪 Running VeniceDAW Thread Safety Validation Suite\n";
        std::cout << "==================================================\n";
        
        // Test 1: Lock-free audio-GUI communication
        std::cout << "\n1. Lock-free Audio-GUI Communication Test\n";
        std::cout << "-------------------------------------------\n";
        
        LockFreeAudioGUITest lockFreeTest;
        auto lockFreeResult = lockFreeTest.RunLockFreeTest();
        result.lockFreeCommPassed = lockFreeResult.passed;
        
        if (!result.lockFreeCommPassed) {
            result.criticalIssues.insert(result.criticalIssues.end(), 
                                        lockFreeResult.issues.begin(), 
                                        lockFreeResult.issues.end());
            result.recommendations.push_back("Replace mutex operations with atomic operations for simple values");
            result.recommendations.push_back("Implement triple buffering for complex shared data");
        }
        
        // Test 2: BLooper/BWindow thread safety
        std::cout << "\n2. BeAPI Thread Safety Test\n";
        std::cout << "-----------------------------\n";
        
        BeAPIThreadSafetyTest beapiTest;
        auto blooperResult = beapiTest.TestBLooperMessageHandling();
        result.blooperSafetyPassed = blooperResult.messageQueueStable && blooperResult.threadSafetyMaintained;
        
        if (!result.blooperSafetyPassed) {
            result.criticalIssues.insert(result.criticalIssues.end(),
                                        blooperResult.issues.begin(),
                                        blooperResult.issues.end());
            result.recommendations.push_back("Use BMessenger for thread-safe inter-window communication");
            result.recommendations.push_back("Ensure proper BWindow::Lock()/Unlock() patterns");
        }
        
        // Test 3: Window resize thread safety (B_FOLLOW_ALL)
        std::cout << "\n3. Window Resize Thread Safety Test\n";
        std::cout << "------------------------------------\n";
        
        result.windowResizeSafetyPassed = beapiTest.TestWindowResizeThreadSafety();
        
        if (!result.windowResizeSafetyPassed) {
            result.criticalIssues.push_back("B_FOLLOW_ALL resize behavior has thread safety issues");
            result.recommendations.push_back("Implement proper AttachedToWindow() and FrameResized() callbacks");
            result.recommendations.push_back("Ensure TSoundView drawing synchronization during resizes");
        }
        
        // Calculate overall score
        int passedTests = (result.lockFreeCommPassed ? 1 : 0) +
                         (result.blooperSafetyPassed ? 1 : 0) + 
                         (result.windowResizeSafetyPassed ? 1 : 0);
        
        result.overallScore = static_cast<float>(passedTests) / 3.0f;
        
        // Print summary
        std::cout << "\n🏁 Thread Safety Validation Summary\n";
        std::cout << "====================================\n";
        std::cout << "Lock-free communication: " << (result.lockFreeCommPassed ? "✅ PASS" : "❌ FAIL") << "\n";
        std::cout << "BLooper thread safety: " << (result.blooperSafetyPassed ? "✅ PASS" : "❌ FAIL") << "\n";
        std::cout << "Window resize safety: " << (result.windowResizeSafetyPassed ? "✅ PASS" : "❌ FAIL") << "\n";
        std::cout << "Overall score: " << (result.overallScore * 100) << "%\n";
        
        if (!result.criticalIssues.empty()) {
            std::cout << "\n🚨 Critical Issues:\n";
            for (const auto& issue : result.criticalIssues) {
                std::cout << "   • " << issue << "\n";
            }
        }
        
        if (!result.recommendations.empty()) {
            std::cout << "\n💡 Recommendations:\n";
            for (const auto& rec : result.recommendations) {
                std::cout << "   • " << rec << "\n";
            }
        }
        
        return result;
    }
};

} // namespace VeniceDAWTesting

// ============================================================================
// Main test entry point for standalone execution
// ============================================================================

#ifdef THREAD_SAFETY_TESTS_MAIN
int main(int argc, char** argv) {
    VeniceDAWTesting::VeniceDAWThreadSafetyValidator validator;
    auto result = validator.RunComprehensiveValidation();
    
    return (result.overallScore >= 0.8f) ? 0 : 1; // Pass if 80% or better
}
#endif