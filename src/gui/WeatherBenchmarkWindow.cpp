/*
 * WeatherBenchmarkWindow.cpp - Revolutionary Weather-based Performance UI
 */

#include "WeatherBenchmarkWindow.h"
#include <Application.h>
#include <LayoutBuilder.h>
#include <GroupLayout.h>
#include <Alert.h>
#include <Path.h>
#include <FindDirectory.h>
#include <File.h>
#include <FilePanel.h>
#include <StorageKit.h>
#include <String.h>
#include <ScrollView.h>
#include <TextView.h>
#include <StringList.h>
#include <kernel/OS.h>
#include <sys/utsname.h>
#include <math.h>
#include <time.h>

namespace HaikuDAW {

// Professional Performance Meter Implementation
PerformanceMeterView::PerformanceMeterView(BRect frame)
    : BView(frame, "performance_meter", B_FOLLOW_ALL, B_WILL_DRAW),
      fCPU(0.0f), fMemory(0.0f), fAudio(0.0f), fIO(0.0f), fLatency(0.0f),
      fProgress(0.0f), fCurrentTest("Ready"),
      fCPUCores(0), fTotalRAM(0), fCPUType("Unknown"), fCPUModel("Unknown"), fHaikuVersion("Unknown"),
      fAudioDriver("Unknown"), fGraphicsDriver("Unknown")
{
    SetViewColor(AbletonColors::BACKGROUND);
    
    // Get system information
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        fCPUCores = sysInfo.cpu_count;
        fTotalRAM = int(sysInfo.max_pages * B_PAGE_SIZE / (1024 * 1024));  // MB
        fCPUType.SetTo("x86_64");  // Most common for Haiku
        
        // Format Haiku version - use a simple approach
        char versionBuffer[128];
        snprintf(versionBuffer, sizeof(versionBuffer), "Haiku R1/Beta5+");
        fHaikuVersion.SetTo(versionBuffer);
    }
    
    // Detect CPU info
    DetectCPUInfo();
    
    // Detect audio driver
    DetectAudioDriver();
    
    // Detect graphics driver  
    DetectGraphicsDriver();
}

PerformanceMeterView::~PerformanceMeterView()
{
}

void PerformanceMeterView::AttachedToWindow()
{
    BView::AttachedToWindow();
    SetViewColor(AbletonColors::BACKGROUND);
}

void PerformanceMeterView::Draw(BRect updateRect)
{
    // Clean Ableton-style background
    SetHighColor(AbletonColors::BACKGROUND);
    FillRect(Bounds());
    
    // Panel background
    BRect panelRect = Bounds();
    panelRect.InsetBy(5, 5);
    SetHighColor(AbletonColors::PANEL);
    FillRoundRect(panelRect, 4, 4);
    
    // Panel border
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(panelRect, 4, 4);
    
    // Title
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    DrawString("SYSTEM STATUS", BPoint(panelRect.left + 10, panelRect.top + fh.ascent + 10));
    
    // Performance meters - adjust spacing to fit better
    float meterTop = panelRect.top + 35;
    float meterHeight = 22;
    float meterSpacing = 26;
    
    BRect meterRect(panelRect.left + 10, meterTop, panelRect.right - 10, meterTop + meterHeight);
    
    DrawMeter(meterRect, "CPU", fCPU, (fCPU > 0.9f) ? AbletonColors::RED : 
              (fCPU > 0.8f) ? AbletonColors::YELLOW : AbletonColors::GREEN, 
              (fCPU > 0.9f) ? "CRITICAL" : (fCPU > 0.8f) ? "WARN" : "GOOD");
    
    meterRect.OffsetBy(0, meterSpacing);
    DrawMeter(meterRect, "Memory", fMemory, (fMemory > 0.9f) ? AbletonColors::RED :
              (fMemory > 0.8f) ? AbletonColors::YELLOW : AbletonColors::GREEN,
              (fMemory > 0.9f) ? "HIGH" : (fMemory > 0.8f) ? "MED" : "LOW");
    
    meterRect.OffsetBy(0, meterSpacing);
    DrawMeter(meterRect, "Audio", fAudio, (fAudio > 0.95f) ? AbletonColors::GREEN :
              (fAudio > 0.8f) ? AbletonColors::YELLOW : AbletonColors::RED,
              (fAudio > 0.95f) ? "OPTIMAL" : (fAudio > 0.8f) ? "GOOD" : "POOR");
    
    meterRect.OffsetBy(0, meterSpacing);
    DrawMeter(meterRect, "I/O", fIO, (fIO > 0.8f) ? AbletonColors::GREEN :
              (fIO > 0.5f) ? AbletonColors::YELLOW : AbletonColors::RED,
              (fIO > 0.8f) ? "FAST" : (fIO > 0.5f) ? "OK" : "SLOW");
    
    // Progress bar
    BRect progressRect(panelRect.left + 10, meterTop + 4 * meterSpacing + 5,
                      panelRect.right - 10, meterTop + 4 * meterSpacing + 20);
    DrawProgressBar(progressRect);
    
    // Overall status and latency  
    DrawStatusOverview(BRect(panelRect.left + 10, meterTop + 4 * meterSpacing + 25, 
                            panelRect.right - 10, panelRect.bottom - 10));
}

void PerformanceMeterView::DrawMeter(BRect rect, const char* label, float value, rgb_color color, const char* status)
{
    // SIMPLE FIXED LAYOUT - garantisce che tutto stia dentro il pannello
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    
    // Fixed layout with guaranteed fit
    float labelX = rect.left + 5;
    float meterStart = rect.left + 80;  // After label
    float meterEnd = rect.right - 120;  // Leave space for value + status
    float valueX = meterEnd + 10;      // After meter
    float statusX = rect.right - 60;   // Fixed from right edge
    
    // Draw label
    DrawString(label, BPoint(labelX, rect.top + fh.ascent));
    
    // Meter background
    BRect meterBg(meterStart, rect.top + 3, meterEnd, rect.bottom - 3);
    SetHighColor(AbletonColors::BORDER);
    FillRect(meterBg);
    
    // Meter fill
    BRect meterFill = meterBg;
    float fillRatio = (value > 1.0f) ? 1.0f : value;
    meterFill.right = meterFill.left + (meterFill.Width() * fillRatio);
    SetHighColor(color);
    FillRect(meterFill);
    
    // Over 100% indicator
    if (value > 1.0f) {
        SetHighColor(AbletonColors::ORANGE);
        StrokeRect(meterBg);
    }
    
    // Value text
    char valueText[16];
    sprintf(valueText, "%.0f%%", value * 100);
    SetHighColor(AbletonColors::TEXT);
    DrawString(valueText, BPoint(valueX, rect.top + fh.ascent));
    
    // Status text - TRUNCATED if too long to guarantee fit
    char shortStatus[8];
    if (strlen(status) > 6) {
        strncpy(shortStatus, status, 6);
        shortStatus[6] = '\0';
    } else {
        strcpy(shortStatus, status);
    }
    DrawString(shortStatus, BPoint(statusX, rect.top + fh.ascent));
}

void PerformanceMeterView::DrawStatusOverview(BRect bounds)
{
    float overallScore = (fCPU + fMemory + fAudio + fIO) / 4.0f;
    
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    
    const char* overallStatus;
    if (overallScore > 0.85f && fAudio > 0.9f) {
        overallStatus = "OPTIMAL FOR AUDIO RECORDING";
        SetHighColor(AbletonColors::GREEN);
    } else if (overallScore > 0.7f) {
        overallStatus = "GOOD FOR MOST AUDIO WORK";
        SetHighColor(AbletonColors::YELLOW);
    } else {
        overallStatus = "PERFORMANCE ISSUES DETECTED";
        SetHighColor(AbletonColors::RED);
    }
    
    DrawString("Overall:", BPoint(bounds.left, bounds.top + fh.ascent));
    DrawString(overallStatus, BPoint(bounds.left + 70, bounds.top + fh.ascent));
    
    // Latency (only essential info remains in System Status)
    SetHighColor(AbletonColors::TEXT);
    char latencyText[64];
    sprintf(latencyText, "Latency: %.1fms", fLatency);
    DrawString(latencyText, BPoint(bounds.left, bounds.top + fh.ascent + 20));
}

void PerformanceMeterView::SetMetrics(float cpu, float memory, float audio, float io)
{
    fCPU = cpu;
    fMemory = memory;
    fAudio = audio;
    fIO = io;
    Invalidate();
}

void PerformanceMeterView::SetLatency(float latencyMs)
{
    fLatency = latencyMs;
    Invalidate();
}

void PerformanceMeterView::SetProgress(float progress, const char* currentTest)
{
    fProgress = progress;
    if (currentTest)
        fCurrentTest.SetTo(currentTest);
    Invalidate();
}

void PerformanceMeterView::DrawProgressBar(BRect bounds)
{
    // Progress bar background
    SetHighColor(AbletonColors::BORDER);
    FillRect(bounds);
    
    // Progress fill
    if (fProgress > 0.0f) {
        BRect fillRect = bounds;
        fillRect.right = fillRect.left + (fillRect.Width() * fProgress);
        SetHighColor(AbletonColors::ORANGE);
        FillRect(fillRect);
    }
    
    // Progress text
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    
    BString progressText;
    progressText << (int)(fProgress * 100) << "% - " << fCurrentTest;
    
    float textWidth = StringWidth(progressText.String());
    float textX = bounds.left + (bounds.Width() - textWidth) / 2;
    float textY = bounds.top + (bounds.Height() + fh.ascent - fh.descent) / 2;
    
    DrawString(progressText.String(), BPoint(textX, textY));
}

void PerformanceMeterView::DetectAudioDriver()
{
    // Try to detect audio driver through various methods
    
    // Method 1: Check for common audio drivers in /dev/audio
    BDirectory devDir("/dev/audio");
    if (devDir.InitCheck() == B_OK) {
        BEntry entry;
        while (devDir.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);
            BString deviceName = path.Leaf();
            
            if (deviceName.IFindFirst("hda") >= 0) {
                fAudioDriver.SetTo("Intel HDA Audio");
                return;
            } else if (deviceName.IFindFirst("ac97") >= 0) {
                fAudioDriver.SetTo("AC97 Audio");  
                return;
            } else if (deviceName.IFindFirst("usb") >= 0) {
                fAudioDriver.SetTo("USB Audio");
                return;
            }
        }
    }
    
    // Method 2: Try BMediaRoster to get audio nodes
    // This would require BMediaKit headers, so fallback to generic
    fAudioDriver.SetTo("Haiku Audio");
}

