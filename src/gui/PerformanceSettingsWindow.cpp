/*
 * PerformanceSettingsWindow.cpp - Performance quality settings implementation
 */

#include "PerformanceSettingsWindow.h"
#include "Mixer3DWindow.h"
#include <interface/LayoutBuilder.h>
#include <interface/Box.h>
#include <interface/SeparatorView.h>
#include <kernel/OS.h>
#include <stdio.h>

namespace HaikuDAW {

// ========== PerformanceSettings Preset Factory ==========

PerformanceSettings PerformanceSettings::CreateLowQuality()
{
    PerformanceSettings settings;
    settings.quality = PerformanceQuality::LOW;
    settings.enableWaveformCache = true;   // Cache always beneficial
    settings.waveformQualityLevel = 0;     // Blocks only, no waveform
    settings.enableParticles = false;      // Disable particles
    settings.enableGlow = false;           // Disable glow
    settings.enableShadows = false;
    settings.maxVisibleTracks = 50;        // Aggressive culling
    settings.lodDistanceClose = 10.0f;     // Earlier LOD transitions
    settings.lodDistanceFar = 18.0f;
    settings.targetFPS = 15;               // Lower frame rate target
    return settings;
}

PerformanceSettings PerformanceSettings::CreateMediumQuality()
{
    PerformanceSettings settings;
    settings.quality = PerformanceQuality::MEDIUM;
    settings.enableWaveformCache = true;
    settings.waveformQualityLevel = 1;     // Downsampled waveform
    settings.enableParticles = false;      // Still disable particles
    settings.enableGlow = false;           // Disable glow for medium
    settings.enableShadows = false;
    settings.maxVisibleTracks = 75;
    settings.lodDistanceClose = 15.0f;     // Default LOD distances
    settings.lodDistanceFar = 25.0f;
    settings.targetFPS = 30;               // Smooth 30 FPS
    return settings;
}

PerformanceSettings PerformanceSettings::CreateHighQuality()
{
    PerformanceSettings settings;
    settings.quality = PerformanceQuality::HIGH;
    settings.enableWaveformCache = true;
    settings.waveformQualityLevel = 2;     // Full quality waveform
    settings.enableParticles = true;       // Enable all effects
    settings.enableGlow = true;
    settings.enableShadows = false;        // Shadows not yet implemented
    settings.maxVisibleTracks = 100;       // No aggressive culling
    settings.lodDistanceClose = 15.0f;
    settings.lodDistanceFar = 25.0f;
    settings.targetFPS = 60;               // Maximum frame rate
    return settings;
}

// ========== PerformanceSettingsWindow Implementation ==========

PerformanceSettingsWindow::PerformanceSettingsWindow(Mixer3DView* targetView)
    : BWindow(BRect(100, 100, 550, 450), "Performance Settings",
              B_TITLED_WINDOW, B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE)
    , fTargetView(targetView)
    , fSettings(PerformanceSettings::CreateHighQuality())  // Default to high
{
    CreateUI();
    LoadCurrentSettings();
    CenterOnScreen();
}

PerformanceSettingsWindow::~PerformanceSettingsWindow()
{
}

void PerformanceSettingsWindow::CreateUI()
{
    // Create quality preset radio buttons
    fLowQualityRadio = new BRadioButton("Low (Legacy Systems)",
        new BMessage(MSG_QUALITY_LOW));
    fMediumQualityRadio = new BRadioButton("Medium (Balanced)",
        new BMessage(MSG_QUALITY_MEDIUM));
    fHighQualityRadio = new BRadioButton("High (Modern Systems)",
        new BMessage(MSG_QUALITY_HIGH));
    fCustomQualityRadio = new BRadioButton("Custom",
        new BMessage(MSG_QUALITY_CUSTOM));

    // Create feature toggles
    fWaveformCacheCheck = new BCheckBox("Enable Waveform Cache",
        new BMessage(MSG_TOGGLE_CACHE));
    fParticlesCheck = new BCheckBox("Enable Audio Particles",
        new BMessage(MSG_TOGGLE_PARTICLES));
    fGlowCheck = new BCheckBox("Enable Track Glow Effects",
        new BMessage(MSG_TOGGLE_GLOW));

    // System info display
    fSystemInfoView = new BStringView("sysinfo", "Detecting system...");
    fRecommendationView = new BStringView("recommend", "");

    // Buttons
    fDetectButton = new BButton("Auto-Detect", new BMessage(MSG_DETECT));
    fApplyButton = new BButton("Apply", new BMessage(MSG_APPLY));
    fCancelButton = new BButton("Cancel", new BMessage(MSG_CANCEL));

    // Make Apply the default button
    fApplyButton->MakeDefault(true);

    // Create layout
    BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
        .SetInsets(B_USE_WINDOW_INSETS)
        .AddGroup(B_VERTICAL, 0)
            .Add(new BStringView("title", "Rendering Quality Presets"))
            .Add(new BSeparatorView(B_HORIZONTAL))
            .AddGroup(B_VERTICAL, 0)
                .SetInsets(B_USE_DEFAULT_SPACING)
                .Add(fLowQualityRadio)
                .Add(fMediumQualityRadio)
                .Add(fHighQualityRadio)
                .Add(fCustomQualityRadio)
            .End()
        .End()
        .AddStrut(B_USE_DEFAULT_SPACING)
        .AddGroup(B_VERTICAL, 0)
            .Add(new BStringView("features_title", "Advanced Options"))
            .Add(new BSeparatorView(B_HORIZONTAL))
            .AddGroup(B_VERTICAL, 0)
                .SetInsets(B_USE_DEFAULT_SPACING)
                .Add(fWaveformCacheCheck)
                .Add(fParticlesCheck)
                .Add(fGlowCheck)
            .End()
        .End()
        .AddStrut(B_USE_DEFAULT_SPACING)
        .AddGroup(B_VERTICAL, 0)
            .Add(new BStringView("sysinfo_title", "System Information"))
            .Add(new BSeparatorView(B_HORIZONTAL))
            .AddGroup(B_VERTICAL, 0)
                .SetInsets(B_USE_DEFAULT_SPACING)
                .Add(fSystemInfoView)
                .Add(fRecommendationView)
            .End()
        .End()
        .AddGlue()
        .AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
            .Add(fDetectButton)
            .AddGlue()
            .Add(fCancelButton)
            .Add(fApplyButton)
        .End()
    .End();

    // Trigger auto-detection on startup
    DetectSystemCapabilities();
}

void PerformanceSettingsWindow::LoadCurrentSettings()
{
    // Load current settings from the 3D view (if available)
    // For now, default to HIGH quality
    fSettings = PerformanceSettings::CreateHighQuality();
    UpdateUIFromSettings();
}

void PerformanceSettingsWindow::UpdateUIFromSettings()
{
    // Update radio buttons
    switch (fSettings.quality) {
        case PerformanceQuality::LOW:
            fLowQualityRadio->SetValue(B_CONTROL_ON);
            break;
        case PerformanceQuality::MEDIUM:
            fMediumQualityRadio->SetValue(B_CONTROL_ON);
            break;
        case PerformanceQuality::HIGH:
            fHighQualityRadio->SetValue(B_CONTROL_ON);
            break;
        case PerformanceQuality::CUSTOM:
            fCustomQualityRadio->SetValue(B_CONTROL_ON);
            break;
    }

    // Update checkboxes
    fWaveformCacheCheck->SetValue(fSettings.enableWaveformCache ? B_CONTROL_ON : B_CONTROL_OFF);
    fParticlesCheck->SetValue(fSettings.enableParticles ? B_CONTROL_ON : B_CONTROL_OFF);
    fGlowCheck->SetValue(fSettings.enableGlow ? B_CONTROL_ON : B_CONTROL_OFF);

    // Enable/disable custom controls based on preset selection
    bool isCustom = (fSettings.quality == PerformanceQuality::CUSTOM);
    fWaveformCacheCheck->SetEnabled(isCustom);
    fParticlesCheck->SetEnabled(isCustom);
    fGlowCheck->SetEnabled(isCustom);
}

void PerformanceSettingsWindow::DetectSystemCapabilities()
{
    system_info info;
    get_system_info(&info);

    // Get CPU count and memory
    uint32 cpuCount = info.cpu_count;
    uint64 totalRAM = (info.max_pages * B_PAGE_SIZE) / (1024 * 1024);  // MB

    // Build system info string
    BString sysInfo;
    sysInfo.SetToFormat("CPU cores: %u, RAM: %llu MB", cpuCount, totalRAM);
    fSystemInfoView->SetText(sysInfo.String());

    // Recommend quality level based on system specs
    BString recommendation;
    PerformanceQuality recommended;

    if (totalRAM < 1024 || cpuCount == 1) {
        // < 1GB RAM or single core -> LOW
        recommendation = "Recommended: Low Quality (legacy system detected)";
        recommended = PerformanceQuality::LOW;
    } else if (totalRAM < 3072 || cpuCount <= 2) {
        // 1-3GB RAM or dual core -> MEDIUM
        recommendation = "Recommended: Medium Quality (mid-range system)";
        recommended = PerformanceQuality::MEDIUM;
    } else {
        // >= 3GB RAM and 3+ cores -> HIGH
        recommendation = "Recommended: High Quality (modern system)";
        recommended = PerformanceQuality::HIGH;
    }

    fRecommendationView->SetText(recommendation.String());

    printf("[Performance Settings] Auto-detection: %u CPUs, %llu MB RAM -> %s\n",
           cpuCount, totalRAM, recommendation.String());
}

BString PerformanceSettingsWindow::GetSystemInfo()
{
    system_info info;
    get_system_info(&info);

    BString result;
    result.SetToFormat("CPUs: %u, RAM: %llu MB",
                       info.cpu_count,
                       (info.max_pages * B_PAGE_SIZE) / (1024 * 1024));
    return result;
}

void PerformanceSettingsWindow::ApplySettings()
{
    if (!fTargetView) {
        printf("[Performance Settings] Warning: No target view to apply settings\n");
        return;
    }

    // Apply particle system setting
    fTargetView->SetParticlesEnabled(fSettings.enableParticles);

    printf("[Performance Settings] Applied %s quality preset:\n",
           fSettings.quality == PerformanceQuality::LOW ? "LOW" :
           fSettings.quality == PerformanceQuality::MEDIUM ? "MEDIUM" :
           fSettings.quality == PerformanceQuality::HIGH ? "HIGH" : "CUSTOM");
    printf("  - Waveform cache: %s\n", fSettings.enableWaveformCache ? "ON" : "OFF");
    printf("  - Waveform quality: %d\n", fSettings.waveformQualityLevel);
    printf("  - Particles: %s\n", fSettings.enableParticles ? "ON" : "OFF");
    printf("  - Glow: %s\n", fSettings.enableGlow ? "ON" : "OFF");
    printf("  - Target FPS: %d\n", fSettings.targetFPS);
    printf("  - LOD distances: %.1f / %.1f\n", fSettings.lodDistanceClose, fSettings.lodDistanceFar);

    // Force redraw
    fTargetView->Invalidate();
}

bool PerformanceSettingsWindow::QuitRequested()
{
    return true;
}

void PerformanceSettingsWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_QUALITY_LOW:
            fSettings = PerformanceSettings::CreateLowQuality();
            UpdateUIFromSettings();
            break;

