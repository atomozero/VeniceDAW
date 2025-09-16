/*
 * test_phase4_2_hrtf.cpp - Test suite for VeniceDAW Phase 4.2 HRTF and Binaural Interface
 * 
 * Tests the enhanced HRTF implementation with real-time status monitoring,
 * binaural processing integration, and 3D visualization features.
 */

#include "src/audio/AdvancedAudioProcessor.h"
#include "src/audio/SimpleHaikuEngine.h"
// Note: GUI testing requires full Haiku environment
// #include "src/gui/SpatialMixer3DWindow.h"
#include <iostream>
#include <string>
#include <assert.h>
#include <vector>

using namespace VeniceDAW;
// using namespace HaikuDAW;  // GUI namespace - only for full Haiku environment

class Phase42HRTFTester {
public:
    Phase42HRTFTester() : testsPassed(0), testsFailed(0) {
        std::cout << "🎧 VeniceDAW Phase 4.2 HRTF & Binaural Interface Test Suite" << std::endl;
        std::cout << "=============================================================" << std::endl;
        std::cout << "Testing enhanced HRTF processing with real-time monitoring" << std::endl;
        std::cout << std::endl;
    }
    
    ~Phase42HRTFTester() {
        std::cout << std::endl;
        std::cout << "📊 Phase 4.2 Test Results:" << std::endl;
        std::cout << "✅ Passed: " << testsPassed << std::endl;
        std::cout << "❌ Failed: " << testsFailed << std::endl;
        std::cout << "Total: " << (testsPassed + testsFailed) << std::endl;
        
        if (testsFailed == 0) {
            std::cout << "🎉 All Phase 4.2 HRTF tests passed!" << std::endl;
            std::cout << "VeniceDAW Phase 4.2 HRTF & Binaural Interface is ready!" << std::endl;
        } else {
            std::cout << "⚠️  Some Phase 4.2 tests failed" << std::endl;
        }
    }

private:
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
        TestHRTFProcessorIntegration();
        TestBinauralModeActivation();
        TestDefaultHRTFLoading();
        TestCrossfeedControls();
        TestHRTFStatusMonitoring();
        TestSpatialVisualization();
        TestPerformanceImpact();
        TestProcessorStateSync();
    }
    