void PerformanceMeterView::DetectGraphicsDriver()
{
    // Try to detect graphics driver
    
    // Method 1: Check OpenGL renderer string
    // This is a simplified approach - in real code we'd use OpenGL calls
    
    // Method 2: Check /dev/graphics entries
    BDirectory devDir("/dev/graphics");
    if (devDir.InitCheck() == B_OK) {
        BEntry entry;
        while (devDir.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);
            BString deviceName = path.Leaf();
            
            if (deviceName.IFindFirst("intel") >= 0) {
                fGraphicsDriver.SetTo("Intel Graphics");
                return;
            } else if (deviceName.IFindFirst("radeon") >= 0) {
                fGraphicsDriver.SetTo("AMD Radeon");
                return;
            } else if (deviceName.IFindFirst("nvidia") >= 0) {
                fGraphicsDriver.SetTo("NVIDIA");
                return;
            } else if (deviceName.IFindFirst("vesa") >= 0) {
                fGraphicsDriver.SetTo("VESA");
                return;
            }
        }
    }
    
    // Fallback
    fGraphicsDriver.SetTo("Haiku Graphics");
}

void PerformanceMeterView::DetectCPUInfo()
{
    // Use system_info for modern Haiku
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        fCPUCores = sysInfo.cpu_count;
        
        // Detect architecture using uname instead of system_info fields
        struct utsname unameData;
        if (uname(&unameData) == 0) {
            BString machine(unameData.machine);
            if (machine.IFindFirst("x86_64") != B_ERROR || machine.IFindFirst("amd64") != B_ERROR) {
                fCPUType.SetTo("x86-64");
                if (sysInfo.cpu_count > 1) {
                    fCPUModel.SetTo("Multi-core x86-64 processor");
                } else {
                    fCPUModel.SetTo("Single-core x86-64 processor");
                }
            } else if (machine.IFindFirst("x86") != B_ERROR || machine.IFindFirst("i386") != B_ERROR) {
                fCPUType.SetTo("x86");
                if (sysInfo.cpu_count > 1) {
                    fCPUModel.SetTo("Multi-core x86 processor");
                } else {
                    fCPUModel.SetTo("Single-core x86 processor");
                }
            } else {
                fCPUType.SetTo(machine.String());
                if (sysInfo.cpu_count > 1) {
                    fCPUModel.SetTo("Multi-core processor");
                } else {
                    fCPUModel.SetTo("Single-core processor");
                }
            }
        } else {
            // Fallback: Most modern Haiku systems are x86_64
            fCPUType.SetTo("x86-64");
            if (sysInfo.cpu_count > 1) {
                fCPUModel.SetTo("Multi-core processor");
            } else {
                fCPUModel.SetTo("Single-core processor");
            }
        }
        return;
    }
    
    // Fallback if system_info fails
    fCPUCores = 1;
    fCPUType.SetTo("Unknown");
    fCPUModel.SetTo("Unknown processor");
}

// PC Information Display Implementation
PCInfoView::PCInfoView(BRect frame)
    : BView(frame, "pc_info", B_FOLLOW_ALL, B_WILL_DRAW),
      fCPUCores(0), fTotalRAM(0), fLatency(0.0f)
{
    SetViewColor(AbletonColors::PANEL);
}

PCInfoView::~PCInfoView()
{
}

void PCInfoView::AttachedToWindow()
{
    BView::AttachedToWindow();
    SetViewColor(AbletonColors::PANEL);
    DetectSystemInfo();
    Invalidate();
}

void PCInfoView::DetectSystemInfo()
{
    // Get detailed system information
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        fCPUCores = sysInfo.cpu_count;
        fTotalRAM = (sysInfo.max_pages * B_PAGE_SIZE) / (1024 * 1024);  // MB
        
        // Detect architecture using uname
        struct utsname unameData;
        if (uname(&unameData) == 0) {
            BString machine(unameData.machine);
            BString release(unameData.release);
            BString sysname(unameData.sysname);
            
            if (machine.IFindFirst("x86_64") != B_ERROR) {
                fCPUType.SetTo("x86-64");
                fCPUModel.SetTo("64-bit Multi-core");
            } else if (machine.IFindFirst("x86") != B_ERROR) {
                fCPUType.SetTo("x86");  
                fCPUModel.SetTo("32-bit Multi-core");
            } else {
                fCPUType.SetTo(machine.String());
                fCPUModel.SetTo("Multi-core processor");
            }
            
            // Enhanced Haiku version from uname
            fHaikuVersion.SetTo(sysname);
            fHaikuVersion << " " << release;
        } else {
            fCPUType.SetTo("x86-64");
            fCPUModel.SetTo("Multi-core processor");
            fHaikuVersion.SetTo("Haiku R1/Beta5+");
        }
    } else {
        fCPUCores = 1;
        fTotalRAM = 512;
        fCPUType.SetTo("Unknown");
        fCPUModel.SetTo("Unknown CPU");
        fHaikuVersion.SetTo("Haiku OS");
    }
    
    // Detect detailed audio driver information
    DetectAudioDriverDetails();
    
    // Detect graphics driver information  
    DetectGraphicsDriverDetails();
    
    // Default latency - will be updated by benchmark
    fLatency = 2.9f;
}

void PCInfoView::DetectAudioDriverDetails()
{
    // Try to detect specific audio hardware
    BDirectory audioDir("/dev/audio");
    if (audioDir.InitCheck() == B_OK) {
        BEntry entry;
        BString deviceList;
        
        while (audioDir.GetNextEntry(&entry) == B_OK) {
            char name[B_FILE_NAME_LENGTH];
            if (entry.GetName(name) == B_OK) {
                if (deviceList.Length() > 0) deviceList << ", ";
                deviceList << name;
            }
        }
        
        if (deviceList.Length() > 0) {
            fAudioDriver.SetTo("Media Kit");
            fAudioDevice.SetTo(deviceList);
        } else {
            fAudioDriver.SetTo("Media Kit");
            fAudioDevice.SetTo("Generic Audio");
        }
    } else {
        fAudioDriver.SetTo("Media Kit");
        fAudioDevice.SetTo("No Audio Device");
    }
}

void PCInfoView::DetectGraphicsDriverDetails()
{
    // Try to detect graphics information from /dev/graphics
    BDirectory graphicsDir("/dev/graphics");
    if (graphicsDir.InitCheck() == B_OK) {
        BEntry entry;
        BString deviceList;
        
        while (graphicsDir.GetNextEntry(&entry) == B_OK) {
            char name[B_FILE_NAME_LENGTH];
            if (entry.GetName(name) == B_OK) {
                if (deviceList.Length() > 0) deviceList << ", ";
                deviceList << name;
            }
        }
        
        if (deviceList.Length() > 0) {
            fGraphicsDriver.SetTo("Haiku Video");
            fGraphicsCard.SetTo(deviceList);
        } else {
            fGraphicsDriver.SetTo("Haiku Video");
            fGraphicsCard.SetTo("Generic Display");
        }
    } else {
        fGraphicsDriver.SetTo("Haiku Video");
        fGraphicsCard.SetTo("Unknown Graphics");
    }
    
    // Try to detect network adapter
    BDirectory netDir("/dev/net");
    if (netDir.InitCheck() == B_OK) {
        BEntry entry;
        BString netList;
        int count = 0;
        
        while (netDir.GetNextEntry(&entry) == B_OK && count < 3) {
            char name[B_FILE_NAME_LENGTH];
            if (entry.GetName(name) == B_OK) {
                if (netList.Length() > 0) netList << ", ";
                netList << name;
                count++;
            }
        }
        
        if (netList.Length() > 0) {
            fNetworkAdapter.SetTo(netList);
        } else {
            fNetworkAdapter.SetTo("No Network");
        }
    } else {
        fNetworkAdapter.SetTo("Network N/A");
    }
    
    // Basic storage info
    fStorageInfo.SetTo("IDE/SATA Drive");
}

void PCInfoView::Draw(BRect updateRect)
{
    // Draw panel background with same style as SYSTEM STATUS
    BRect panelRect = Bounds();
    panelRect.InsetBy(5, 5);  // Same inset as other panels
    SetHighColor(AbletonColors::PANEL);
    FillRoundRect(panelRect, 4, 4);  // Same corner radius as other panels
    
    // Panel border
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(panelRect, 4, 4);
    
    // Draw content
    DrawSystemInfo(panelRect);  // Use panelRect for consistency
}

void PCInfoView::DrawSystemInfo(BRect bounds)
{
    SetHighColor(AbletonColors::TEXT);
    
    font_height fh;
    GetFontHeight(&fh);
    float lineHeight = fh.ascent + fh.descent + fh.leading + 1;
    
    // Align title with SYSTEM STATUS card (use same Y position)
    BPoint textPos(bounds.left + 10, bounds.top + fh.ascent + 10);
    
    // Title - same color as other cards
    SetHighColor(AbletonColors::TEXT);
    DrawString("SYSTEM INFORMATION", textPos);
    textPos.y += lineHeight + 5;  // Add more space after title like other cards
    
    SetHighColor(AbletonColors::TEXT);
    
    // CPU Info (compact)
    char cpuText[128];
    sprintf(cpuText, "CPU: %s (%d cores)", fCPUType.String(), fCPUCores);
    DrawString(cpuText, textPos);
    textPos.y += lineHeight;
    
    // RAM  
    char ramText[64];
    sprintf(ramText, "RAM: %d MB", fTotalRAM);
    DrawString(ramText, textPos);
    textPos.y += lineHeight;
    
    // OS (compact)
    char osText[96];
    sprintf(osText, "OS: %s", fHaikuVersion.String());
    DrawString(osText, textPos);
    textPos.y += lineHeight;
    
    // Audio with device
    char audioText[128]; 
    sprintf(audioText, "Audio: %s", fAudioDevice.String());
    DrawString(audioText, textPos);
    textPos.y += lineHeight;
    
    // Graphics with device  
    char gfxText[128];
    sprintf(gfxText, "Video: %s", fGraphicsCard.String());
    DrawString(gfxText, textPos);
    textPos.y += lineHeight;
    
    // Network
    char netText[128];
    sprintf(netText, "Network: %s", fNetworkAdapter.String());
    DrawString(netText, textPos);
    textPos.y += lineHeight;
    
    // Latency
    char latencyText[64];
    sprintf(latencyText, "Latency: %.1f ms", fLatency);
    DrawString(latencyText, textPos);
}