        case MSG_QUALITY_MEDIUM:
            fSettings = PerformanceSettings::CreateMediumQuality();
            UpdateUIFromSettings();
            break;

        case MSG_QUALITY_HIGH:
            fSettings = PerformanceSettings::CreateHighQuality();
            UpdateUIFromSettings();
            break;

        case MSG_QUALITY_CUSTOM:
            fSettings.quality = PerformanceQuality::CUSTOM;
            fWaveformCacheCheck->SetEnabled(true);
            fParticlesCheck->SetEnabled(true);
            fGlowCheck->SetEnabled(true);
            break;

        case MSG_TOGGLE_CACHE:
            fSettings.enableWaveformCache = (fWaveformCacheCheck->Value() == B_CONTROL_ON);
            fSettings.quality = PerformanceQuality::CUSTOM;
            fCustomQualityRadio->SetValue(B_CONTROL_ON);
            break;

        case MSG_TOGGLE_PARTICLES:
            fSettings.enableParticles = (fParticlesCheck->Value() == B_CONTROL_ON);
            fSettings.quality = PerformanceQuality::CUSTOM;
            fCustomQualityRadio->SetValue(B_CONTROL_ON);
            break;

        case MSG_TOGGLE_GLOW:
            fSettings.enableGlow = (fGlowCheck->Value() == B_CONTROL_ON);
            fSettings.quality = PerformanceQuality::CUSTOM;
            fCustomQualityRadio->SetValue(B_CONTROL_ON);
            break;

        case MSG_DETECT:
            DetectSystemCapabilities();
            break;

        case MSG_APPLY:
            ApplySettings();
            Quit();
            break;

        case MSG_CANCEL:
            Quit();
            break;

        default:
            BWindow::MessageReceived(message);
            break;
    }
}

} // namespace HaikuDAW