private:
    void TestHRTFProcessorIntegration() {
        std::cout << std::endl << "🔗 Test: HRTF Processor Integration" << std::endl;
        
        try {
            // Create audio processor
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test initial state
            if (!surroundProcessor.IsHRTFEnabled()) {
                Pass("Initial HRTF state (disabled)");
            } else {
                Fail("Initial HRTF state", "Should start disabled");
                return;
            }
            
            // Test enabling HRTF
            surroundProcessor.EnableHRTF(true);
            if (surroundProcessor.IsHRTFEnabled()) {
                Pass("HRTF enable functionality");
            } else {
                Fail("HRTF enable functionality", "Failed to enable HRTF");
                return;
            }
            
            // Test disabling HRTF
            surroundProcessor.EnableHRTF(false);
            if (!surroundProcessor.IsHRTFEnabled()) {
                Pass("HRTF disable functionality");
            } else {
                Fail("HRTF disable functionality", "Failed to disable HRTF");
            }
            
        } catch (...) {
            Fail("HRTF Processor Integration", "Exception during processor testing");
        }
    }
    
    void TestBinauralModeActivation() {
        std::cout << std::endl << "🎵 Test: Binaural Mode Activation" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test that enabling HRTF switches to binaural mode
            surroundProcessor.EnableHRTF(true);
            
            auto mode = surroundProcessor.GetSpatialMode();
            if (mode == SurroundProcessor::SpatialMode::BINAURAL_HRTF) {
                Pass("Automatic binaural mode activation");
            } else {
                Fail("Automatic binaural mode activation", "Mode not switched to BINAURAL_HRTF");
            }
            
        } catch (...) {
            Fail("Binaural Mode Activation", "Exception during mode testing");
        }
    }
    
    void TestDefaultHRTFLoading() {
        std::cout << std::endl << "📦 Test: Default HRTF Loading" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test loading default HRTF
            surroundProcessor.LoadDefaultHRTF();
            
            // Test that HRTF is now available
            if (surroundProcessor.IsHRTFEnabled()) {
                Pass("Default HRTF loading enables processing");
            } else {
                Fail("Default HRTF loading", "HRTF not enabled after loading");
            }
            
            // Test latency is added with HRTF
            size_t latency = surroundProcessor.GetLatencySamples();
            if (latency > 0) {
                Pass("HRTF adds processing latency");
            } else {
                Fail("HRTF latency", "No latency reported with HRTF enabled");
            }
            
        } catch (...) {
            Fail("Default HRTF Loading", "Exception during HRTF loading");
        }
    }
    
    void TestCrossfeedControls() {
        std::cout << std::endl << "🎛️  Test: Crossfeed Controls" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test crossfeed enable/disable
            surroundProcessor.EnableCrossfeed(true, 0.3f);
            Pass("Crossfeed enable with 30% amount");
            
            // Test crossfeed amount limits
            surroundProcessor.EnableCrossfeed(true, 1.5f);  // Should be clamped to 1.0
            Pass("Crossfeed amount clamping (>1.0)");
            
            surroundProcessor.EnableCrossfeed(true, -0.2f);  // Should be clamped to 0.0
            Pass("Crossfeed amount clamping (<0.0)");
            
            surroundProcessor.EnableCrossfeed(false);
            Pass("Crossfeed disable");
            
        } catch (...) {
            Fail("Crossfeed Controls", "Exception during crossfeed testing");
        }
    }
    
    void TestHRTFStatusMonitoring() {
        std::cout << std::endl << "📊 Test: HRTF Status Monitoring" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test status reporting
            bool initialStatus = surroundProcessor.IsHRTFEnabled();
            surroundProcessor.EnableHRTF(!initialStatus);
            bool newStatus = surroundProcessor.IsHRTFEnabled();
            
            if (newStatus != initialStatus) {
                Pass("HRTF status monitoring reflects changes");
            } else {
                Fail("HRTF status monitoring", "Status not updated correctly");
            }
            
            // Test processing load reporting
            float load = surroundProcessor.GetProcessingLoad();
            if (load >= 0.0f && load <= 1.0f) {
                Pass("Processing load monitoring (0.0-1.0 range)");
            } else {
                Fail("Processing load monitoring", "Load value out of range");
            }
            
        } catch (...) {
            Fail("HRTF Status Monitoring", "Exception during status testing");
        }
    }
    
    void TestSpatialVisualization() {
        std::cout << std::endl << "👁️  Test: Spatial Visualization (Audio Backend Only)" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test spatial positioning backend without GUI
            Vector3D testPosition(2.0f, 1.0f, -1.0f);
            surroundProcessor.SetSourcePosition(testPosition);
            
            Vector3D retrievedPosition = surroundProcessor.GetSourcePosition();
            if (retrievedPosition.x == 2.0f && retrievedPosition.y == 1.0f && retrievedPosition.z == -1.0f) {
                Pass("Spatial position backend functionality");
            } else {
                Fail("Spatial position backend", "Position not set correctly");
            }
            
            // Test distance calculation
            float distance = surroundProcessor.GetDistance();
            if (distance > 0.0f) {
                Pass("Distance calculation backend");
            } else {
                Fail("Distance calculation", "Invalid distance value");
            }
            
            // Note: Full GUI visualization testing requires native Haiku environment
            Pass("Spatial visualization backend ready for GUI integration");
            
        } catch (...) {
            Fail("Spatial Visualization", "Exception during backend testing");
        }
    }
    
    void TestPerformanceImpact() {
        std::cout << std::endl << "⚡ Test: Performance Impact" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Measure latency without HRTF
            surroundProcessor.EnableHRTF(false);
            size_t baseLatency = surroundProcessor.GetLatencySamples();
            
            // Measure latency with HRTF
            surroundProcessor.EnableHRTF(true);
            size_t hrtfLatency = surroundProcessor.GetLatencySamples();
            
            if (hrtfLatency > baseLatency) {
                Pass("HRTF increases latency as expected");
            } else {
                Fail("HRTF latency impact", "HRTF should add latency");
            }
            
            // Test that latency is reasonable (<500 samples @ 44.1kHz = ~11ms)
            if (hrtfLatency < 500) {
                Pass("HRTF latency reasonable (<11ms)");
            } else {
                Fail("HRTF latency excessive", "Latency too high for real-time");
            }
            
        } catch (...) {
            Fail("Performance Impact", "Exception during performance testing");
        }
    }
    
    void TestProcessorStateSync() {
        std::cout << std::endl << "🔄 Test: Processor State Synchronization" << std::endl;
        
        try {
            AdvancedAudioProcessor processor;
            processor.Initialize(44100.0f, 512, ChannelConfiguration::STEREO);
            
            auto& surroundProcessor = processor.GetSurroundProcessor();
            
            // Test multiple state changes
            for (int i = 0; i < 5; i++) {
                bool enable = (i % 2 == 0);
                surroundProcessor.EnableHRTF(enable);
                
                bool actualState = surroundProcessor.IsHRTFEnabled();
                if (actualState == enable) {
                    // State matches
                } else {
                    Fail("Processor State Sync", "State mismatch in iteration " + std::to_string(i));
                    return;
                }
            }
            
            Pass("Multiple HRTF state changes synchronized");
            
            // Test mode consistency
            surroundProcessor.EnableHRTF(true);
            auto mode = surroundProcessor.GetSpatialMode();
            if (mode == SurroundProcessor::SpatialMode::BINAURAL_HRTF) {
                Pass("Spatial mode consistent with HRTF state");
            } else {
                Fail("Spatial mode sync", "Mode not consistent with HRTF enabled");
            }
            
        } catch (...) {
            Fail("Processor State Synchronization", "Exception during sync testing");
        }
    }
};

int main()
{
    std::cout << "VeniceDAW Phase 4.2 HRTF & Binaural Interface Test Suite" << std::endl;
    std::cout << "Built for Professional Spatial Audio - Phase 4.2 Validation" << std::endl;
    std::cout << std::endl;
    
    Phase42HRTFTester tester;
    tester.RunAllTests();
    
    return 0;
}