void PCInfoView::UpdateSystemInfo()
{
    DetectSystemInfo();
    Invalidate();
}

// Professional Benchmark Controls Implementation
BenchmarkControlsView::BenchmarkControlsView(BRect frame)
    : BView(frame, "benchmark_controls", B_FOLLOW_ALL, B_WILL_DRAW)
{
    SetViewColor(AbletonColors::BACKGROUND);
    
    // Create centered buttons with clean Ableton style
    float panelWidth = frame.Width();
    float buttonWidth = 150;
    float buttonLeft = (panelWidth - buttonWidth) / 2;  // Center horizontally
    BRect buttonRect(buttonLeft, 25, buttonLeft + buttonWidth, 50);
    
    fRunFullButton = new BButton(buttonRect, "run_full", "Run Full Test", 
                                new BMessage(MSG_RUN_WEATHER_BENCHMARK));
    AddChild(fRunFullButton);
    
    buttonRect.OffsetBy(0, 35);
    fQuickTestButton = new BButton(buttonRect, "quick_test", "Quick Test", 
                                  new BMessage(MSG_RUN_WEATHER_BENCHMARK));
    AddChild(fQuickTestButton);
    
    buttonRect.OffsetBy(0, 35);
    fDetailsButton = new BButton(buttonRect, "details", "View Details", 
                                new BMessage(MSG_SHOW_TECHNICAL));
    AddChild(fDetailsButton);
    
    buttonRect.OffsetBy(0, 35);
    fExportButton = new BButton(buttonRect, "export", "Export Report", 
                               new BMessage(MSG_EXPORT_WEATHER_REPORT));
    fExportButton->SetEnabled(false);
    AddChild(fExportButton);
    
    buttonRect.OffsetBy(0, 35);
    fDetailedReportButton = new BButton(buttonRect, "detailed_report", "Detailed Report", 
                                       new BMessage(MSG_DETAILED_REPORT));
    fDetailedReportButton->SetEnabled(false);
    AddChild(fDetailedReportButton);
}

BenchmarkControlsView::~BenchmarkControlsView()
{
}

void BenchmarkControlsView::AttachedToWindow()
{
    BView::AttachedToWindow();
    SetViewColor(AbletonColors::BACKGROUND);
    
    // Set button targets
    fRunFullButton->SetTarget(Window());
    fQuickTestButton->SetTarget(Window());
    fDetailsButton->SetTarget(Window());
    fExportButton->SetTarget(Window());
    fDetailedReportButton->SetTarget(Window());
}

void BenchmarkControlsView::Draw(BRect updateRect)
{
    // Clean background
    SetHighColor(AbletonColors::BACKGROUND);
    FillRect(Bounds());
    
    // Panel background
    BRect panelRect = Bounds();
    panelRect.InsetBy(5, 5);
    SetHighColor(AbletonColors::PANEL);
    FillRoundRect(panelRect, 4, 4);
    
    // Panel border
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(panelRect, 4, 4);
    
    // Title
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    DrawString("BENCHMARK CONTROLS", BPoint(panelRect.left + 10, panelRect.top + fh.ascent + 5));
}

void BenchmarkControlsView::SetRunning(bool running)
{
    if (running) {
        fRunFullButton->SetLabel("Running...");
        fRunFullButton->SetEnabled(false);
        fQuickTestButton->SetEnabled(false);
    } else {
        fRunFullButton->SetLabel("Run Full Test");
        fRunFullButton->SetEnabled(true);
        fQuickTestButton->SetEnabled(true);
    }
}

void BenchmarkControlsView::EnableExport(bool enabled)
{
    fExportButton->SetEnabled(enabled);
    fDetailedReportButton->SetEnabled(enabled);
}

// Professional Results Detail Implementation
ResultsDetailView::ResultsDetailView(BRect frame)
    : BView(frame, "results_detail", B_FOLLOW_ALL, B_WILL_DRAW),
      fExpanded(false)
{
    SetViewColor(AbletonColors::BACKGROUND);
}

ResultsDetailView::~ResultsDetailView()
{
}

void ResultsDetailView::Draw(BRect updateRect)
{
    // Clean background
    SetHighColor(AbletonColors::BACKGROUND);
    FillRect(Bounds());
    
    if (!fExpanded || fResults.empty()) {
        return;
    }
    
    // Panel background
    BRect panelRect = Bounds();
    panelRect.InsetBy(5, 5);
    SetHighColor(AbletonColors::PANEL);
    FillRoundRect(panelRect, 4, 4);
    
    // Panel border
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(panelRect, 4, 4);
    
    // Title
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    DrawString("PERFORMANCE DETAILS", BPoint(panelRect.left + 10, panelRect.top + fh.ascent + 10));
    
    // Results bars
    float barTop = panelRect.top + 35;
    float barHeight = 20;
    float barSpacing = 25;
    
    for (size_t i = 0; i < fResults.size() && i < 8; i++) {
        BRect barRect(panelRect.left + 10, barTop + i * barSpacing, 
                     panelRect.right - 10, barTop + i * barSpacing + barHeight);
        
        DrawResultBar(barRect, fResults[i].name, fResults[i].score, 
                     GetStatusText(fResults[i].score));
    }
}

void ResultsDetailView::DrawResultBar(BRect rect, const std::string& testName, float score, const char* status)
{
    // SIMPLE FIXED LAYOUT - garantisce che tutto stia dentro
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    GetFontHeight(&fh);
    
    // Fixed positions to guarantee fit
    float nameX = rect.left + 5;
    float barStart = rect.left + 180;   // After name
    float barEnd = rect.right - 140;    // Leave space for score + status  
    float scoreX = barEnd + 10;         // After bar
    float statusX = rect.right - 80;    // Fixed from right
    
    // Draw test name (truncate if needed)
    char shortName[25];
    if (testName.length() > 24) {
        strncpy(shortName, testName.c_str(), 21);
        strcpy(shortName + 21, "...");
    } else {
        strcpy(shortName, testName.c_str());
    }
    DrawString(shortName, BPoint(nameX, rect.top + fh.ascent + 2));
    
    // Score bar background
    BRect barBg(barStart, rect.top + 3, barEnd, rect.bottom - 3);
    SetHighColor(AbletonColors::BORDER);
    FillRect(barBg);
    
    // Score bar fill
    BRect barFill = barBg;
    float fillRatio = score / 100.0f;
    if (fillRatio > 1.0f) fillRatio = 1.0f;
    barFill.right = barFill.left + (barFill.Width() * fillRatio);
    SetHighColor(GetStatusColor(score));
    FillRect(barFill);
    
    // Score text
    char scoreText[16];
    sprintf(scoreText, "%.0f%%", score);
    SetHighColor(AbletonColors::TEXT);
    DrawString(scoreText, BPoint(scoreX, rect.top + fh.ascent + 2));
    
    // Status text - TRUNCATED to fit
    char shortStatus[8];
    if (strlen(status) > 7) {
        strncpy(shortStatus, status, 7);
        shortStatus[7] = '\0';
    } else {
        strcpy(shortStatus, status);
    }
    DrawString(shortStatus, BPoint(statusX, rect.top + fh.ascent + 2));
}

rgb_color ResultsDetailView::GetStatusColor(float score)
{
    if (score >= 80.0f) return AbletonColors::GREEN;
    if (score >= 60.0f) return AbletonColors::YELLOW;
    return AbletonColors::RED;
}

const char* ResultsDetailView::GetStatusText(float score)
{
    if (score >= 90.0f) return "EXCELLENT";
    if (score >= 80.0f) return "GOOD";
    if (score >= 60.0f) return "FAIR";
    if (score >= 40.0f) return "POOR";
    return "CRITICAL";
}

void ResultsDetailView::SetResults(const std::vector<BenchmarkResult>& results)
{
    fResults = results;
    Invalidate();
}

void ResultsDetailView::SetExpanded(bool expanded)
{
    if (fExpanded != expanded) {
        fExpanded = expanded;
        Invalidate();
    }
}

// Weather Metaphor Engine Implementation
WeatherMetaphorEngine::WeatherMetaphorEngine()
    : fOverallCondition(SUNNY)
    , fSunBrightness(1.0f)
    , fCloudCoverage(0.2f)
    , fMusicClarity(1.0f)
    , fWindSpeed(0.3f)
    , fOverallScore(100.0f)
{
    GenerateStory();
}

void WeatherMetaphorEngine::UpdateFromBenchmark(const std::vector<BenchmarkResult>& results)
{
    if (results.empty()) return;
    
    // Calculate overall performance
    float totalScore = 0.0f;
    float audioScore = 0.0f;
    float memoryScore = 0.0f;
    float cpuScore = 0.0f;
    int audioCount = 0, memoryCount = 0, cpuCount = 0;
    
    for (const auto& result : results) {
        totalScore += result.score;
        
        if (result.category.find("Audio") != std::string::npos) {
            audioScore += result.score;
            audioCount++;
        } else if (result.category.find("Memory") != std::string::npos) {
            memoryScore += result.score;
            memoryCount++;
        } else if (result.category.find("CPU") != std::string::npos || 
                   result.category.find("System") != std::string::npos) {
            cpuScore += result.score;
            cpuCount++;
        }
    }
    
    fOverallScore = totalScore / results.size();
    
    // Map performance to weather metaphors
    fSunBrightness = (cpuCount > 0) ? (cpuScore / cpuCount) / 100.0f : 1.0f;
    fMusicClarity = (audioCount > 0) ? (audioScore / audioCount) / 100.0f : 1.0f;
    fCloudCoverage = (memoryCount > 0) ? (1.0f - (memoryScore / memoryCount) / 100.0f) : 0.2f;
    fWindSpeed = fOverallScore / 100.0f;
    
    // Clamp values
    fSunBrightness = std::max(0.1f, std::min(1.0f, fSunBrightness));
    fMusicClarity = std::max(0.1f, std::min(1.0f, fMusicClarity));
    fCloudCoverage = std::max(0.0f, std::min(0.9f, fCloudCoverage));
    fWindSpeed = std::max(0.1f, std::min(1.0f, fWindSpeed));
    
    CalculateWeatherCondition();
    GenerateStory();
}

void WeatherMetaphorEngine::CalculateWeatherCondition()
{
    if (fOverallScore >= 90.0f) {
        fOverallCondition = SUNNY;
    } else if (fOverallScore >= 70.0f) {
        fOverallCondition = PARTLY_CLOUDY;
    } else if (fOverallScore >= 50.0f) {
        fOverallCondition = CLOUDY;
    } else if (fOverallScore >= 30.0f) {
        fOverallCondition = OVERCAST;
    } else if (fOverallScore >= 10.0f) {
        fOverallCondition = RAINY;
    } else {
        fOverallCondition = STORMY;
    }
}

std::string WeatherMetaphorEngine::GetWeatherStory() const
{
    return fWeatherStory;
}

std::string WeatherMetaphorEngine::GetQuickForecast() const
{
    return fQuickForecast;
}

void WeatherMetaphorEngine::GenerateStory()
{
    // Generate human-readable weather story
    std::string condition_name;
    std::string condition_emoji;
    
    switch (fOverallCondition) {
        case SUNNY:
            condition_name = "sunny and bright";
            condition_emoji = "☀️";
            break;
        case PARTLY_CLOUDY:
            condition_name = "partly cloudy but pleasant";
            condition_emoji = "⛅";
            break;
        case CLOUDY:
            condition_name = "cloudy but stable";
            condition_emoji = "☁️";
            break;
        case OVERCAST:
            condition_name = "overcast and sluggish";
            condition_emoji = "🌫️";
            break;
        case RAINY:
            condition_name = "rainy and struggling";
            condition_emoji = "🌧️";
            break;
        case STORMY:
            condition_name = "stormy and chaotic";
            condition_emoji = "⛈️";
            break;
    }
    
    // Create engaging narrative
    fWeatherStory = "Your system ecosystem is " + condition_emoji + " " + condition_name + " today!\n\n";
    
    // Sun (CPU) description
    if (fSunBrightness >= 0.8f) {
        fWeatherStory += "☀️ The sun shines brilliantly overhead - your processor is running smoothly and efficiently, ";
        fWeatherStory += "providing plenty of computational power for demanding tasks.\n\n";
    } else if (fSunBrightness >= 0.6f) {
        fWeatherStory += "🌤️ The sun provides steady warmth - your processor is working well, ";
        fWeatherStory += "handling most tasks without strain.\n\n";
    } else {
        fWeatherStory += "🌫️ The sun struggles through the clouds - your processor is working hard, ";
        fWeatherStory += "consider closing unnecessary applications.\n\n";
    }
    
    // Clouds (Memory) description  
    if (fCloudCoverage <= 0.3f) {
        fWeatherStory += "💨 Just a few wispy clouds drift by - plenty of memory available, ";
        fWeatherStory += "your system has room to breathe.\n\n";
    } else if (fCloudCoverage <= 0.6f) {
        fWeatherStory += "☁️ Some clouds gather overhead - memory usage is moderate, ";
        fWeatherStory += "still comfortable for most activities.\n\n";
    } else {
        fWeatherStory += "🌫️ Heavy clouds block the sky - memory is getting tight, ";
        fWeatherStory += "consider freeing up some space.\n\n";
    }
    
    // Music (Audio) description
    if (fMusicClarity >= 0.9f) {
        fWeatherStory += "🎵 Crystal-clear melodies fill the air - audio performance is exceptional, ";
        fWeatherStory += "perfect for professional recording and mixing.\n\n";
    } else if (fMusicClarity >= 0.7f) {
        fWeatherStory += "🎶 Pleasant music drifts through the ecosystem - audio quality is good, ";
        fWeatherStory += "suitable for most creative work.\n\n";
    } else {
        fWeatherStory += "🎵 The music sounds a bit fuzzy - audio system is stressed, ";
        fWeatherStory += "consider increasing buffer sizes or reducing tracks.\n\n";
    }
    
    // Generate forecast
    if (fOverallScore >= 80.0f) {
        fQuickForecast = "Excellent conditions ahead! Perfect weather for intensive creative work.";
    } else if (fOverallScore >= 60.0f) {
        fQuickForecast = "Good conditions continue. Ideal for most audio projects.";
    } else if (fOverallScore >= 40.0f) {
        fQuickForecast = "Mixed conditions expected. Some tasks may face headwinds.";
    } else {
        fQuickForecast = "Rough weather ahead. Consider system optimization before heavy work.";
    }
}

// Ecosystem View Implementation
EcosystemView::EcosystemView(BRect frame)
    : BView(frame, "ecosystem", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
    , fWeatherEngine(nullptr)
    , fDetailLevel(0)
    , fAnimating(false)
    , fAnimationStart(0)
    , fGlobalAnimationPhase(0.0f)
    , fSunPosition(frame.Width() * 0.75f, frame.Height() * 0.25f)
    , fOffscreenBitmap(nullptr)
    , fOffscreenView(nullptr)
{
    SetViewColor(B_TRANSPARENT_COLOR);
    
    // Initialize fonts
    fTitleFont = *be_bold_font;
    fTitleFont.SetSize(18);
    fWeatherFont = *be_plain_font;
    fWeatherFont.SetSize(12);
    
    // Initialize ecosystem elements
    fClouds.resize(5);
    fMusicNotes.resize(8);
    fRaindrops.resize(20);
    
    // Position clouds randomly
    for (size_t i = 0; i < fClouds.size(); i++) {
        fClouds[i].position = BPoint(
            (rand() % (int)frame.Width()),
            (rand() % (int)(frame.Height() * 0.4f)) + 50
        );
        fClouds[i].animation_phase = (rand() % 100) / 100.0f;
        fClouds[i].color = (rgb_color){240, 240, 240, 200};
        fClouds[i].visible = true;
    }
    
    // Position music notes
    for (size_t i = 0; i < fMusicNotes.size(); i++) {
        fMusicNotes[i].position = BPoint(
            (rand() % (int)frame.Width()),
            frame.Height() * 0.6f + (rand() % (int)(frame.Height() * 0.3f))
        );
        fMusicNotes[i].animation_phase = (rand() % 100) / 100.0f;
        fMusicNotes[i].color = (rgb_color){100, 200, 255, 180};
        fMusicNotes[i].visible = true;
    }
}

EcosystemView::~EcosystemView()
{
    delete fOffscreenBitmap;
}

void EcosystemView::AttachedToWindow()
{
    BView::AttachedToWindow();
    
    // Create offscreen bitmap
    BRect bounds = Bounds();
    fOffscreenBitmap = new BBitmap(bounds, B_RGB32, true);
    if (fOffscreenBitmap && fOffscreenBitmap->IsValid()) {
        fOffscreenView = new BView(bounds, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
        fOffscreenBitmap->AddChild(fOffscreenView);
    }
    
    // Enable pulse for animations
    Window()->SetPulseRate(50000); // 20 FPS for smooth weather
    StartAnimation();
}

void EcosystemView::SetWeatherEngine(WeatherMetaphorEngine* engine)
{
    fWeatherEngine = engine;
    Invalidate();
}

void EcosystemView::SetDetailLevel(int level)
{
    fDetailLevel = level;
    Invalidate();
}

void EcosystemView::StartAnimation()
{
    fAnimating = true;
    fAnimationStart = system_time();
}

void EcosystemView::StopAnimation()
{
    fAnimating = false;
}

void EcosystemView::Pulse()
{
    if (fAnimating) {
        UpdateAnimations();
        Invalidate();
    }
}

void EcosystemView::UpdateAnimations()
{
    bigtime_t now = system_time();
    fGlobalAnimationPhase = ((now - fAnimationStart) / 1000000.0f);
    
    // Update cloud positions (slow drift)
    for (auto& cloud : fClouds) {
        cloud.animation_phase += 0.005f;
        if (cloud.animation_phase > 1.0f) cloud.animation_phase = 0.0f;
        
        // Gentle horizontal drift
        cloud.position.x += sinf(fGlobalAnimationPhase * 0.1f) * 0.2f;
        if (cloud.position.x > Bounds().Width() + 50) {
            cloud.position.x = -50;
        }
    }
    
    // Update music note animations
    for (auto& note : fMusicNotes) {
        note.animation_phase += 0.02f;
        if (note.animation_phase > 1.0f) {
            note.animation_phase = 0.0f;
            // Reset position
            note.position.y = Bounds().Height() * 0.9f;
        }
        
        // Float upward
        note.position.y -= 0.5f;
        note.position.x += sinf(note.animation_phase * 6.28f) * 0.3f;
    }
}

void EcosystemView::Draw(BRect updateRect)
{
    if (!fOffscreenBitmap) return;
    
    fOffscreenBitmap->Lock();
    
    BRect bounds = Bounds();
    
    // Clear background
    fOffscreenView->SetHighColor(135, 206, 250); // Sky blue
    fOffscreenView->FillRect(bounds);
    
    // Draw ecosystem layers (back to front)
    DrawSky(bounds);
    DrawMountains(bounds);
    DrawCity(bounds);
    DrawRiver(bounds);
    DrawClouds(bounds);
    DrawSun(bounds);
    DrawMusicNotes(bounds);
    DrawWeatherEffects(bounds);
    
    // Draw UI overlays based on detail level
    DrawDetailOverlays(bounds);
    
    fOffscreenView->Sync();
    fOffscreenBitmap->Unlock();
    
    // Copy to screen
    DrawBitmap(fOffscreenBitmap, BPoint(0, 0));
}

void EcosystemView::DrawSky(BRect bounds)
{
    // Gradient sky based on weather condition
    rgb_color skyTop = {135, 206, 250, 255};    // Default light blue
    rgb_color skyBottom = {176, 224, 230, 255}; // Default powder blue
    
    if (fWeatherEngine) {
        WeatherCondition condition = fWeatherEngine->GetOverallWeather();
        switch (condition) {
            case SUNNY:
                skyTop = (rgb_color){100, 149, 237, 255};    // Cornflower blue
                skyBottom = (rgb_color){176, 224, 230, 255}; // Powder blue
                break;
            case PARTLY_CLOUDY:
                skyTop = (rgb_color){119, 136, 153, 255};    // Light slate gray
                skyBottom = (rgb_color){176, 196, 222, 255}; // Light steel blue
                break;
            case CLOUDY:
                skyTop = (rgb_color){105, 105, 105, 255};    // Dim gray
                skyBottom = (rgb_color){169, 169, 169, 255}; // Dark gray
                break;
            case OVERCAST:
                skyTop = (rgb_color){96, 96, 96, 255};       // Gray
                skyBottom = (rgb_color){128, 128, 128, 255}; // Gray
                break;
            case RAINY:
                skyTop = (rgb_color){70, 70, 70, 255};       // Dark gray
                skyBottom = (rgb_color){105, 105, 105, 255}; // Dim gray
                break;
            case STORMY:
                skyTop = (rgb_color){47, 47, 79, 255};       // Dark slate gray
                skyBottom = (rgb_color){85, 85, 85, 255};    // Very dark gray
                break;
            default:
                skyTop = (rgb_color){135, 206, 250, 255};    // Light blue (default)
                skyBottom = (rgb_color){176, 224, 230, 255}; // Powder blue (default)
                break;
        }
    }
    
    // Draw gradient
    for (int y = 0; y < bounds.Height() * 0.7f; y++) {
        float t = y / (bounds.Height() * 0.7f);
        rgb_color currentColor = {
            (uint8)(skyTop.red + (skyBottom.red - skyTop.red) * t),
            (uint8)(skyTop.green + (skyBottom.green - skyTop.green) * t),
            (uint8)(skyTop.blue + (skyBottom.blue - skyTop.blue) * t),
            255
        };
        
        fOffscreenView->SetHighColor(currentColor);
        fOffscreenView->StrokeLine(
            BPoint(0, y),
            BPoint(bounds.Width(), y)
        );
    }
}

void EcosystemView::DrawSun(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    float brightness = fWeatherEngine->GetSunBrightness();
    float cloudCoverage = fWeatherEngine->GetCloudCoverage();
    
    // Sun visibility based on cloud coverage
    if (cloudCoverage > 0.8f) return; // Hidden by clouds
    
    BPoint center = fSunPosition;
    float radius = 30 + sinf(fGlobalAnimationPhase * 0.5f) * 3; // Gentle pulsing
    
    // Sun glow effect
    for (int i = 5; i >= 0; i--) {
        float glowRadius = radius + i * 8;
        uint8 alpha = (uint8)((brightness * 50) / (i + 1));
        
        rgb_color glowColor = {255, 255, 150, alpha};
        fOffscreenView->SetHighColor(glowColor);
        fOffscreenView->SetDrawingMode(B_OP_ALPHA);
        fOffscreenView->FillEllipse(center, glowRadius, glowRadius);
    }
    
    // Main sun body
    rgb_color sunColor = {
        255,
        (uint8)(255 * brightness),
        (uint8)(100 + 155 * brightness),
        255
    };
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
    fOffscreenView->SetHighColor(sunColor);
    fOffscreenView->FillEllipse(center, radius, radius);
    
    // Sun rays (if bright enough)
    if (brightness > 0.7f) {
        fOffscreenView->SetPenSize(2);
        for (int i = 0; i < 8; i++) {
            float angle = (i * 45 + fGlobalAnimationPhase * 10) * M_PI / 180.0f;
            float rayLength = radius + 15 + sinf(fGlobalAnimationPhase + i) * 5;
            
            BPoint rayEnd = center + BPoint(
                cosf(angle) * rayLength,
                sinf(angle) * rayLength
            );
            
            fOffscreenView->StrokeLine(
                center + BPoint(cosf(angle) * radius, sinf(angle) * radius),
                rayEnd
            );
        }
        fOffscreenView->SetPenSize(1);
    }
}

// Continue with more drawing methods...
void EcosystemView::DrawClouds(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    float cloudCoverage = fWeatherEngine->GetCloudCoverage();
    
    for (size_t i = 0; i < fClouds.size() && i < (size_t)(cloudCoverage * 8); i++) {
        const auto& cloud = fClouds[i];
        if (!cloud.visible) continue;
        
        BPoint pos = cloud.position;
        pos.y += sinf(fGlobalAnimationPhase * 0.2f + i) * 3; // Gentle vertical float
        
        // Cloud opacity based on coverage
        uint8 alpha = (uint8)(180 * cloudCoverage);
        rgb_color cloudColor = {240, 240, 240, alpha};
        
        fOffscreenView->SetDrawingMode(B_OP_ALPHA);
        fOffscreenView->SetHighColor(cloudColor);
        
        // Draw cloud as overlapping ellipses
        float baseSize = 30 + sinf(cloud.animation_phase * 2) * 5;
        fOffscreenView->FillEllipse(pos, baseSize, baseSize * 0.7f);
        fOffscreenView->FillEllipse(pos + BPoint(-15, -5), baseSize * 0.8f, baseSize * 0.6f);
        fOffscreenView->FillEllipse(pos + BPoint(15, -3), baseSize * 0.9f, baseSize * 0.5f);
        fOffscreenView->FillEllipse(pos + BPoint(0, 8), baseSize * 0.7f, baseSize * 0.4f);
    }
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawMountains(BRect bounds)
{
    // Draw mountains in background (static landscape)
    rgb_color mountainColor = {85, 107, 47, 255}; // Dark olive green
    fOffscreenView->SetHighColor(mountainColor);
    
    // Simple mountain silhouette
    BPoint mountains[] = {
        BPoint(0, bounds.Height() * 0.7f),
        BPoint(bounds.Width() * 0.2f, bounds.Height() * 0.5f),
        BPoint(bounds.Width() * 0.4f, bounds.Height() * 0.6f),
        BPoint(bounds.Width() * 0.6f, bounds.Height() * 0.4f),
        BPoint(bounds.Width() * 0.8f, bounds.Height() * 0.55f),
        BPoint(bounds.Width(), bounds.Height() * 0.65f),
        BPoint(bounds.Width(), bounds.Height()),
        BPoint(0, bounds.Height())
    };
    
    fOffscreenView->FillPolygon(mountains, 8);
}

void EcosystemView::DrawCity(BRect bounds)
{
    // Draw simple city skyline representing system activity
    if (!fWeatherEngine) return;
    
    float cpuActivity = fWeatherEngine->GetSunBrightness();
    
    // City buildings with varying heights based on CPU activity
    rgb_color buildingColor = {64, 64, 64, 255};
    rgb_color windowColor = {255, 255, 200, 180};
    
    fOffscreenView->SetHighColor(buildingColor);
    
    float baseY = bounds.Height() * 0.8f;
    float buildingWidth = bounds.Width() / 12.0f;
    
    for (int i = 0; i < 10; i++) {
        float x = i * buildingWidth + buildingWidth * 0.1f;
        float height = (30 + cpuActivity * 50 + sinf(i * 0.5f) * 20) + 
                      sinf(fGlobalAnimationPhase * 0.3f + i) * (cpuActivity * 10);
        
        BRect building(x, baseY - height, x + buildingWidth * 0.8f, baseY);
        fOffscreenView->FillRect(building);
        
        // Windows (if CPU is active)
        if (cpuActivity > 0.3f && (int)(fGlobalAnimationPhase * 2 + i) % 3 == 0) {
            fOffscreenView->SetHighColor(windowColor);
            fOffscreenView->SetDrawingMode(B_OP_ALPHA);
            
            for (int floor = 0; floor < height / 15; floor++) {
                for (int window = 0; window < 2; window++) {
                    BRect windowRect(
                        x + 5 + window * 8,
                        baseY - height + floor * 15 + 3,
                        x + 10 + window * 8,
                        baseY - height + floor * 15 + 8
                    );
                    fOffscreenView->FillRect(windowRect);
                }
            }
            fOffscreenView->SetDrawingMode(B_OP_COPY);
            fOffscreenView->SetHighColor(buildingColor);
        }
    }
}

void EcosystemView::DrawRiver(BRect bounds)
{
    // River represents memory flow
    if (!fWeatherEngine) return;
    
    float memoryFlow = 1.0f - fWeatherEngine->GetCloudCoverage(); // Inverse of cloud coverage
    
    // River curve
    rgb_color riverColor = {100, 149, 237, 180}; // Cornflower blue
    fOffscreenView->SetHighColor(riverColor);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    float riverY = bounds.Height() * 0.85f;
    float riverWidth = 20 + memoryFlow * 30;
    
    // Draw flowing river with sine wave
    for (int x = 0; x < bounds.Width(); x += 2) {
        float wave = sinf((x / 50.0f) + (fGlobalAnimationPhase * memoryFlow)) * 10;
        BRect waterSegment(x, riverY + wave - riverWidth/2, 
                          x + 2, riverY + wave + riverWidth/2);
        fOffscreenView->FillRect(waterSegment);
        
        // Add sparkles if flow is good
        if (memoryFlow > 0.7f && (x + (int)fGlobalAnimationPhase * 10) % 50 < 3) {
            fOffscreenView->SetHighColor(255, 255, 255, 150);
            fOffscreenView->FillRect(BRect(x, riverY + wave - 2, x + 2, riverY + wave + 2));
            fOffscreenView->SetHighColor(riverColor);
        }
    }
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawMusicNotes(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    float musicClarity = fWeatherEngine->GetMusicClarity();
    
    // Only show notes if audio is good
    if (musicClarity < 0.3f) return;
    
    rgb_color noteColor = {100, 200, 255, (uint8)(180 * musicClarity)};
    fOffscreenView->SetHighColor(noteColor);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    for (const auto& note : fMusicNotes) {
        if (!note.visible || note.position.y < bounds.Height() * 0.3f) continue;
        
        BPoint pos = note.position;
        pos.x += sinf(note.animation_phase * 6.28f) * 15;
        
        // Draw musical note (simplified)
        float size = 8 + sinf(note.animation_phase * 4) * 2;
        
        // Note head
        fOffscreenView->FillEllipse(pos, size, size);
        
        // Note stem (if clear enough)
        if (musicClarity > 0.6f) {
            fOffscreenView->StrokeLine(
                BPoint(pos.x + size * 0.7f, pos.y),
                BPoint(pos.x + size * 0.7f, pos.y - size * 2)
            );
        }
        
        // Note flag (if very clear)
        if (musicClarity > 0.8f) {
            BPoint flag[] = {
                BPoint(pos.x + size * 0.7f, pos.y - size * 2),
                BPoint(pos.x + size * 1.5f, pos.y - size * 1.5f),
                BPoint(pos.x + size * 1.2f, pos.y - size),
                BPoint(pos.x + size * 0.7f, pos.y - size * 1.2f)
            };
            fOffscreenView->FillPolygon(flag, 4);
        }
    }
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawWeatherEffects(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    WeatherCondition condition = fWeatherEngine->GetOverallWeather();
    
    // Draw weather effects based on condition
    switch (condition) {
        case RAINY:
        case STORMY:
            DrawRain(bounds);
            break;
        case OVERCAST:
            DrawFog(bounds);
            break;
        default:
            // No special effects for sunny/cloudy weather
            break;
    }
}

void EcosystemView::DrawRain(BRect bounds)
{
    rgb_color rainColor = {200, 200, 255, 120};
    fOffscreenView->SetHighColor(rainColor);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    fOffscreenView->SetPenSize(2);
    
    // Animated raindrops
    for (auto& drop : fRaindrops) {
        // Update drop position
        drop.position.y += 8 + sinf(drop.animation_phase) * 3;
        drop.position.x += 1; // Wind effect
        
        if (drop.position.y > bounds.Height()) {
            drop.position.y = -10;
            drop.position.x = rand() % (int)bounds.Width();
        }
        
        // Draw raindrop as line
        fOffscreenView->StrokeLine(
            drop.position,
            drop.position + BPoint(-2, 10)
        );
    }
    
    fOffscreenView->SetPenSize(1);
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawFog(BRect bounds)
{
    // Create fog overlay for poor performance
    rgb_color fogColor = {180, 180, 180, 80};
    fOffscreenView->SetHighColor(fogColor);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    // Draw animated fog patches
    for (int i = 0; i < 5; i++) {
        float x = bounds.Width() * (i + 1) / 6.0f;
        float y = bounds.Height() * 0.6f + sinf(fGlobalAnimationPhase + i) * 20;
        float size = 80 + cosf(fGlobalAnimationPhase * 0.5f + i * 0.5f) * 30;
        
        BRect fogPatch(x - size/2, y - size/3, x + size/2, y + size/3);
        fOffscreenView->FillEllipse(fogPatch);
    }
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawDetailOverlays(BRect bounds)
{
    switch (fDetailLevel) {
        case 1: // Meteorologist level
            DrawMeteorologistOverlay(bounds);
            break;
        case 2: // Technical level
            DrawTechnicalOverlay(bounds);
            break;
        default:
            // Weather level - no overlay
            break;
    }
}

void EcosystemView::DrawMeteorologistOverlay(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    // Semi-transparent background
    rgb_color overlayBg = {0, 0, 0, 100};
    fOffscreenView->SetHighColor(overlayBg);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    BRect infoPanel(bounds.right - 300, 20, bounds.right - 20, 200);
    fOffscreenView->FillRoundRect(infoPanel, 15, 15);
    
    // Draw detailed weather data
    fOffscreenView->SetHighColor(255, 255, 255, 200);
    fOffscreenView->SetFont(&fWeatherFont);
    
    char buffer[256];
    float y = infoPanel.top + 25;
    
    sprintf(buffer, "☀️ CPU Performance: %.1f%%", fWeatherEngine->GetSunBrightness() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "☁️ Memory Usage: %.1f%%", fWeatherEngine->GetCloudCoverage() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "🎵 Audio Quality: %.1f%%", fWeatherEngine->GetMusicClarity() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "💨 System Speed: %.1f%%", fWeatherEngine->GetWindSpeed() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 25;
    
    // Weather forecast
    fOffscreenView->SetFont(&fTitleFont);
    fOffscreenView->DrawString("Forecast:", BPoint(infoPanel.left + 15, y));
    y += 18;
    
    fOffscreenView->SetFont(&fWeatherFont);
    std::string forecast = fWeatherEngine->GetQuickForecast();
    fOffscreenView->DrawString(forecast.c_str(), BPoint(infoPanel.left + 15, y));
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawTechnicalOverlay(BRect bounds)
{
    if (!fWeatherEngine) return;
    
    // Technical data panel
    rgb_color overlayBg = {20, 20, 20, 180};
    fOffscreenView->SetHighColor(overlayBg);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    BRect techPanel(20, bounds.bottom - 150, bounds.right - 20, bounds.bottom - 20);
    fOffscreenView->FillRoundRect(techPanel, 10, 10);
    
    // Technical readouts
    fOffscreenView->SetHighColor(0, 255, 0, 220);
    fOffscreenView->SetFont(&fWeatherFont);
    
    float x = techPanel.left + 20;
    float y = techPanel.top + 25;
    
    // System metrics in technical format
    fOffscreenView->DrawString("SYSTEM TELEMETRY:", BPoint(x, y));
    y += 25;
    
    char buffer[512];
    sprintf(buffer, "CPU: %.2f GHz equiv. | MEM: %.1f%% util | AUDIO: %.1f ms latency",
        fWeatherEngine->GetSunBrightness() * 3.0f,
        fWeatherEngine->GetCloudCoverage() * 100,
        (1.0f - fWeatherEngine->GetMusicClarity()) * 50
    );
    fOffscreenView->DrawString(buffer, BPoint(x, y));
    y += 20;
    
    sprintf(buffer, "I/O: %.1f MB/s | NET: %.1f ms | OVERALL: %.1f/10",
        fWeatherEngine->GetWindSpeed() * 1000,
        (1.0f - fWeatherEngine->GetWindSpeed()) * 100,
        (fWeatherEngine->GetSunBrightness() + fWeatherEngine->GetMusicClarity() + 
         fWeatherEngine->GetWindSpeed() + (1.0f - fWeatherEngine->GetCloudCoverage())) * 2.5f
    );
    fOffscreenView->DrawString(buffer, BPoint(x, y));
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

BRect EcosystemView::GetElementRect(const char* element) const
{
    BRect bounds = Bounds();
    
    if (strcmp(element, "sun") == 0) {
        return BRect(bounds.Width() * 0.7f - 40, bounds.Height() * 0.2f - 40,
                    bounds.Width() * 0.7f + 40, bounds.Height() * 0.2f + 40);
    } else if (strcmp(element, "city") == 0) {
        return BRect(bounds.Width() * 0.1f, bounds.Height() * 0.4f,
                    bounds.Width() * 0.4f, bounds.Height() * 0.8f);
    } else if (strcmp(element, "river") == 0) {
        return BRect(0, bounds.Height() * 0.8f, bounds.Width(), bounds.Height());
    }
    
    return BRect();
}

void EcosystemView::ShowElementDetails(const char* element)
{
    // Send message to parent window to show details
    BMessage msg(MSG_ECOSYSTEM_CLICKED);
    msg.AddString("element", element);
    Window()->PostMessage(&msg);
}

void EcosystemView::MouseDown(BPoint where)
{
    // Check which ecosystem element was clicked
    BMessage msg(MSG_ECOSYSTEM_CLICKED);
    
    // Check sun - calculate distance manually
    BPoint diff = where - fSunPosition;
    float distance = sqrtf(diff.x * diff.x + diff.y * diff.y);
    if (distance < 40) {
        msg.AddString("element", "sun");
        msg.AddString("description", "CPU Performance");
    }
    // Check other elements as needed
    
    Window()->PostMessage(&msg);
}

void EcosystemView::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
    // Update hover state and show tooltips
    fLastMousePos = where;
    
    // Simple tooltip logic
    BPoint diff = where - fSunPosition;
    float distance = sqrtf(diff.x * diff.x + diff.y * diff.y);
    if (distance < 40) {
        if (fHoveredElement != "sun") {
            fHoveredElement = "sun";
            SetToolTip("☀️ CPU Performance - Click for details");
        }
    } else {
        if (fHoveredElement == "sun") {
            fHoveredElement = "";
            SetToolTip((const char*)nullptr);
        }
    }
}

void EcosystemView::FrameResized(float width, float height)
{
    BView::FrameResized(width, height);
    
    // Recreate offscreen bitmap
    delete fOffscreenBitmap;
    fOffscreenBitmap = nullptr;
    
    BRect bounds(0, 0, width, height);
    fOffscreenBitmap = new BBitmap(bounds, B_RGB32, true);
    if (fOffscreenBitmap && fOffscreenBitmap->IsValid()) {
        fOffscreenView = new BView(bounds, "offscreen", B_FOLLOW_ALL, B_WILL_DRAW);
        fOffscreenBitmap->AddChild(fOffscreenView);
    }
    
    Invalidate();
}

// WeatherStoryView implementation
WeatherStoryView::WeatherStoryView(BRect frame)
    : BView(frame, "weather_story", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
      fStoryAnimationPhase(0.0f),
      fAnimatingIn(false)
{
    SetViewColor(245, 250, 255);
    
    fStoryFont.SetSize(14);
    fForecastFont.SetSize(12);
    fForecastFont.SetFace(B_ITALIC_FACE);
}

WeatherStoryView::~WeatherStoryView()
{
}

void WeatherStoryView::AttachedToWindow()
{
    BView::AttachedToWindow();
    SetViewColor(245, 250, 255);
}

void WeatherStoryView::Draw(BRect updateRect)
{
    DrawBackground(Bounds());
    DrawStoryText(Bounds());
    DrawForecastBox(Bounds());
}

void WeatherStoryView::DrawBackground(BRect bounds)
{
    // Gentle gradient background
    rgb_color topColor = {245, 250, 255};
    rgb_color bottomColor = {230, 240, 250};
    
    SetHighColor(topColor);
    FillRect(BRect(bounds.left, bounds.top, bounds.right, bounds.Height()/2));
    
    SetHighColor(bottomColor);
    FillRect(BRect(bounds.left, bounds.Height()/2, bounds.right, bounds.bottom));
    
    // Decorative border
    SetHighColor(180, 200, 220);
    StrokeRect(bounds);
}

void WeatherStoryView::DrawStoryText(BRect bounds)
{
    if (fStory.empty()) return;
    
    SetHighColor(40, 60, 80);
    SetFont(&fStoryFont);
    
    BRect textRect = bounds;
    textRect.InsetBy(20, 20);
    textRect.bottom = bounds.bottom * 0.75f;
    
    // Title
    SetFont(&fForecastFont);
    DrawString("🌤️ Performance Weather Report", BPoint(textRect.left, textRect.top + 20));
    
    // Story text with line wrapping
    SetFont(&fStoryFont);
    float y = textRect.top + 50;
    
    // Simple word wrapping for story
    const char* text = fStory.c_str();
    int len = strlen(text);
    int start = 0;
    
    while (start < len) {
        int lineEnd = start;
        while (lineEnd < len && text[lineEnd] != '\n' && 
               (lineEnd - start) < 80) {
            lineEnd++;
        }
        
        // Find word boundary
        if (lineEnd < len && text[lineEnd] != '\n') {
            while (lineEnd > start && text[lineEnd] != ' ') {
                lineEnd--;
            }
        }
        
        if (lineEnd == start) lineEnd = start + 1;
        
        BString line(text + start, lineEnd - start);
        DrawString(line.String(), BPoint(textRect.left, y));
        
        y += 18;
        start = lineEnd + 1;
        
        if (y > textRect.bottom - 20) break;
    }
}

void WeatherStoryView::DrawForecastBox(BRect bounds)
{
    if (fForecast.empty()) return;
    
    // Forecast box
    BRect forecastRect = bounds;
    forecastRect.top = bounds.bottom * 0.75f + 10;
    forecastRect.InsetBy(20, 10);
    
    // Box background
    rgb_color boxColor = {220, 235, 250};
    SetHighColor(boxColor);
    FillRoundRect(forecastRect, 10, 10);
    
    // Box border
    SetHighColor(150, 180, 210);
    StrokeRoundRect(forecastRect, 10, 10);
    
    // Forecast text
    SetHighColor(60, 80, 100);
    SetFont(&fForecastFont);
    
    BPoint textPoint = forecastRect.LeftTop();
    textPoint.x += 15;
    textPoint.y += 20;
    
    DrawString("⚡ Quick Forecast:", textPoint);
    textPoint.y += 18;
    
    DrawString(fForecast.c_str(), textPoint);
}

void WeatherStoryView::SetStory(const std::string& story)
{
    fStory = story;
    Invalidate();
}

void WeatherStoryView::SetForecast(const std::string& forecast)
{
    fForecast = forecast;
    Invalidate();
}

void WeatherStoryView::AnimateStoryIn()
{
    fAnimatingIn = true;
    fStoryAnimationPhase = 0.0f;
    // Animation would be handled in Pulse() if needed
}

// TechnicalDetailsView implementation
TechnicalDetailsView::TechnicalDetailsView(BRect frame)
    : BView(frame, "technical_details", B_FOLLOW_ALL, B_WILL_DRAW),
      fExpanded(false),
      fExpansionPhase(0.0f)
{
    SetViewColor(240, 240, 240);
    
    fTechnicalFont.SetSize(11);
    fTechnicalFont.SetFace(B_REGULAR_FACE);
    
    fHeaderFont.SetSize(13);
    fHeaderFont.SetFace(B_BOLD_FACE);
}

TechnicalDetailsView::~TechnicalDetailsView()
{
}

void TechnicalDetailsView::Draw(BRect updateRect)
{
    DrawTechnicalData(Bounds());
    if (fExpanded) {
        DrawPerformanceGraphs(Bounds());
    }
}

void TechnicalDetailsView::DrawTechnicalData(BRect bounds)
{
    // Background
    rgb_color bgColor = {240, 240, 240};
    SetHighColor(bgColor);
    FillRect(bounds);
    
    // Header
    SetHighColor(60, 60, 60);
    SetFont(&fHeaderFont);
    DrawString("📊 Technical Performance Data", BPoint(20, 25));
    
    if (fResults.empty()) {
        SetFont(&fTechnicalFont);
        SetHighColor(120, 120, 120);
        DrawString("Run benchmark to see detailed technical results...", BPoint(20, 50));
        return;
    }
    
    // Results table
    SetFont(&fTechnicalFont);
    SetHighColor(40, 40, 40);
    
    float y = 55;
    for (const auto& result : fResults) {
        char buffer[512];
        sprintf(buffer, "%-20s: %8.2f %s (%.1f%% of optimal)",
            result.name.c_str(),
            result.value,
            result.unit.c_str(),
            result.score
        );
        DrawString(buffer, BPoint(20, y));
        y += 16;
        
        if (y > bounds.bottom - 10) break;
    }
}

void TechnicalDetailsView::DrawPerformanceGraphs(BRect bounds)
{
    if (fResults.empty()) return;
    
    // Mini performance graphs
    BRect graphArea = bounds;
    graphArea.top = bounds.Height() * 0.5f;
    graphArea.InsetBy(20, 10);
    
    SetHighColor(100, 100, 100);
    StrokeRect(graphArea);
    
    // Simple bar chart of scores
    float barWidth = graphArea.Width() / fResults.size();
    
    for (size_t i = 0; i < fResults.size(); i++) {
        float score = fResults[i].score / 100.0f; // Normalize
        
        rgb_color barColor;
        if (score > 0.8f) {
            barColor = {50, 200, 50}; // Green
        } else if (score > 0.6f) {
            barColor = {200, 200, 50}; // Yellow
        } else {
            barColor = {200, 50, 50}; // Red
        }
        
        SetHighColor(barColor);
        
        BRect bar;
        bar.left = graphArea.left + i * barWidth + 2;
        bar.right = bar.left + barWidth - 4;
        bar.bottom = graphArea.bottom - 2;
        bar.top = bar.bottom - (graphArea.Height() - 4) * score;
        
        FillRect(bar);
    }
}

void TechnicalDetailsView::SetBenchmarkResults(const std::vector<BenchmarkResult>& results)
{
    fResults = results;
    Invalidate();
}

void TechnicalDetailsView::SetExpanded(bool expanded)
{
    if (fExpanded != expanded) {
        fExpanded = expanded;
        Invalidate();
    }
}

void TechnicalDetailsView::AnimateExpansion(bool expand)
{
    SetExpanded(expand);
    // Animation logic could be added here
}

// WeatherBenchmarkWindow implementation
WeatherBenchmarkWindow::WeatherBenchmarkWindow(BRect frame)
    : BWindow(frame, "VeniceDAW Performance Station", 
              B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE),
      fPerformanceView(nullptr),
      fControlsView(nullptr),
      fResultsView(nullptr),
      fEcosystemView(nullptr),
      fStoryView(nullptr),
      fTechnicalView(nullptr),
      fMainLayout(nullptr),
      fShowingDetails(false),
      fWeatherEngine(nullptr),
      fBenchmark(nullptr),
      fBenchmarkThread(-1),
      fRunning(false),
      fCurrentDetailLevel(0)
{
    InitWeatherSystem();
    InitUI();
}

WeatherBenchmarkWindow::~WeatherBenchmarkWindow()
{
    if (fBenchmarkThread > 0) {
        kill_thread(fBenchmarkThread);
    }
    delete fBenchmark;
    delete fWeatherEngine;
}

void WeatherBenchmarkWindow::InitWeatherSystem()
{
    fWeatherEngine = new WeatherMetaphorEngine();
    fBenchmark = new PerformanceStation();
}

void WeatherBenchmarkWindow::InitUI()
{
    // Main container with Ableton dark background
    BView* mainView = new BView(Bounds(), "main", B_FOLLOW_ALL, B_WILL_DRAW);
    mainView->SetViewColor(AbletonColors::BACKGROUND);
    AddChild(mainView);
    
    // NEW LAYOUT: 3 sections - Top row has System Status + Controls, Performance Details separate below
    BRect bounds = Bounds();
    
    // TOP ROW: System Status (left) + PC Info (center) + Controls (right)
    BRect topRowRect = bounds;
    topRowRect.bottom = bounds.Height() * 0.35f;  // Reduced height
    
    // System Status (left - 40% width)
    BRect sysStatusRect = topRowRect;
    sysStatusRect.right = bounds.Width() * 0.4f;
    sysStatusRect.InsetBy(5, 5);
    fPerformanceView = new PerformanceMeterView(sysStatusRect);
    mainView->AddChild(fPerformanceView);
    
    // PC Info panel (center - 35% width) - NEW!
    BRect pcInfoRect = topRowRect;
    pcInfoRect.left = bounds.Width() * 0.4f;
    pcInfoRect.right = bounds.Width() * 0.75f;
    pcInfoRect.InsetBy(5, 5);
    fPCInfoView = new PCInfoView(pcInfoRect);
    mainView->AddChild(fPCInfoView);
    
    // Controls panel (right - 25% width)
    BRect controlsRect = topRowRect;
    controlsRect.left = bounds.Width() * 0.75f;
    controlsRect.InsetBy(5, 5);
    fControlsView = new BenchmarkControlsView(controlsRect);
    mainView->AddChild(fControlsView);
    
    // Performance Details (bottom - reduced height)
    BRect resultsRect = bounds;
    resultsRect.top = bounds.Height() * 0.35f;
    resultsRect.InsetBy(10, 10);
    fResultsView = new ResultsDetailView(resultsRect);
    mainView->AddChild(fResultsView);
    
    // Keep legacy views for compatibility but hidden initially
    fEcosystemView = new EcosystemView(BRect(-1000, -1000, -900, -900));
    fEcosystemView->SetWeatherEngine(fWeatherEngine);
    mainView->AddChild(fEcosystemView);
    
    fStoryView = new WeatherStoryView(BRect(-1000, -1000, -900, -900));
    mainView->AddChild(fStoryView);
    
    fTechnicalView = new TechnicalDetailsView(BRect(-1000, -1000, -900, -900));
    mainView->AddChild(fTechnicalView);
}

void WeatherBenchmarkWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case MSG_RUN_WEATHER_BENCHMARK:
            RunBenchmark();
            break;
            
        case MSG_TOGGLE_DETAIL_LEVEL:
            ToggleDetailLevel();
            break;
            
        case MSG_SHOW_TECHNICAL:
            ShowTechnicalDetails();
            break;
            
        case MSG_EXPORT_WEATHER_REPORT:
            ExportWeatherReport();
            break;
            
        case MSG_DETAILED_REPORT:
            ShowDetailedReport();
            break;
            
        case MSG_ECOSYSTEM_CLICKED:
        {
            const char* element = nullptr;
            if (message->FindString("element", &element) == B_OK) {
                // Show details for clicked ecosystem element
                char buffer[256];
                sprintf(buffer, "You clicked on: %s", element);
                // Could show tooltip or details panel
            }
            break;
        }
        
        case MSG_BENCHMARK_PROGRESS:
        {
            float progress;
            const char* testName = nullptr;
            
            if (message->FindFloat("progress", &progress) == B_OK &&
                message->FindString("test_name", &testName) == B_OK) {
                
                // Update progress bar in performance view
                if (fPerformanceView) {
                    fPerformanceView->SetProgress(progress, testName);
                }
            }
            break;
        }
        
        case MSG_BENCHMARK_COMPLETE:
            OnBenchmarkComplete();
            break;
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

bool WeatherBenchmarkWindow::QuitRequested()
{
    if (fRunning && fBenchmarkThread > 0) {
        // Stop benchmark thread before quitting
        kill_thread(fBenchmarkThread);
    }
    return true;
}

void WeatherBenchmarkWindow::RunBenchmark()
{
    if (fRunning) return;
    
    fRunning = true;
    fControlsView->SetRunning(true);
    fControlsView->EnableExport(false);
    
    // Start benchmark thread
    fBenchmarkThread = spawn_thread(BenchmarkThreadEntry, "weather_benchmark", 
                                   B_NORMAL_PRIORITY, this);
    resume_thread(fBenchmarkThread);
    
    // Start ecosystem animation (legacy)
    if (fEcosystemView) fEcosystemView->StartAnimation();
}

void WeatherBenchmarkWindow::ToggleDetailLevel()
{
    // New professional UI: toggle results detail view
    fShowingDetails = !fShowingDetails;
    fResultsView->SetExpanded(fShowingDetails);
}

void WeatherBenchmarkWindow::ShowTechnicalDetails()
{
    // Toggle between professional and legacy view
    fShowingDetails = !fShowingDetails;
    fResultsView->SetExpanded(fShowingDetails);
}

void WeatherBenchmarkWindow::UpdateWeatherDisplay()
{
    if (!fBenchmark || !fWeatherEngine) return;
    
    // Get benchmark results and update weather engine
    std::vector<BenchmarkResult> results = fBenchmark->GetResults();
    fWeatherEngine->UpdateFromBenchmark(results);
    
    // Update professional performance view
    fPerformanceView->SetMetrics(
        fWeatherEngine->GetSunBrightness(),    // CPU
        fWeatherEngine->GetCloudCoverage(),    // Memory
        fWeatherEngine->GetMusicClarity(),     // Audio
        fWeatherEngine->GetWindSpeed()         // I/O
    );
    
    // Calculate and set latency (fake for now, could be real from results)
    float avgLatency = 5.2f; // Could be calculated from audio tests
    fPerformanceView->SetLatency(avgLatency);
    
    // Update results detail view
    fResultsView->SetResults(results);
    
    // Update legacy views for compatibility
    if (fStoryView) {
        fStoryView->SetStory(fWeatherEngine->GetWeatherStory());
        fStoryView->SetForecast(fWeatherEngine->GetQuickForecast());
        fStoryView->AnimateStoryIn();
    }
    
    if (fTechnicalView) {
        fTechnicalView->SetBenchmarkResults(results);
    }
    
    if (fEcosystemView) {
        fEcosystemView->Invalidate();
    }
}

void WeatherBenchmarkWindow::ExportWeatherReport()
{
    if (!fWeatherEngine) return;
    
    // Create weather report export dialog
    BFilePanel* panel = new BFilePanel(B_SAVE_PANEL, nullptr, nullptr, 
                                      B_FILE_NODE, false, 
                                      new BMessage('svfl'));
    panel->SetSaveText("weather_report.txt");
    panel->Show();
}

void WeatherBenchmarkWindow::ShowDetailedReport()
{
    if (!fBenchmark || fBenchmark->GetResults().empty()) {
        BAlert* alert = new BAlert("No Data", "No benchmark results available.\nPlease run a test first.", 
                                  "OK", nullptr, nullptr, B_WIDTH_AS_USUAL, B_INFO_ALERT);
        alert->Go();
        return;
    }
    
    // Generate comprehensive report
    BString detailedReport;
    detailedReport << "=== VeniceDAW Performance Station - Detailed Report ===\n\n";
    
    // System Information - Get data directly since member variables are private
    detailedReport << "SYSTEM INFORMATION:\n";
    system_info sysInfo;
    if (get_system_info(&sysInfo) == B_OK) {
        detailedReport << "- CPU: " << sysInfo.cpu_count << " cores\n";
        detailedReport << "- RAM: " << (sysInfo.max_pages * B_PAGE_SIZE / (1024*1024)) << "MB\n";
        detailedReport << "- Architecture: ";
        
        // Detect architecture using uname
        struct utsname unameData;
        if (uname(&unameData) == 0) {
            BString machine(unameData.machine);
            if (machine.IFindFirst("x86_64") != B_ERROR || machine.IFindFirst("amd64") != B_ERROR) {
                detailedReport << "x86-64 (64-bit)\n";
            } else if (machine.IFindFirst("x86") != B_ERROR || machine.IFindFirst("i386") != B_ERROR) {
                detailedReport << "x86 (32-bit)\n";
            } else {
                detailedReport << machine.String() << "\n";
            }
        } else {
            detailedReport << "x86-64 (assumed)\n";
        }
    }
    detailedReport << "- OS: Haiku R1/Beta5+\n";
    detailedReport << "- Audio: Haiku Media Kit\n";
    detailedReport << "- Graphics: Native Haiku\n\n";
    
    // Benchmark Results
    detailedReport << "DETAILED BENCHMARK RESULTS:\n";
    const auto& results = fBenchmark->GetResults();
    
    for (const auto& result : results) {
        detailedReport << "- " << result.name.c_str() << ":\n";
        detailedReport << "  * Value: " << result.value << " " << result.unit.c_str() << "\n";
        detailedReport << "  * Score: " << result.score << "/100\n";
        detailedReport << "  * Duration: " << result.duration << "ms\n";
        detailedReport << "  * Category: " << result.category.c_str() << "\n\n";
    }
    
    detailedReport << "OVERALL SCORE: " << fBenchmark->GetTotalScore() << "/100\n\n";
    
    // Performance Analysis
    detailedReport << "PERFORMANCE ANALYSIS:\n";
    float totalScore = fBenchmark->GetTotalScore();
    if (totalScore > 85) {
        detailedReport << "Excellent performance - System optimal for professional audio production\n";
    } else if (totalScore > 70) {
        detailedReport << "Good performance - Suitable for most audio work\n";
    } else if (totalScore > 50) {
        detailedReport << "Fair performance - Consider optimization for demanding tasks\n";
    } else {
        detailedReport << "Poor performance - System optimization recommended\n";
    }
    
    // Show in alert with scrollable text
    BTextView* textView = new BTextView(BRect(0, 0, 600, 400), "report_text", 
                                       BRect(5, 5, 595, 395), B_FOLLOW_ALL);
    textView->SetText(detailedReport.String());
    textView->MakeEditable(false);
    textView->SetWordWrap(true);
    
    BWindow* reportWindow = new BWindow(BRect(100, 100, 750, 550), 
                                       "VeniceDAW - Detailed Performance Report",
                                       B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS);
    
    BScrollView* scrollView = new BScrollView("scroll", textView, B_FOLLOW_ALL, 0, 
                                             false, true, B_FANCY_BORDER);
    reportWindow->AddChild(scrollView);
    reportWindow->Show();
}

int32 WeatherBenchmarkWindow::BenchmarkThreadEntry(void* data)
{
    WeatherBenchmarkWindow* window = static_cast<WeatherBenchmarkWindow*>(data);
    window->RunBenchmarkTests();
    return 0;
}

void WeatherBenchmarkWindow::RunBenchmarkTests()
{
    if (!fBenchmark) return;
    
    // Set up progress callback
    fBenchmark->SetProgressCallback(ProgressCallback, this);
    
    // Run all benchmark tests
    fBenchmark->RunAllTests();
    
    // Post completion message to main thread
    BMessage msg(MSG_BENCHMARK_COMPLETE);
    PostMessage(&msg);
}

void WeatherBenchmarkWindow::ProgressCallback(float progress, const char* testName, void* userData)
{
    WeatherBenchmarkWindow* window = (WeatherBenchmarkWindow*)userData;
    
    // Create progress message
    BMessage msg(MSG_BENCHMARK_PROGRESS);
    msg.AddFloat("progress", progress);
    msg.AddString("test_name", testName);
    
    // Post to main thread
    window->PostMessage(&msg);
}

void WeatherBenchmarkWindow::OnBenchmarkComplete()
{
    fRunning = false;
    fBenchmarkThread = -1;
    
    fControlsView->SetRunning(false);
    fControlsView->EnableExport(true);
    
    // Update weather display with results
    UpdateWeatherDisplay();
    
    // Auto-expand results after completion
    fShowingDetails = true;
    fResultsView->SetExpanded(true);
    
    // Stop ecosystem animation (legacy)
    if (fEcosystemView) fEcosystemView->StopAnimation();
}

} // namespace HaikuDAW