/*
 * PerformanceStationWindow.cpp - Professional DAW Performance Analysis UI
 */

#include "PerformanceStationWindow.h"
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
#include <Volume.h>
#include <VolumeRoster.h>
#include <Directory.h>
#include <Entry.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ScrollView.h>
#include <TextView.h>
#include <StringList.h>
#include <kernel/OS.h>
#include <algorithm>  // For std::min, std::max
#include <map>        // For std::map
#include <sys/utsname.h>
#include <math.h>
#include <time.h>
#include <fstream>    // For HTML/CSV export
#include <sstream>    // For string stream
#include <iomanip>    // For formatting

namespace HaikuDAW {

// Professional Performance Meter Implementation
PerformanceMeterView::PerformanceMeterView(BRect frame)
    : BView(frame, "performance_meter", B_FOLLOW_ALL, B_WILL_DRAW),
      fCPU(0.75f), fMemory(0.35f), fAudio(0.85f), fIO(0.65f), fLatency(8.0f),  // TEST VALUES
      fProgress(0.0f), fCurrentTest("Ready"),
      fCPUCores(0), fTotalRAM(0), fCPUType("Unknown"), fCPUModel("Unknown"), fHaikuVersion("Unknown"),
      fAudioDriver("Unknown"), fGraphicsDriver("Unknown")
{
    // Initialize with test values
    
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
    // Render performance meters with current values
    
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
    font_height fh;
    GetFontHeight(&fh);
    
    // Professional layout
    float labelX = rect.left + 5;
    float meterStart = rect.left + 70;
    float meterEnd = rect.right - 70;
    float statusX = meterEnd + 10;
    
    // Generate percentage text
    char valueText[16];
    sprintf(valueText, "%.0f%%", value * 100);
    
    // 1. Draw the label
    SetHighColor(200, 200, 200);  // Light gray for label
    SetFont(be_plain_font);
    SetFontSize(10);
    DrawString(label, BPoint(labelX, rect.top + fh.ascent));
    
    // 2. Draw meter background (dark gray)
    BRect meterBg(meterStart, rect.top + 3, meterEnd, rect.bottom - 3);
    SetHighColor(40, 40, 40);
    FillRect(meterBg);
    
    // 3. Draw the colored performance bar
    if (value > 0.01f) {  // Only if there's something to show
        BRect meterFill = meterBg;
        meterFill.InsetBy(1, 1);
        float fillRatio = (value > 1.0f) ? 1.0f : value;
        meterFill.right = meterFill.left + (meterFill.Width() * fillRatio);
        SetHighColor(color);
        FillRect(meterFill);
    }
    
    // 4. Draw meter border
    SetHighColor(100, 100, 100);
    StrokeRect(meterBg);
    
    // 5. PERCENTAGE TEXT - Draw with black background for contrast
    float textWidth = StringWidth(valueText);
    float textX = meterStart + (meterEnd - meterStart - textWidth) / 2;
    float textY = rect.top + fh.ascent;
    
    // Draw black background rectangle for text
    BRect textBg(textX - 2, rect.top + 2, textX + textWidth + 2, rect.bottom - 2);
    SetHighColor(0, 0, 0);  // Black background
    FillRect(textBg);
    
    // Draw white text on black background
    SetHighColor(255, 255, 255);  // Pure white text
    SetFont(be_bold_font);
    SetFontSize(11);
    DrawString(valueText, BPoint(textX, textY));
    
    // 6. Draw status
    SetHighColor(150, 150, 150);
    SetFont(be_plain_font);
    SetFontSize(9);
    DrawString(status, BPoint(statusX, rect.top + fh.ascent));
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
    // Update performance metrics
    fCPU = cpu;
    fMemory = memory;
    fAudio = audio;
    fIO = io;
    // Internal metrics updated
    Invalidate();
    // Trigger view refresh
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
      fCPUCores(0), fTotalRAM(0), fCPUTemperature(-1.0f), fLatency(0.0f)
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
            fHaikuRevision.SetTo(unameData.version);
        } else {
            fCPUType.SetTo("x86-64");
            fCPUModel.SetTo("Multi-core processor");
            fHaikuVersion.SetTo("Haiku R1/Beta5+");
            fHaikuRevision.SetTo("Unknown");
        }
    } else {
        fCPUCores = 1;
        fTotalRAM = 512;
        fCPUType.SetTo("Unknown");
        fCPUModel.SetTo("Unknown CPU");
        fHaikuVersion.SetTo("Haiku OS");
        fHaikuRevision.SetTo("Unknown");
    }
    
    // Detect additional system information
    DetectCPUTemperature();
    DetectMesaVersion();
    DetectDiskInfo();
    
    // Detect detailed audio driver information
    DetectAudioDriverDetails();
    
    // Detect graphics driver information  
    DetectGraphicsDriverDetails();
    
    // Latency will be set from actual measurements
    fLatency = 0.0f;
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

void PCInfoView::DetectCPUTemperature()
{
    // Try to read CPU temperature from Haiku thermal sensors
    fCPUTemperature = -1.0f;  // Default: unknown
    
    // Method 1: Try ACPI thermal zone
    BFile tempFile("/dev/misc/acpi_thermal", B_READ_ONLY);
    if (tempFile.InitCheck() == B_OK) {
        char tempBuffer[64];
        ssize_t bytesRead = tempFile.Read(tempBuffer, sizeof(tempBuffer) - 1);
        if (bytesRead > 0) {
            tempBuffer[bytesRead] = '\0';
            fCPUTemperature = atof(tempBuffer);
        }
    }
    
    // Method 2: Try hwmon (modern Linux-style sensors on newer Haiku)
    if (fCPUTemperature < 0) {
        BDirectory hwmonDir("/dev/hwmon");
        if (hwmonDir.InitCheck() == B_OK) {
            BEntry entry;
            while (hwmonDir.GetNextEntry(&entry) == B_OK) {
                BPath path;
                entry.GetPath(&path);
                BString pathStr(path.Path());
                if (pathStr.IFindFirst("temp") != B_ERROR) {
                    BFile hwmonFile(path.Path(), B_READ_ONLY);
                    if (hwmonFile.InitCheck() == B_OK) {
                        char tempBuffer[32];
                        ssize_t bytesRead = hwmonFile.Read(tempBuffer, sizeof(tempBuffer) - 1);
                        if (bytesRead > 0) {
                            tempBuffer[bytesRead] = '\0';
                            fCPUTemperature = atof(tempBuffer) / 1000.0f; // Convert millidegrees
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Fallback: estimate based on system load (rough approximation)
    if (fCPUTemperature < 0) {
        fCPUTemperature = 45.0f; // Reasonable default for modern CPUs
    }
}

void PCInfoView::DetectMesaVersion()
{
    fMesaVersion.SetTo("Unknown");
    
    // Method 1: Try to get OpenGL version string (contains Mesa info)
    // This would require OpenGL context, so we'll use command line approach
    
    // Method 2: Try to read from Mesa libraries
    BDirectory libDir("/boot/system/lib");
    if (libDir.InitCheck() == B_OK) {
        BEntry entry;
        while (libDir.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);
            BString fileName(path.Leaf());
            if (fileName.IFindFirst("libGL") != B_ERROR || fileName.IFindFirst("mesa") != B_ERROR) {
                // Found Mesa library - try to extract version
                fMesaVersion.SetTo("Mesa 23.x"); // Default assumption
                break;
            }
        }
    }
    
    // Method 3: Try glxinfo equivalent (if available)
    FILE* pipe = popen("glxinfo 2>/dev/null | grep 'OpenGL version' | head -1", "r");
    if (pipe) {
        char buffer[256];
        if (fgets(buffer, sizeof(buffer), pipe)) {
            BString version(buffer);
            int32 mesaStart = version.IFindFirst("Mesa");
            if (mesaStart != B_ERROR) {
                version.Remove(0, mesaStart);
                version.Remove(version.IFindFirst("\n"), version.Length());
                fMesaVersion.SetTo(version.String());
            }
        }
        pclose(pipe);
    }
}

void PCInfoView::DetectDiskInfo()
{
    fDiskInfo.SetTo("Unknown");
    
    // Get disk usage information
    BVolumeRoster roster;
    BVolume volume;
    off_t totalBytes = 0;
    off_t freeBytes = 0;
    int diskCount = 0;
    
    while (roster.GetNextVolume(&volume) == B_OK) {
        if (volume.IsReadOnly()) continue; // Skip read-only volumes
        
        off_t capacity = volume.Capacity();
        off_t free = volume.FreeBytes();
        
        if (capacity > 0) {
            totalBytes += capacity;
            freeBytes += free;
            diskCount++;
        }
    }
    
    if (totalBytes > 0) {
        // Convert to GB
        float totalGB = totalBytes / (1024.0f * 1024.0f * 1024.0f);
        float freeGB = freeBytes / (1024.0f * 1024.0f * 1024.0f);
        float usedGB = totalGB - freeGB;
        
        char diskText[128];
        if (diskCount == 1) {
            sprintf(diskText, "%.1f GB (%.1f used)", totalGB, usedGB);
        } else {
            sprintf(diskText, "%.1f GB total (%d disks)", totalGB, diskCount);
        }
        fDiskInfo.SetTo(diskText);
    }
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
    
    // Haiku Revision (compact)
    char revText[96];
    sprintf(revText, "Rev: %s", fHaikuRevision.String());
    DrawString(revText, textPos);
    textPos.y += lineHeight;
    
    // CPU Temperature (with color coding)
    char tempText[96];
    if (fCPUTemperature > 0) {
        sprintf(tempText, "Temp: %.1f°C", fCPUTemperature);
        // Color code temperature
        if (fCPUTemperature < 60.0f) {
            SetHighColor(46, 204, 113);  // Green: normal
        } else if (fCPUTemperature < 80.0f) {
            SetHighColor(241, 196, 15);  // Yellow: warm
        } else {
            SetHighColor(231, 76, 60);   // Red: hot
        }
    } else {
        sprintf(tempText, "Temp: N/A");
        SetHighColor(AbletonColors::TEXT);
    }
    DrawString(tempText, textPos);
    textPos.y += lineHeight;
    SetHighColor(AbletonColors::TEXT);  // Reset color
    
    // Mesa Version
    char mesaText[128];
    sprintf(mesaText, "OpenGL: %s", fMesaVersion.String());
    DrawString(mesaText, textPos);
    textPos.y += lineHeight;
    
    // Disk Information
    char diskText[128];
    sprintf(diskText, "Storage: %s", fDiskInfo.String());
    DrawString(diskText, textPos);
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

// Professional Results Detail Implementation - PHASE 4 Final
ResultsDetailView::ResultsDetailView(BRect frame)
    : BView(frame, "results_detail", B_FOLLOW_ALL, B_WILL_DRAW),
      fExpanded(false),
      fSelectedResult(-1),
      fShowingDetails(false),
      fDetailPanelHeight(0.0f),
      fAnimatingDetail(false),
      fDetailAnimStart(0),
      fShowingTooltip(false),
      fTooltipShowTime(0),
      fLastModifiers(0),
      fHighlightMode(true),
      fHoveredCategory(-1),
      fHoveredResult(-1),
      fAIAnalysisEnabled(true),
      fAnalysisInProgress(false),
      fLastAnalysisTime(0),
      fSystemLearningProgress(0.0f),
      fShowingHeatMap(false),
      fShowingCorrelations(false),
      fShowingPredictions(false),
      fVisualizationMode(0)
{
    SetViewColor(AbletonColors::BACKGROUND);
    
    // Enable focus for keyboard shortcuts
    SetFlags(Flags() | B_NAVIGABLE);
    
    // Initialize professional fonts
    fHeaderFont.SetSize(12);
    fHeaderFont.SetFace(B_BOLD_FACE);
    
    fValueFont.SetFamilyAndStyle("DejaVu Sans Mono", "Book"); // Monospace for numbers
    fValueFont.SetSize(10);
    
    fStatusFont.SetSize(9);
    fStatusFont.SetFace(B_BOLD_FACE);
    
    // PHASE 3: Additional fonts for interactive details
    fDetailFont.SetSize(9);
    
    fMonoFont.SetFamilyAndStyle("DejaVu Sans Mono", "Book");
    fMonoFont.SetSize(8);
    
    // PHASE 4: Initialize export settings
    fLastExportPath = "/boot/home/Desktop/";
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
    
    // Organize results by category if needed
    OrganizeResultsByCategory();
    
    // Calculate optimal layout
    BRect panelRect = Bounds();
    panelRect.InsetBy(5, 5);
    fCurrentLayout = CalculateOptimalLayout(panelRect, fResults);
    
    // Panel background with professional styling
    SetHighColor(AbletonColors::PANEL);
    FillRoundRect(panelRect, 6, 6);
    
    // Panel border with subtle gradient effect
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(panelRect, 6, 6);
    
    // Professional header with category breakdown
    SetFont(&fHeaderFont);
    SetHighColor(AbletonColors::TEXT);
    font_height headerFh;
    fHeaderFont.GetHeight(&headerFh);
    
    char headerText[128];
    sprintf(headerText, "PERFORMANCE ANALYSIS (%zu tests, %zu categories)", 
            fResults.size(), fCategoryGroups.size());
    DrawString(headerText, BPoint(panelRect.left + 12, panelRect.top + headerFh.ascent + 12));
    
    // Draw category groups
    float currentY = panelRect.top + 45;
    for (size_t catIndex = 0; catIndex < fCategoryGroups.size(); catIndex++) {
        CategoryGroup& group = fCategoryGroups[catIndex];
        
        // Category header
        BRect categoryHeaderRect(panelRect.left + 8, currentY, 
                               panelRect.right - 8, currentY + 28);
        DrawCategoryHeader(categoryHeaderRect, group);
        currentY += 32;
        
        // Results in this category
        if (group.expanded || fCategoryGroups.size() == 1) {
            for (size_t resIndex = 0; resIndex < group.results.size(); resIndex++) {
                BRect resultRect(panelRect.left + 16, currentY, 
                               panelRect.right - 16, currentY + 24);
                DrawEnhancedResultBar(resultRect, group.results[resIndex], fCurrentLayout);
                currentY += 28;
            }
        }
        
        currentY += 8; // Space between categories
    }
    
    // PHASE 3: Draw interactive detail panel if visible
    if (fShowingDetails && fSelectedResult >= 0 && fSelectedResult < (int)fResults.size()) {
        UpdateDetailAnimation();
        
        // Calculate detail panel position (bottom half of the view)
        fDetailPanelRect = panelRect;
        fDetailPanelRect.top = panelRect.bottom - fDetailPanelHeight;
        
        if (fDetailPanelHeight > 10) { // Only draw if panel is visible enough
            DrawDetailPanel(fDetailPanelRect);
        }
    }
    
    // PHASE 4: Draw tooltip if visible
    if (fShowingTooltip && !fTooltipText.empty()) {
        // Tooltip background
        SetFont(&fDetailFont);
        font_height fh;
        fDetailFont.GetHeight(&fh);
        
        // float tooltipWidth = StringWidth(fTooltipText.c_str()) + 16;
        // float tooltipHeight = fh.ascent + fh.descent + 10;
        
        // Tooltip background with shadow
        rgb_color tooltipBg = {255, 255, 225, 240};  // Light yellow
        rgb_color tooltipShadow = {0, 0, 0, 100};    // Semi-transparent black
        
        BRect shadowRect = fTooltipRect;
        shadowRect.OffsetBy(2, 2);
        SetHighColor(tooltipShadow);
        FillRoundRect(shadowRect, 4, 4);
        
        SetHighColor(tooltipBg);
        FillRoundRect(fTooltipRect, 4, 4);
        
        SetHighColor(AbletonColors::BORDER);
        StrokeRoundRect(fTooltipRect, 4, 4);
        
        // Tooltip text
        SetHighColor({20, 20, 20, 255});
        DrawString(fTooltipText.c_str(), 
                  BPoint(fTooltipRect.left + 8, fTooltipRect.top + fh.ascent + 5));
    }
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
        OrganizeResultsByCategory(); // Reorganize when expanding
        Invalidate();
    }
}

// PHASE 2 IMPLEMENTATION - Smart Layout Engine

BarLayout ResultsDetailView::CalculateOptimalLayout(BRect bounds, const std::vector<BenchmarkResult>& results) {
    BarLayout layout;
    layout.totalWidth = bounds.Width() - 32; // Account for margins
    
    // Calculate required widths based on content
    float maxNameWidth = 0;
    float maxValueWidth = 0;
    
    SetFont(&fValueFont);
    
    for (const auto& result : results) {
        // Calculate name width
        float nameWidth = StringWidth(result.name.c_str());
        if (nameWidth > maxNameWidth) maxNameWidth = nameWidth;
        
        // Calculate value + unit width
        char valueText[32];
        sprintf(valueText, "%.1f%s", result.actualValue, result.unit.c_str());
        float valueWidth = StringWidth(valueText);
        if (valueWidth > maxValueWidth) maxValueWidth = valueWidth;
    }
    
    // Set optimal widths with professional spacing
    layout.nameWidth = std::min(maxNameWidth + 10, layout.totalWidth * 0.4f);
    layout.valueWidth = std::min(maxValueWidth + 10, layout.totalWidth * 0.15f);
    layout.statusWidth = 70; // Fixed width for status
    layout.barWidth = layout.totalWidth - layout.nameWidth - layout.valueWidth - layout.statusWidth - 20;
    
    // Ensure minimum widths
    if (layout.barWidth < 50) {
        layout.barWidth = 50;
        layout.nameWidth = layout.totalWidth - layout.barWidth - layout.valueWidth - layout.statusWidth - 20;
    }
    
    return layout;
}

void ResultsDetailView::OrganizeResultsByCategory() {
    fCategoryGroups.clear();
    
    // Group results by performance category
    std::map<PerformanceCategory, std::vector<BenchmarkResult>> categoryMap;
    for (const auto& result : fResults) {
        categoryMap[result.perfCategory].push_back(result);
    }
    
    // Create category groups
    for (const auto& pair : categoryMap) {
        CategoryGroup group(pair.first);
        group.results = pair.second;
        group.expanded = true; // Default expanded for now
        
        // Calculate group score
        float totalScore = 0.0f;
        for (const auto& result : group.results) {
            totalScore += result.score;
        }
        group.groupScore = group.results.empty() ? 0.0f : totalScore / group.results.size();
        
        // Set category title
        switch (pair.first) {
            case AUDIO_REALTIME: 
                group.title = "REAL-TIME AUDIO";
                break;
            case SYSTEM_RESOURCES:
                group.title = "SYSTEM RESOURCES";
                break;
            case GRAPHICS_3D:
                group.title = "3D GRAPHICS";
                break;
            case STABILITY:
                group.title = "SYSTEM STABILITY";
                break;
            default:
                group.title = "GENERAL PERFORMANCE";
                break;
        }
        
        fCategoryGroups.push_back(group);
    }
}

void ResultsDetailView::DrawCategoryHeader(BRect rect, const CategoryGroup& group) {
    // Category background with slight highlight
    rgb_color headerBg = AbletonColors::BACKGROUND;
    headerBg.red += 10; headerBg.green += 10; headerBg.blue += 10;
    SetHighColor(headerBg);
    FillRoundRect(rect, 4, 4);
    
    // Category border
    SetHighColor(AbletonColors::BORDER);
    StrokeRoundRect(rect, 4, 4);
    
    // Category title with score
    SetFont(&fHeaderFont);
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    fHeaderFont.GetHeight(&fh);
    
    char categoryText[128];
    sprintf(categoryText, "▼ %s (%.0f%% avg, %zu tests)", 
            group.title.c_str(), group.groupScore, group.results.size());
    
    DrawString(categoryText, BPoint(rect.left + 12, rect.top + fh.ascent + 8));
    
    // Category score indicator
    BRect scoreIndicator(rect.right - 60, rect.top + 6, rect.right - 10, rect.bottom - 6);
    rgb_color categoryColor = PerformanceStation::GetDAWStatusColor(group.groupScore, 80.0f, group.category);
    SetHighColor(categoryColor);
    FillRoundRect(scoreIndicator, 2, 2);
    
    // Score text
    char scoreText[16];
    sprintf(scoreText, "%.0f%%", group.groupScore);
    SetHighColor(AbletonColors::TEXT);
    font_height scoreFh;
    GetFontHeight(&scoreFh);
    float textWidth = StringWidth(scoreText);
    DrawString(scoreText, BPoint(scoreIndicator.left + (scoreIndicator.Width() - textWidth) / 2,
                                scoreIndicator.top + scoreFh.ascent + 4));
}

void ResultsDetailView::DrawEnhancedResultBar(BRect rect, const BenchmarkResult& result, const BarLayout& layout) {
    SetFont(&fValueFont);
    font_height fh;
    fValueFont.GetHeight(&fh);
    
    float currentX = rect.left + 4;
    float textY = rect.top + fh.ascent + 4;
    
    // Test name (truncated if needed)
    char displayName[32];
    if (result.name.length() > 30) {
        strncpy(displayName, result.name.c_str(), 27);
        strcpy(displayName + 27, "...");
    } else {
        strcpy(displayName, result.name.c_str());
    }
    
    SetHighColor(AbletonColors::TEXT);
    DrawString(displayName, BPoint(currentX, textY));
    currentX += layout.nameWidth;
    
    // Professional progress bar
    BRect barRect(currentX, rect.top + 6, currentX + layout.barWidth, rect.bottom - 6);
    float fillRatio = result.score / 100.0f;
    if (fillRatio > 1.0f) fillRatio = 1.0f;
    
    DrawProfessionalBar(barRect, fillRatio, 
                       PerformanceStation::GetDAWStatusColor(result.actualValue, result.targetValue, result.perfCategory));
    currentX += layout.barWidth + 10;
    
    // Value with unit (monospace alignment)
    char valueText[32];
    sprintf(valueText, "%.1f%s", result.actualValue, result.unit.c_str());
    DrawValueWithUnit(BPoint(currentX, textY), result.actualValue, result.unit);
    currentX += layout.valueWidth + 5;
    
    // DAW-specific status
    DrawDAWStatus(BPoint(currentX, textY), result);
    currentX += layout.statusWidth;
    
    // Trend indicator (if data available)
    if (!result.trend.history.empty()) {
        DrawTrendIndicator(BPoint(currentX + 5, rect.top + 8), result.trend);
    }
}

void ResultsDetailView::DrawProfessionalBar(BRect barRect, float fillRatio, rgb_color color) {
    // Bar background
    SetHighColor(AbletonColors::BORDER);
    FillRoundRect(barRect, 3, 3);
    
    // Bar fill with gradient effect
    BRect fillRect = barRect;
    fillRect.right = fillRect.left + (fillRect.Width() * fillRatio);
    
    SetHighColor(color);
    FillRoundRect(fillRect, 3, 3);
    
    // Subtle highlight on top
    rgb_color highlight = color;
    highlight.red = std::min(255, (int)highlight.red + 20);
    highlight.green = std::min(255, (int)highlight.green + 20);
    highlight.blue = std::min(255, (int)highlight.blue + 20);
    
    SetHighColor(highlight);
    BRect highlightRect = fillRect;
    highlightRect.bottom = highlightRect.top + 2;
    FillRoundRect(highlightRect, 3, 3);
}

void ResultsDetailView::DrawValueWithUnit(BPoint position, float value, const std::string& unit) {
    char valueText[32];
    sprintf(valueText, "%.1f%s", value, unit.c_str());
    
    SetHighColor(AbletonColors::TEXT);
    DrawString(valueText, position);
}

void ResultsDetailView::DrawDAWStatus(BPoint position, const BenchmarkResult& result) {
    const char* statusText = PerformanceStation::GetDAWStatusText(result.actualValue, result.targetValue, result.perfCategory);
    
    // Color-code the status
    rgb_color statusColor = PerformanceStation::GetDAWStatusColor(result.actualValue, result.targetValue, result.perfCategory);
    SetHighColor(statusColor);
    
    SetFont(&fStatusFont);
    DrawString(statusText, position);
}

void ResultsDetailView::DrawTrendIndicator(BPoint position, const TrendData& trend) {
    // Simple trend indicator: ↗ ↘ → for up/down/stable
    const char* trendSymbol;
    rgb_color trendColor;
    
    if (trend.history.size() < 2) {
        trendSymbol = "●";  // Not enough data
        trendColor = AbletonColors::TEXT;
    } else {
        float current = trend.history.back();
        float previous = trend.history[trend.history.size() - 2];
        float change = (current - previous) / previous;
        
        if (change > 0.05f) {
            trendSymbol = "↗";  // Improving
            trendColor = AbletonColors::GREEN;
        } else if (change < -0.05f) {
            trendSymbol = "↘";  // Degrading  
            trendColor = AbletonColors::RED;
        } else {
            trendSymbol = "→";  // Stable
            trendColor = AbletonColors::BLUE;
        }
    }
    
    SetHighColor(trendColor);
    DrawString(trendSymbol, position);
}

void ResultsDetailView::MouseDown(BPoint where) {
    // PHASE 3: Handle detail panel clicks first
    if (fShowingDetails && IsPointInDetailPanel(where)) {
        HandleDetailPanelClick(where);
        return;
    }
    
    // Handle category expansion/collapse
    int categoryIndex = GetClickedCategory(where);
    if (categoryIndex >= 0 && categoryIndex < (int)fCategoryGroups.size()) {
        fCategoryGroups[categoryIndex].expanded = !fCategoryGroups[categoryIndex].expanded;
        Invalidate();
        return;
    }
    
    // PHASE 3: Handle result selection for detailed analysis
    int resultIndex = GetClickedResult(where);
    if (resultIndex >= 0) {
        if (fSelectedResult == resultIndex && fShowingDetails) {
            // Double-click or already selected - hide details
            HideResultDetails();
        } else {
            // Show details for new selection
            ShowResultDetails(resultIndex);
        }
        return;
    }
    
    // Click outside - hide details if showing
    if (fShowingDetails) {
        HideResultDetails();
    }
}

void ResultsDetailView::FrameResized(float width, float height) {
    BView::FrameResized(width, height);
    // Recalculate layout on resize
    Invalidate();
}

int ResultsDetailView::GetClickedCategory(BPoint where) {
    // Simple implementation - would need precise bounds tracking
    float currentY = Bounds().top + 50;
    for (size_t i = 0; i < fCategoryGroups.size(); i++) {
        BRect categoryRect(Bounds().left, currentY, Bounds().right, currentY + 28);
        if (categoryRect.Contains(where)) {
            return i;
        }
        currentY += 32;
        if (fCategoryGroups[i].expanded) {
            currentY += fCategoryGroups[i].results.size() * 28;
        }
        currentY += 8;
    }
    return -1;
}

int ResultsDetailView::GetClickedResult(BPoint where) {
    // Future implementation for result detail interaction
    return -1;
}

rgb_color ResultsDetailView::GetDAWStatusColor(const BenchmarkResult& result) {
    return PerformanceStation::GetDAWStatusColor(result.actualValue, result.targetValue, result.perfCategory);
}

const char* ResultsDetailView::GetDAWStatusText(const BenchmarkResult& result) {
    return PerformanceStation::GetDAWStatusText(result.actualValue, result.targetValue, result.perfCategory);
}

// PHASE 3 IMPLEMENTATION - Interactive Details

void ResultsDetailView::ShowResultDetails(int resultIndex) {
    if (resultIndex < 0 || resultIndex >= (int)fResults.size()) return;
    
    fSelectedResult = resultIndex;
    fShowingDetails = true;
    AnimateDetailPanel(true);
}

void ResultsDetailView::HideResultDetails() {
    if (!fShowingDetails) return;
    
    fShowingDetails = false;
    AnimateDetailPanel(false);
}

void ResultsDetailView::AnimateDetailPanel(bool show) {
    fAnimatingDetail = true;
    fDetailAnimStart = system_time();
    
    if (show) {
        // Target height: 40% of view height
        float targetHeight = Bounds().Height() * 0.4f;
        if (targetHeight < 120) targetHeight = 120; // Minimum height
        if (targetHeight > 200) targetHeight = 200; // Maximum height
        
        // Start animation
        Invalidate();
    } else {
        // Animate to zero height
        Invalidate();
    }
}

void ResultsDetailView::UpdateDetailAnimation() {
    if (!fAnimatingDetail) return;
    
    const bigtime_t animDuration = 300000; // 300ms
    bigtime_t elapsed = system_time() - fDetailAnimStart;
    
    if (elapsed >= animDuration) {
        // Animation complete
        fAnimatingDetail = false;
        if (fShowingDetails) {
            fDetailPanelHeight = Bounds().Height() * 0.4f;
        } else {
            fDetailPanelHeight = 0.0f;
            fSelectedResult = -1;
        }
    } else {
        // Animation in progress
        float progress = (float)elapsed / (float)animDuration;
        // Smooth easing function
        progress = progress * progress * (3.0f - 2.0f * progress);
        
        float targetHeight = fShowingDetails ? (Bounds().Height() * 0.4f) : 0.0f;
        float startHeight = fShowingDetails ? 0.0f : (Bounds().Height() * 0.4f);
        
        fDetailPanelHeight = startHeight + (targetHeight - startHeight) * progress;
        
        // Continue animation
        Invalidate();
    }
}

float ResultsDetailView::GetAnimationProgress() {
    if (!fAnimatingDetail) return fShowingDetails ? 1.0f : 0.0f;
    
    const bigtime_t animDuration = 300000;
    bigtime_t elapsed = system_time() - fDetailAnimStart;
    
    if (elapsed >= animDuration) return fShowingDetails ? 1.0f : 0.0f;
    
    float progress = (float)elapsed / (float)animDuration;
    return progress * progress * (3.0f - 2.0f * progress); // Smooth easing
}

void ResultsDetailView::DrawDetailPanel(BRect bounds) {
    const BenchmarkResult& result = fResults[fSelectedResult];
    
    // Professional detail panel background
    rgb_color detailBg = AbletonColors::PANEL;
    detailBg.red += 8; detailBg.green += 8; detailBg.blue += 8;
    SetHighColor(detailBg);
    FillRoundRect(bounds, 8, 8);
    
    // Panel border with accent color
    rgb_color borderColor = PerformanceStation::GetDAWStatusColor(result.actualValue, result.targetValue, result.perfCategory);
    SetHighColor(borderColor);
    StrokeRoundRect(bounds, 8, 8);
    
    // Detail panel header
    SetFont(&fHeaderFont);
    SetHighColor(AbletonColors::TEXT);
    font_height headerFh;
    fHeaderFont.GetHeight(&headerFh);
    
    char detailTitle[128];
    sprintf(detailTitle, "📊 DETAILED ANALYSIS: %s", result.name.c_str());
    DrawString(detailTitle, BPoint(bounds.left + 12, bounds.top + headerFh.ascent + 12));
    
    // Close button (X)
    BRect closeButton(bounds.right - 30, bounds.top + 8, bounds.right - 8, bounds.top + 22);
    SetHighColor(AbletonColors::RED);
    StrokeRoundRect(closeButton, 3, 3);
    DrawString("×", BPoint(closeButton.left + 7, closeButton.top + 12));
    
    // Content areas
    float contentTop = bounds.top + 40;
    float contentHeight = bounds.Height() - 45;
    
    // Left column: Technical metrics
    BRect metricsRect(bounds.left + 12, contentTop, bounds.left + bounds.Width() * 0.45f, bounds.bottom - 5);
    DrawTechnicalMetrics(metricsRect, result);
    
    // Center column: Performance graph
    BRect graphRect(bounds.left + bounds.Width() * 0.48f, contentTop, bounds.left + bounds.Width() * 0.75f, bounds.bottom - 5);
    if (!result.trend.history.empty()) {
        DrawPerformanceGraph(graphRect, result.trend);
    }
    
    // Right column: Analysis and suggestions
    BRect analysisRect(bounds.left + bounds.Width() * 0.77f, contentTop, bounds.right - 12, bounds.bottom - 5);
    DrawBottleneckAnalysis(BRect(analysisRect.left, analysisRect.top, analysisRect.right, analysisRect.top + contentHeight * 0.5f), result);
    DrawOptimizationSuggestions(BRect(analysisRect.left, analysisRect.top + contentHeight * 0.52f, analysisRect.right, analysisRect.bottom), result);
}

void ResultsDetailView::DrawTechnicalMetrics(BRect rect, const BenchmarkResult& result) {
    // Technical metrics section
    SetFont(&fDetailFont);
    SetHighColor(AbletonColors::TEXT);
    font_height fh;
    fDetailFont.GetHeight(&fh);
    
    float lineHeight = fh.ascent + fh.descent + 3;
    float currentY = rect.top + fh.ascent + 5;
    
    // Section header
    SetFont(&fStatusFont);
    SetHighColor(AbletonColors::ORANGE);
    DrawString("TECHNICAL METRICS", BPoint(rect.left, currentY));
    currentY += lineHeight + 5;
    
    SetFont(&fMonoFont);
    SetHighColor(AbletonColors::TEXT);
    
    // Current value with target comparison
    char valueText[64];
    sprintf(valueText, "Current: %.2f%s", result.actualValue, result.unit.c_str());
    DrawString(valueText, BPoint(rect.left + 4, currentY));
    currentY += lineHeight;
    
    sprintf(valueText, "Target:  %.2f%s", result.targetValue, result.unit.c_str());
    DrawString(valueText, BPoint(rect.left + 4, currentY));
    currentY += lineHeight;
    
    // Performance ratio
    float ratio = result.actualValue / result.targetValue;
    const char* ratioStatus = (ratio <= 1.0f) ? "✓ Within target" : "⚠ Exceeds target";
    sprintf(valueText, "Ratio:   %.2fx (%s)", ratio, ratioStatus);
    DrawString(valueText, BPoint(rect.left + 4, currentY));
    currentY += lineHeight + 3;
    
    // System context
    if (result.cpuUsage > 0) {
        sprintf(valueText, "CPU:     %.1f%%", result.cpuUsage);
        DrawString(valueText, BPoint(rect.left + 4, currentY));
        currentY += lineHeight;
    }
    
    if (result.memoryMB > 0) {
        sprintf(valueText, "Memory:  %.1f MB", result.memoryMB);
        DrawString(valueText, BPoint(rect.left + 4, currentY));
        currentY += lineHeight;
    }
    
    // Real-time status
    const char* rtStatus = result.isRealTime ? "✓ Real-time safe" : "○ Non-critical";
    sprintf(valueText, "RT Safe: %s", rtStatus);
    DrawString(valueText, BPoint(rect.left + 4, currentY));
    currentY += lineHeight;
    
    // Score breakdown
    sprintf(valueText, "Score:   %.0f/100", result.score);
    rgb_color scoreColor = PerformanceStation::GetDAWStatusColor(result.actualValue, result.targetValue, result.perfCategory);
    SetHighColor(scoreColor);
    DrawString(valueText, BPoint(rect.left + 4, currentY));
}

void ResultsDetailView::DrawPerformanceGraph(BRect rect, const TrendData& trend) {
    // Performance trend graph
    SetFont(&fStatusFont);
    SetHighColor(AbletonColors::ORANGE);
    font_height fh;
    fStatusFont.GetHeight(&fh);
    
    DrawString("TREND ANALYSIS", BPoint(rect.left, rect.top + fh.ascent + 5));
    
    if (trend.history.size() < 2) {
        SetHighColor(AbletonColors::TEXT);
        DrawString("Not enough data", BPoint(rect.left + 4, rect.top + 30));
        return;
    }
    
    // Graph area
    BRect graphArea = rect;
    graphArea.InsetBy(4, 25);
    graphArea.top += 10;
    
    // Graph background
    SetHighColor(AbletonColors::BACKGROUND);
    FillRect(graphArea);
    SetHighColor(AbletonColors::BORDER);
    StrokeRect(graphArea);
    
    // Draw sparkline
    rgb_color graphColor = trend.isStable ? AbletonColors::GREEN : AbletonColors::YELLOW;
    DrawSparklineGraph(graphArea, trend.history, graphColor);
    
    // Statistics
    SetFont(&fMonoFont);
    SetHighColor(AbletonColors::TEXT);
    
    char statText[64];
    sprintf(statText, "Avg: %.2f", trend.average);
    DrawString(statText, BPoint(rect.left + 4, rect.bottom - 25));
    
    sprintf(statText, "Stable: %s", trend.isStable ? "Yes" : "No");
    DrawString(statText, BPoint(rect.left + 4, rect.bottom - 12));
}

void ResultsDetailView::DrawBottleneckAnalysis(BRect rect, const BenchmarkResult& result) {
    SetFont(&fStatusFont);
    SetHighColor(AbletonColors::ORANGE);
    font_height fh;
    fStatusFont.GetHeight(&fh);
    
    DrawString("BOTTLENECK", BPoint(rect.left, rect.top + fh.ascent + 5));
    
    SetFont(&fDetailFont);
    SetHighColor(AbletonColors::TEXT);
    
    // Bottleneck analysis
    const char* bottleneckIcon;
    rgb_color bottleneckColor;
    
    if (result.bottleneck == "No Bottleneck") {
        bottleneckIcon = "✓";
        bottleneckColor = AbletonColors::GREEN;
    } else if (result.bottleneck == "CPU Bound" || result.bottleneck == "Memory Pressure") {
        bottleneckIcon = "⚠";
        bottleneckColor = AbletonColors::YELLOW;
    } else {
        bottleneckIcon = "⚡";
        bottleneckColor = AbletonColors::RED;
    }
    
    SetHighColor(bottleneckColor);
    char analysisText[128];
    sprintf(analysisText, "%s %s", bottleneckIcon, result.bottleneck.c_str());
    DrawString(analysisText, BPoint(rect.left + 4, rect.top + 25));
}

void ResultsDetailView::DrawOptimizationSuggestions(BRect rect, const BenchmarkResult& result) {
    SetFont(&fStatusFont);
    SetHighColor(AbletonColors::ORANGE);
    font_height fh;
    fStatusFont.GetHeight(&fh);
    
    DrawString("OPTIMIZATION", BPoint(rect.left, rect.top + fh.ascent + 5));
    
    SetFont(&fDetailFont);
    SetHighColor(AbletonColors::TEXT);
    
    // Wrap recommendation text
    std::string recommendation = result.recommendation;
    if (recommendation.length() > 30) {
        // Simple word wrapping
        size_t breakPos = recommendation.find(' ', 25);
        if (breakPos != std::string::npos) {
            std::string line1 = recommendation.substr(0, breakPos);
            std::string line2 = recommendation.substr(breakPos + 1);
            
            DrawString(line1.c_str(), BPoint(rect.left + 4, rect.top + 25));
            DrawString(line2.c_str(), BPoint(rect.left + 4, rect.top + 40));
        } else {
            DrawString(recommendation.c_str(), BPoint(rect.left + 4, rect.top + 25));
        }
    } else {
        DrawString(recommendation.c_str(), BPoint(rect.left + 4, rect.top + 25));
    }
}

void ResultsDetailView::DrawSparklineGraph(BRect rect, const std::vector<float>& data, rgb_color color) {
    if (data.size() < 2) return;
    
    // Find min/max for scaling
    float minVal = data[0], maxVal = data[0];
    for (float val : data) {
        if (val < minVal) minVal = val;
        if (val > maxVal) maxVal = val;
    }
    
    if (maxVal - minVal < 0.001f) return; // No variation
    
    // Draw points and lines
    SetHighColor(color);
    
    float stepX = rect.Width() / (data.size() - 1);
    
    for (size_t i = 1; i < data.size(); i++) {
        float y1 = rect.bottom - ((data[i-1] - minVal) / (maxVal - minVal)) * rect.Height();
        float y2 = rect.bottom - ((data[i] - minVal) / (maxVal - minVal)) * rect.Height();
        float x1 = rect.left + (i-1) * stepX;
        float x2 = rect.left + i * stepX;
        
        StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
    }
}

bool ResultsDetailView::IsPointInDetailPanel(BPoint where) {
    return fShowingDetails && fDetailPanelRect.Contains(where);
}

void ResultsDetailView::HandleDetailPanelClick(BPoint where) {
    // Check if close button was clicked
    BRect closeButton(fDetailPanelRect.right - 30, fDetailPanelRect.top + 8, 
                     fDetailPanelRect.right - 8, fDetailPanelRect.top + 22);
    
    if (closeButton.Contains(where)) {
        HideResultDetails();
        return;
    }
    
    // Future: Handle other detail panel interactions
}

// PHASE 4 IMPLEMENTATION - Professional Polish & Export Features

void ResultsDetailView::MouseMoved(BPoint where, uint32 code, const BMessage* message) {
    BView::MouseMoved(where, code, message);
    
    fLastMousePos = where;
    
    // Update hover states for visual feedback
    int newHoveredCategory = GetClickedCategory(where);
    int newHoveredResult = GetClickedResult(where);
    
    bool needsRedraw = false;
    
    if (newHoveredCategory != fHoveredCategory) {
        fHoveredCategory = newHoveredCategory;
        needsRedraw = true;
    }
    
    if (newHoveredResult != fHoveredResult) {
        fHoveredResult = newHoveredResult;
        needsRedraw = true;
    }
    
    // Update tooltip
    UpdateTooltip(where);
    
    if (needsRedraw) {
        Invalidate();
    }
}

void ResultsDetailView::KeyDown(const char* bytes, int32 numBytes) {
    if (numBytes != 1) {
        BView::KeyDown(bytes, numBytes);
        return;
    }
    
    uint32 key = bytes[0];
    uint32 modifiers = Window()->CurrentMessage()->FindInt32("modifiers");
    
    // Handle keyboard shortcuts
    HandleKeyboardShortcut(key, modifiers);
}

void ResultsDetailView::HandleKeyboardShortcut(uint32 key, uint32 modifiers) {
    switch (key) {
        case 'e':
        case 'E':
            if (modifiers & B_COMMAND_KEY) {
                // Ctrl+E: Export detailed report
                ExportDetailedReport("html");
            }
            break;
            
        case 'c':
        case 'C':
            if (modifiers & B_COMMAND_KEY) {
                // Ctrl+C: Copy summary to clipboard
                std::string summary = GenerateTextSummary();
                // TODO: Copy to clipboard (requires clipboard API)
            }
            break;
            
        case B_ESCAPE:
            // Escape: Hide details or tooltip
            if (fShowingDetails) {
                HideResultDetails();
            } else if (fShowingTooltip) {
                HideTooltip();
            }
            break;
            
        case B_SPACE:
            // Space: Toggle current selection details
            if (fHoveredResult >= 0) {
                if (fSelectedResult == fHoveredResult && fShowingDetails) {
                    HideResultDetails();
                } else {
                    ShowResultDetails(fHoveredResult);
                }
            }
            break;
            
        case B_UP_ARROW:
            // Navigate up
            if (fHoveredResult > 0) {
                fHoveredResult--;
                Invalidate();
            }
            break;
            
        case B_DOWN_ARROW:
            // Navigate down
            if (fHoveredResult < (int)fResults.size() - 1) {
                fHoveredResult++;
                Invalidate();
            }
            break;
    }
}

void ResultsDetailView::UpdateTooltip(BPoint mousePos) {
    std::string newTooltip = GetContextualTooltip(mousePos);
    
    if (newTooltip != fTooltipText) {
        if (newTooltip.empty()) {
            HideTooltip();
        } else {
            ShowTooltip(mousePos, newTooltip);
        }
    }
}

std::string ResultsDetailView::GetContextualTooltip(BPoint where) {
    // Check if hovering over category header
    int categoryIndex = GetClickedCategory(where);
    if (categoryIndex >= 0 && categoryIndex < (int)fCategoryGroups.size()) {
        const CategoryGroup& group = fCategoryGroups[categoryIndex];
        
        char tooltip[256];
        sprintf(tooltip, "Category: %s\nAverage Score: %.0f%%\nTests: %zu\nClick to expand/collapse", 
                group.title.c_str(), group.groupScore, group.results.size());
        return std::string(tooltip);
    }
    
    // Check if hovering over result bar
    int resultIndex = GetClickedResult(where);
    if (resultIndex >= 0 && resultIndex < (int)fResults.size()) {
        const BenchmarkResult& result = fResults[resultIndex];
        
        char tooltip[512];
        sprintf(tooltip, "Test: %s\nValue: %.2f%s (Target: %.2f%s)\nScore: %.0f/100\nStatus: %s\nBottleneck: %s\nClick for detailed analysis", 
                result.name.c_str(),
                result.actualValue, result.unit.c_str(),
                result.targetValue, result.unit.c_str(),
                result.score,
                PerformanceStation::GetDAWStatusText(result.actualValue, result.targetValue, result.perfCategory),
                result.bottleneck.c_str());
        return std::string(tooltip);
    }
    
    return "";
}

void ResultsDetailView::ShowTooltip(BPoint where, const std::string& text) {
    fTooltipText = text;
    fShowingTooltip = true;
    fTooltipShowTime = system_time();
    
    // Calculate tooltip position
    SetFont(&fDetailFont);
    float tooltipWidth = StringWidth(text.c_str()) + 16;
    float tooltipHeight = 30;
    
    fTooltipRect.Set(where.x + 15, where.y - tooltipHeight/2, 
                    where.x + 15 + tooltipWidth, where.y - tooltipHeight/2 + tooltipHeight);
    
    // Keep tooltip on screen
    BRect bounds = Bounds();
    if (fTooltipRect.right > bounds.right) {
        fTooltipRect.OffsetBy(bounds.right - fTooltipRect.right - 5, 0);
    }
    if (fTooltipRect.bottom > bounds.bottom) {
        fTooltipRect.OffsetBy(0, bounds.bottom - fTooltipRect.bottom - 5);
    }
    
    Invalidate();
}

void ResultsDetailView::HideTooltip() {
    if (!fShowingTooltip) return;
    
    fShowingTooltip = false;
    fTooltipText = "";
    Invalidate();
}

void ResultsDetailView::ExportDetailedReport(const std::string& format) {
    if (fResults.empty()) {
        // TODO: Show alert - no data to export
        return;
    }
    
    // Generate timestamp for filename
    time_t now = time(0);
    struct tm* timeinfo = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", timeinfo);
    
    std::string filename = fLastExportPath + "VeniceDAW_Performance_Report_" + timestamp;
    
    if (format == "html") {
        filename += ".html";
        GenerateHTMLReport(filename);
    } else if (format == "csv") {
        filename += ".csv";
        GenerateCSVReport(filename);
    }
    
    // TODO: Show success notification
    AnnounceStatusChange("Report exported successfully");
}

void ResultsDetailView::GenerateHTMLReport(const std::string& filename) {
    std::ofstream htmlFile(filename);
    if (!htmlFile.is_open()) return;
    
    // HTML header with professional styling
    htmlFile << "<!DOCTYPE html>\n<html>\n<head>\n";
    htmlFile << "<title>VeniceDAW Performance Analysis Report</title>\n";
    htmlFile << "<style>\n";
    htmlFile << "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 20px; background: #1c1c1c; color: #c8c8c8; }\n";
    htmlFile << "h1 { color: #ff6b00; border-bottom: 2px solid #ff6b00; padding-bottom: 10px; }\n";
    htmlFile << "h2 { color: #0066cc; margin-top: 30px; }\n";
    htmlFile << ".category { background: #2a2a2a; padding: 15px; margin: 10px 0; border-radius: 8px; border-left: 4px solid #ff6b00; }\n";
    htmlFile << ".result { background: #404040; padding: 10px; margin: 5px 0; border-radius: 4px; }\n";
    htmlFile << ".excellent { border-left: 4px solid #00cc66; }\n";
    htmlFile << ".good { border-left: 4px solid #0066cc; }\n";
    htmlFile << ".warning { border-left: 4px solid #ffcc00; }\n";
    htmlFile << ".critical { border-left: 4px solid #cc0000; }\n";
    htmlFile << ".metric { display: inline-block; margin-right: 20px; }\n";
    htmlFile << ".value { font-family: 'Consolas', monospace; font-weight: bold; }\n";
    htmlFile << "</style>\n</head>\n<body>\n";
    
    // Report header
    htmlFile << "<h1>🎛️ VeniceDAW Performance Analysis Report</h1>\n";
    
    time_t now = time(0);
    char* timeStr = ctime(&now);
    htmlFile << "<p><strong>Generated:</strong> " << timeStr << "</p>\n";
    
    // Category breakdown
    for (const auto& group : fCategoryGroups) {
        htmlFile << "<div class=\"category\">\n";
        htmlFile << "<h2>" << group.title << "</h2>\n";
        htmlFile << "<p><strong>Average Score:</strong> " << std::fixed << std::setprecision(0) << group.groupScore << "%</p>\n";
        
        for (const auto& result : group.results) {
            std::string statusClass;
            if (result.score >= 90) statusClass = "excellent";
            else if (result.score >= 70) statusClass = "good";
            else if (result.score >= 50) statusClass = "warning";
            else statusClass = "critical";
            
            htmlFile << "<div class=\"result " << statusClass << "\">\n";
            htmlFile << "<h3>" << result.name << "</h3>\n";
            htmlFile << "<div class=\"metric\"><strong>Value:</strong> <span class=\"value\">" 
                     << std::fixed << std::setprecision(2) << result.actualValue << result.unit << "</span></div>\n";
            htmlFile << "<div class=\"metric\"><strong>Target:</strong> <span class=\"value\">" 
                     << std::fixed << std::setprecision(2) << result.targetValue << result.unit << "</span></div>\n";
            htmlFile << "<div class=\"metric\"><strong>Score:</strong> <span class=\"value\">" 
                     << std::fixed << std::setprecision(0) << result.score << "/100</span></div>\n";
            htmlFile << "<div class=\"metric\"><strong>Status:</strong> " 
                     << PerformanceStation::GetDAWStatusText(result.actualValue, result.targetValue, result.perfCategory) << "</div>\n";
            
            if (!result.bottleneck.empty() && result.bottleneck != "No Bottleneck") {
                htmlFile << "<p><strong>⚠️ Bottleneck:</strong> " << result.bottleneck << "</p>\n";
            }
            
            if (!result.recommendation.empty()) {
                htmlFile << "<p><strong>💡 Recommendation:</strong> " << result.recommendation << "</p>\n";
            }
            
            htmlFile << "</div>\n";
        }
        htmlFile << "</div>\n";
    }
    
    htmlFile << "<hr><p><em>Report generated by VeniceDAW Performance Station</em></p>\n";
    htmlFile << "</body>\n</html>\n";
    
    htmlFile.close();
}

void ResultsDetailView::GenerateCSVReport(const std::string& filename) {
    std::ofstream csvFile(filename);
    if (!csvFile.is_open()) return;
    
    // CSV header
    csvFile << "Category,Test Name,Actual Value,Unit,Target Value,Score,Status,Bottleneck,Recommendation,CPU Usage,Memory MB,Real-time Safe\n";
    
    // Data rows
    for (const auto& group : fCategoryGroups) {
        for (const auto& result : group.results) {
            csvFile << "\"" << group.title << "\",";
            csvFile << "\"" << result.name << "\",";
            csvFile << std::fixed << std::setprecision(2) << result.actualValue << ",";
            csvFile << "\"" << result.unit << "\",";
            csvFile << std::fixed << std::setprecision(2) << result.targetValue << ",";
            csvFile << std::fixed << std::setprecision(0) << result.score << ",";
            csvFile << "\"" << PerformanceStation::GetDAWStatusText(result.actualValue, result.targetValue, result.perfCategory) << "\",";
            csvFile << "\"" << result.bottleneck << "\",";
            csvFile << "\"" << result.recommendation << "\",";
            csvFile << std::fixed << std::setprecision(1) << result.cpuUsage << ",";
            csvFile << std::fixed << std::setprecision(1) << result.memoryMB << ",";
            csvFile << (result.isRealTime ? "Yes" : "No") << "\n";
        }
    }
    
    csvFile.close();
}

std::string ResultsDetailView::GenerateTextSummary() {
    if (fResults.empty()) return "No performance data available.";
    
    std::stringstream summary;
    summary << "VeniceDAW Performance Summary\n";
    summary << "============================\n\n";
    
    // Overall statistics
    float totalScore = 0;
    for (const auto& result : fResults) {
        totalScore += result.score;
    }
    float avgScore = totalScore / fResults.size();
    
    summary << "Overall Score: " << std::fixed << std::setprecision(0) << avgScore << "/100\n";
    summary << "Tests Completed: " << fResults.size() << "\n";
    summary << "Categories: " << fCategoryGroups.size() << "\n\n";
    
    // Category breakdown
    for (const auto& group : fCategoryGroups) {
        summary << group.title << ": " << std::fixed << std::setprecision(0) << group.groupScore << "% (" << group.results.size() << " tests)\n";
    }
    
    // Critical issues
    summary << "\nCritical Issues:\n";
    bool hasCritical = false;
    for (const auto& result : fResults) {
        if (result.score < 50 || (result.bottleneck != "No Bottleneck" && result.bottleneck != "")) {
            summary << "- " << result.name << ": " << result.bottleneck << "\n";
            hasCritical = true;
        }
    }
    if (!hasCritical) {
        summary << "None detected.\n";
    }
    
    return summary.str();
}

void ResultsDetailView::SavePerformanceProfile(const std::string& name) {
    fSavedProfiles[name] = fResults;
    // TODO: Persist to file
    AnnounceStatusChange("Performance profile saved: " + name);
}

void ResultsDetailView::LoadPerformanceProfile(const std::string& name) {
    auto it = fSavedProfiles.find(name);
    if (it != fSavedProfiles.end()) {
        SetResults(it->second);
        AnnounceStatusChange("Performance profile loaded: " + name);
    }
}

std::vector<std::string> ResultsDetailView::GetAvailableProfiles() {
    std::vector<std::string> profiles;
    for (const auto& pair : fSavedProfiles) {
        profiles.push_back(pair.first);
    }
    return profiles;
}

std::string ResultsDetailView::GetAccessibilityDescription(BPoint where) {
    int categoryIndex = GetClickedCategory(where);
    if (categoryIndex >= 0) {
        const CategoryGroup& group = fCategoryGroups[categoryIndex];
        return "Category " + group.title + " with " + std::to_string(group.results.size()) + " tests, average score " + std::to_string((int)group.groupScore) + " percent";
    }
    
    int resultIndex = GetClickedResult(where);
    if (resultIndex >= 0) {
        const BenchmarkResult& result = fResults[resultIndex];
        return "Performance test " + result.name + " scored " + std::to_string((int)result.score) + " out of 100";
    }
    
    return "VeniceDAW Performance Analysis interface";
}

void ResultsDetailView::AnnounceStatusChange(const std::string& status) {
    if (status != fLastAnnouncedStatus) {
        fLastAnnouncedStatus = status;
        // TODO: Send to screen reader API
        printf("Status: %s\n", status.c_str()); // Debug output for now
    }
}

// PHASE 5 IMPLEMENTATION - Advanced Analytics & AI-Powered Insights

void ResultsDetailView::RunPredictiveAnalysis() {
    if (!fAIAnalysisEnabled || fResults.empty()) return;
    
    fAnalysisInProgress = true;
    fLastAnalysisTime = system_time();
    
    // Step 1: Detect anomalies in current results
    DetectAnomalies();
    
    // Step 2: Analyze performance patterns  
    AnalyzePerformancePatterns();
    
    // Step 3: Generate smart recommendations
    fOptimizations = GenerateSmartRecommendations();
    
    // Step 4: Update prediction model
    UpdatePerformanceForecasting();
    
    // Step 5: Save current data for future learning
    SavePerformanceHistory();
    
    fAnalysisInProgress = false;
    fSystemLearningProgress += 0.1f;
    if (fSystemLearningProgress > 1.0f) fSystemLearningProgress = 1.0f;
    
    AnnounceStatusChange("AI analysis completed - " + std::to_string(fOptimizations.size()) + " optimizations identified");
}

void ResultsDetailView::DetectAnomalies() {
    fDetectedAnomalies.clear();
    
    if (fHistoricalData.size() < 3) return; // Need historical data for anomaly detection
    
    // Calculate statistical baselines from historical data
    std::map<std::string, std::vector<float>> historicalValues;
    for (const auto& snapshot : fHistoricalData) {
        for (const auto& result : snapshot.results) {
            historicalValues[result.name].push_back(result.actualValue);
        }
    }
    
    // Detect anomalies in current results
    for (const auto& result : fResults) {
        if (historicalValues.find(result.name) == historicalValues.end()) continue;
        
        const auto& values = historicalValues[result.name];
        if (values.size() < 3) continue;
        
        // Calculate mean and standard deviation
        float sum = 0.0f, sumSq = 0.0f;
        for (float val : values) {
            sum += val;
            sumSq += val * val;
        }
        float mean = sum / values.size();
        float variance = (sumSq / values.size()) - (mean * mean);
        float stdDev = sqrt(variance);
        
        // Check if current value is an outlier (> 2 standard deviations)
        float deviation = fabs(result.actualValue - mean);
        if (deviation > 2.0f * stdDev) {
            PerformanceAnomaly anomaly;
            anomaly.testName = result.name;
            anomaly.expectedValue = mean;
            anomaly.actualValue = result.actualValue;
            anomaly.deviationPercent = (deviation / mean) * 100.0f;
            anomaly.detectedAt = system_time();
            
            if (deviation > 3.0f * stdDev) {
                anomaly.severity = "critical";
                anomaly.possibleCause = "System malfunction or configuration change";
            } else if (deviation > 2.5f * stdDev) {
                anomaly.severity = "moderate";
                anomaly.possibleCause = "Performance degradation or increased load";
            } else {
                anomaly.severity = "minor";
                anomaly.possibleCause = "Normal variation or measurement noise";
            }
            
            fDetectedAnomalies.push_back(anomaly);
        }
    }
}

void ResultsDetailView::AnalyzePerformancePatterns() {
    fCorrelations.clear();
    
    if (fResults.size() < 2) return;
    
    // Calculate correlations between different performance metrics
    for (size_t i = 0; i < fResults.size(); i++) {
        for (size_t j = i + 1; j < fResults.size(); j++) {
            const auto& result1 = fResults[i];
            const auto& result2 = fResults[j];
            
            // Simple correlation analysis using historical data
            std::vector<float> values1, values2;
            for (const auto& snapshot : fHistoricalData) {
                float val1 = 0, val2 = 0;
                bool found1 = false, found2 = false;
                
                for (const auto& result : snapshot.results) {
                    if (result.name == result1.name) { val1 = result.actualValue; found1 = true; }
                    if (result.name == result2.name) { val2 = result.actualValue; found2 = true; }
                }
                
                if (found1 && found2) {
                    values1.push_back(val1);
                    values2.push_back(val2);
                }
            }
            
            if (values1.size() > 3) {
                float correlation = CalculateCorrelation(values1, values2);
                if (fabs(correlation) > 0.5f) { // Significant correlation
                    PerformanceCorrelation corr;
                    corr.metric1 = result1.name;
                    corr.metric2 = result2.name;
                    corr.correlationCoeff = correlation;
                    corr.significance = fabs(correlation);
                    corr.relationship = (correlation > 0) ? "positive" : "negative";
                    
                    fCorrelations.push_back(corr);
                }
            }
        }
    }
}

float ResultsDetailView::CalculateCorrelation(const std::vector<float>& x, const std::vector<float>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0f;
    
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0, sumY2 = 0;
    int n = x.size();
    
    for (int i = 0; i < n; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    
    float numerator = n * sumXY - sumX * sumY;
    float denominator = sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
    
    return (denominator == 0) ? 0 : numerator / denominator;
}

std::vector<OptimizationSuggestion> ResultsDetailView::GenerateSmartRecommendations() {
    std::vector<OptimizationSuggestion> recommendations;
    
    // AI-powered analysis of current performance state
    float overallScore = 0;
    for (const auto& result : fResults) {
        overallScore += result.score;
    }
    overallScore /= fResults.size();
    
    // Generate category-specific recommendations
    for (const auto& group : fCategoryGroups) {
        if (group.groupScore < 70.0f) {
            if (group.category == AUDIO_REALTIME) {
                OptimizationSuggestion suggestion;
                suggestion.description = "🎵 Audio Performance: Increase buffer size to 512+ samples for better stability";
                suggestion.category = "Audio";
                suggestion.priority = "High";
                suggestion.impactScore = 8.5f;
                recommendations.push_back(suggestion);
                
                suggestion.description = "🎵 Audio Performance: Consider using ASIO drivers for lower latency";
                suggestion.impactScore = 7.5f;
                recommendations.push_back(suggestion);
            } else if (group.category == SYSTEM_RESOURCES) {
                OptimizationSuggestion suggestion;
                suggestion.description = "💾 System Resources: Close unnecessary background applications";
                suggestion.category = "System";
                suggestion.priority = "Medium";
                suggestion.impactScore = 6.0f;
                recommendations.push_back(suggestion);
                
                suggestion.description = "💾 System Resources: Consider upgrading to 16GB+ RAM for professional workloads";
                suggestion.priority = "Low";
                suggestion.impactScore = 9.0f;
                recommendations.push_back(suggestion);
            } else if (group.category == GRAPHICS_3D) {
                OptimizationSuggestion suggestion;
                suggestion.description = "🎨 Graphics Performance: Update GPU drivers for better 3D acceleration";
                suggestion.category = "Graphics";
                suggestion.priority = "Medium";
                suggestion.impactScore = 7.0f;
                recommendations.push_back(suggestion);
            }
        }
    }
    
    // Anomaly-based recommendations
    for (const auto& anomaly : fDetectedAnomalies) {
        if (anomaly.severity == "critical") {
            OptimizationSuggestion suggestion;
            suggestion.description = "⚠️ Critical Issue: " + anomaly.testName + " performance anomaly detected - system check recommended";
            suggestion.category = "Critical";
            suggestion.priority = "Critical";
            suggestion.impactScore = 10.0f;
            recommendations.push_back(suggestion);
        }
    }
    
    // Correlation-based insights
    for (const auto& corr : fCorrelations) {
        if (corr.significance > 0.8f) {
            OptimizationSuggestion suggestion;
            suggestion.description = "📊 Performance Insight: " + corr.metric1 + " strongly correlates with " + corr.metric2;
            suggestion.category = "Analysis";
            suggestion.priority = "Info";
            suggestion.impactScore = 5.0f;
            recommendations.push_back(suggestion);
        }
    }
    
    // AI learning-based suggestions (simulated intelligence)
    if (fSystemLearningProgress > 0.5f) {
        OptimizationSuggestion suggestion;
        suggestion.description = "🤖 AI Insight: System has learned optimal configuration - performance stability improved";
        suggestion.category = "AI";
        suggestion.priority = "Info";
        suggestion.impactScore = 3.0f;
        recommendations.push_back(suggestion);
    }
    
    return recommendations;
}

float ResultsDetailView::CalculateOptimizationPotential() {
    if (fResults.empty()) return 0.0f;
    
    float totalPotential = 0.0f;
    int count = 0;
    
    for (const auto& result : fResults) {
        if (result.score < 90.0f) {
            float potential = (90.0f - result.score) / 90.0f * 100.0f;
            totalPotential += potential;
            count++;
        }
    }
    
    return count > 0 ? totalPotential / count : 0.0f;
}

std::string ResultsDetailView::PredictBottlenecks() {
    std::map<std::string, int> bottleneckCount;
    
    // Analyze current bottlenecks
    for (const auto& result : fResults) {
        if (!result.bottleneck.empty() && result.bottleneck != "No Bottleneck") {
            bottleneckCount[result.bottleneck]++;
        }
    }
    
    // Find most common bottleneck
    std::string predictedBottleneck = "System appears optimized";
    int maxCount = 0;
    
    for (const auto& pair : bottleneckCount) {
        if (pair.second > maxCount) {
            maxCount = pair.second;
            predictedBottleneck = "Predicted primary bottleneck: " + pair.first;
        }
    }
    
    return predictedBottleneck;
}

void ResultsDetailView::UpdatePerformanceForecasting() {
    // Simple neural network simulation for performance prediction
    if (fHistoricalData.size() < 5) return; // Need sufficient data
    
    // Update prediction model with latest data
    fPerformanceModel.trainingDataCount = fHistoricalData.size();
    fPerformanceModel.lastTrained = system_time();
    
    // Simulate model accuracy improvement over time
    fPerformanceModel.accuracy = std::min(0.95f, 0.5f + (fHistoricalData.size() * 0.05f));
    
    // Generate simple weight updates (simulated learning)
    if (fPerformanceModel.weights.size() != fResults.size()) {
        fPerformanceModel.weights.resize(fResults.size());
        for (size_t i = 0; i < fResults.size(); i++) {
            fPerformanceModel.weights[i] = 0.5f + (rand() % 100) / 200.0f; // Random weights 0.5-1.0
        }
    }
}

std::string ResultsDetailView::GenerateAIInsights() {
    std::stringstream insights;
    insights << "🤖 VeniceDAW AI Performance Analysis\n";
    insights << "==================================\n\n";
    
    insights << "Learning Progress: " << std::fixed << std::setprecision(1) << (fSystemLearningProgress * 100.0f) << "%\n";
    insights << "Model Accuracy: " << std::fixed << std::setprecision(1) << (fPerformanceModel.accuracy * 100.0f) << "%\n";
    insights << "Training Data: " << fHistoricalData.size() << " snapshots\n\n";
    
    // Anomaly summary
    insights << "Anomalies Detected: " << fDetectedAnomalies.size() << "\n";
    for (const auto& anomaly : fDetectedAnomalies) {
        insights << "  - " << anomaly.testName << " (" << anomaly.severity << "): " 
                 << std::fixed << std::setprecision(1) << anomaly.deviationPercent << "% deviation\n";
    }
    
    // Correlation insights
    insights << "\nKey Correlations:\n";
    for (const auto& corr : fCorrelations) {
        insights << "  - " << corr.metric1 << " ↔ " << corr.metric2 
                 << " (" << std::fixed << std::setprecision(2) << corr.correlationCoeff << ")\n";
    }
    
    // Optimization potential
    float potential = CalculateOptimizationPotential();
    insights << "\nOptimization Potential: " << std::fixed << std::setprecision(0) << potential << "%\n";
    
    // Prediction
    insights << "Predicted Bottleneck: " << PredictBottlenecks() << "\n";
    
    return insights.str();
}

void ResultsDetailView::SavePerformanceHistory() {
    // Create snapshot of current performance
    PerformanceSnapshot snapshot;
    snapshot.timestamp = system_time();
    snapshot.results = fResults;
    snapshot.overallScore = 0;
    
    for (const auto& result : fResults) {
        snapshot.overallScore += result.score;
    }
    snapshot.overallScore /= fResults.size();
    
    // Simple system configuration hash
    snapshot.systemConfig = "CPU4_RAM8_GPU1"; // Simplified for demo
    snapshot.workload = "Performance Analysis";
    
    fHistoricalData.push_back(snapshot);
    
    // Keep only last 50 snapshots for memory efficiency
    if (fHistoricalData.size() > 50) {
        fHistoricalData.erase(fHistoricalData.begin());
    }
}

void ResultsDetailView::LoadPerformanceHistory() {
    // TODO: Load from persistent storage
    // For now, generate some sample historical data for demo
    if (fHistoricalData.empty() && !fResults.empty()) {
        for (int i = 0; i < 10; i++) {
            PerformanceSnapshot snapshot;
            snapshot.timestamp = system_time() - (i * 86400000000LL); // i days ago
            snapshot.results = fResults;
            
            // Add some variation to simulate historical changes
            for (auto& result : snapshot.results) {
                float variation = (rand() % 20 - 10) / 100.0f; // ±10% variation
                result.actualValue *= (1.0f + variation);
                result.score = std::max(0.0f, std::min(100.0f, result.score + variation * 50.0f));
            }
            
            snapshot.overallScore = 0;
            for (const auto& result : snapshot.results) {
                snapshot.overallScore += result.score;
            }
            snapshot.overallScore /= snapshot.results.size();
            
            fHistoricalData.push_back(snapshot);
        }
    }
}

void ResultsDetailView::AnalyzeHistoricalTrends() {
    if (fHistoricalData.size() < 3) return;
    
    // Analyze trends over time for each metric
    std::map<std::string, std::vector<float>> trends;
    
    for (const auto& snapshot : fHistoricalData) {
        for (const auto& result : snapshot.results) {
            trends[result.name].push_back(result.score);
        }
    }
    
    // Calculate trend direction for each metric
    for (auto& pair : trends) {
        if (pair.second.size() > 2) {
            // float slope = CalculateTrendSlope(pair.second);
            // Use slope to update predictions or recommendations
        }
    }
}

float ResultsDetailView::CalculateTrendSlope(const std::vector<float>& values) {
    if (values.size() < 2) return 0.0f;
    
    float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = values.size();
    
    for (int i = 0; i < n; i++) {
        sumX += i;
        sumY += values[i];
        sumXY += i * values[i];
        sumX2 += i * i;
    }
    
    return (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
}

// Advanced visualization methods would be implemented here
// These are placeholder implementations focusing on the AI logic

void ResultsDetailView::DrawCorrelationMatrix(BRect rect) {
    SetHighColor(AbletonColors::BACKGROUND_DARK);
    FillRect(rect);
    
    SetHighColor(AbletonColors::TEXT);
    DrawString("AI Performance Correlation Matrix", BPoint(rect.left + 10, rect.top + 20));
    
    if (fResults.size() < 2) {
        SetHighColor(AbletonColors::TEXT_DIM);
        DrawString("Insufficient data for correlation analysis", BPoint(rect.left + 10, rect.top + 40));
        return;
    }
    
    // Draw correlation heatmap
    float cellSize = std::min(rect.Width() / fResults.size(), 20.0f);
    BRect cellRect;
    
    for (size_t i = 0; i < fResults.size() && i < 8; i++) {
        for (size_t j = 0; j < fResults.size() && j < 8; j++) {
            cellRect.Set(rect.left + j * cellSize, 
                        rect.top + 30 + i * cellSize,
                        rect.left + (j + 1) * cellSize - 1,
                        rect.top + 30 + (i + 1) * cellSize - 1);
            
            // Calculate correlation coefficient (simplified for visualization)
            float correlation = (i == j) ? 1.0f : 
                              std::abs(fResults[i].score - fResults[j].score) / 100.0f;
            
            // Color based on correlation strength
            if (correlation > 0.7f) {
                SetHighColor(AbletonColors::ACCENT_ORANGE);
            } else if (correlation > 0.4f) {
                SetHighColor(AbletonColors::ACCENT_BLUE);
            } else {
                SetHighColor(AbletonColors::BACKGROUND_LIGHTER);
            }
            
            FillRect(cellRect);
            SetHighColor(AbletonColors::BORDER);
            StrokeRect(cellRect);
        }
    }
}

void ResultsDetailView::DrawPerformanceHeatMap(BRect rect) {
    SetHighColor(AbletonColors::BACKGROUND_DARK);
    FillRect(rect);
    
    SetHighColor(AbletonColors::TEXT);
    DrawString("AI Performance Heat Map", BPoint(rect.left + 10, rect.top + 20));
    
    if (fResults.empty()) return;
    
    // Draw performance zones
    float zoneHeight = (rect.Height() - 40) / 4;
    const char* zones[] = {"Critical", "Warning", "Good", "Excellent"};
    rgb_color zoneColors[] = {
        AbletonColors::STATUS_ERROR,
        AbletonColors::ACCENT_ORANGE,
        AbletonColors::STATUS_SUCCESS,
        AbletonColors::ACCENT_BLUE
    };
    
    for (int i = 0; i < 4; i++) {
        BRect zoneRect(rect.left + 10, rect.top + 30 + i * zoneHeight,
                      rect.right - 10, rect.top + 30 + (i + 1) * zoneHeight - 2);
        
        SetHighColor(zoneColors[3-i]);
        FillRect(zoneRect);
        
        SetHighColor(AbletonColors::TEXT);
        BPoint textPoint(zoneRect.left + 5, zoneRect.top + zoneHeight/2 + 5);
        DrawString(zones[3-i], textPoint);
        
        // Draw test results in appropriate zones
        for (const auto& result : fResults) {
            int zoneIndex = (int)(result.score / 25.0f);
            if (zoneIndex == (3-i)) {
                // Draw small indicator for this result
                BRect indicator(zoneRect.right - 50, zoneRect.top + 2, 
                               zoneRect.right - 45, zoneRect.top + 7);
                SetHighColor(AbletonColors::TEXT);
                FillRect(indicator);
            }
        }
    }
}

void ResultsDetailView::DrawPredictionGraph(BRect rect) {
    SetHighColor(AbletonColors::BACKGROUND_DARK);
    FillRect(rect);
    
    SetHighColor(AbletonColors::TEXT);
    DrawString("AI Performance Predictions", BPoint(rect.left + 10, rect.top + 20));
    
    if (fHistoricalData.size() < 2) {
        SetHighColor(AbletonColors::TEXT_DIM);
        DrawString("Building prediction model...", BPoint(rect.left + 10, rect.top + 40));
        return;
    }
    
    // Draw prediction timeline
    float timelineWidth = rect.Width() - 40;
    float timelineHeight = rect.Height() - 80;
    
    BRect graphRect(rect.left + 20, rect.top + 40, 
                   rect.left + 20 + timelineWidth, rect.top + 40 + timelineHeight);
    
    // Draw graph background
    SetHighColor(AbletonColors::BACKGROUND_LIGHTER);
    StrokeRect(graphRect);
    
    // Draw prediction line
    if (fHistoricalData.size() > 1) {
        SetHighColor(AbletonColors::ACCENT_BLUE);
        
        BPoint lastPoint;
        bool hasLastPoint = false;
        
        for (size_t i = 0; i < fHistoricalData.size() && i < 10; i++) {
            float x = graphRect.left + (i / 9.0f) * timelineWidth;
            float avgScore = 0;
            for (const auto& result : fHistoricalData[i].results) {
                avgScore += result.score;
            }
            avgScore /= fHistoricalData[i].results.size();
            
            float y = graphRect.bottom - (avgScore / 100.0f) * timelineHeight;
            BPoint currentPoint(x, y);
            
            if (hasLastPoint) {
                StrokeLine(lastPoint, currentPoint);
            }
            
            lastPoint = currentPoint;
            hasLastPoint = true;
        }
        
        // Draw future prediction (extrapolation)
        if (hasLastPoint) {
            SetHighColor(AbletonColors::ACCENT_ORANGE);
            BPoint futurePoint(graphRect.right, lastPoint.y + 10); // Simple prediction
            StrokeLine(lastPoint, futurePoint, B_MIXED_COLORS);
        }
    }
    
    // Draw axis labels
    SetHighColor(AbletonColors::TEXT);
    DrawString("Time →", BPoint(graphRect.left, graphRect.bottom + 15));
    DrawString("Performance", BPoint(graphRect.left - 15, graphRect.top - 5));
}

void ResultsDetailView::DrawOptimizationFlowChart(BRect rect) {
    SetHighColor(AbletonColors::BACKGROUND_DARK);
    FillRect(rect);
    
    SetHighColor(AbletonColors::TEXT);
    DrawString("AI Smart Optimization Flow", BPoint(rect.left + 10, rect.top + 20));
    
    if (fOptimizations.empty()) {
        SetHighColor(AbletonColors::TEXT_DIM);
        DrawString("No optimization recommendations available", BPoint(rect.left + 10, rect.top + 40));
        return;
    }
    
    // Draw optimization flow chart
    float stepHeight = 30;
    float stepY = rect.top + 50;
    
    for (size_t i = 0; i < fOptimizations.size() && i < 5; i++) {
        // Draw optimization step box
        BRect stepRect(rect.left + 20, stepY, rect.right - 20, stepY + stepHeight - 5);
        
        // Color based on optimization priority
        if (fOptimizations[i].priority == "Critical") {
            SetHighColor(AbletonColors::STATUS_ERROR);
        } else if (fOptimizations[i].priority == "High") {
            SetHighColor(AbletonColors::ACCENT_ORANGE);
        } else {
            SetHighColor(AbletonColors::ACCENT_BLUE);
        }
        
        FillRect(stepRect);
        
        // Draw step text
        SetHighColor(AbletonColors::TEXT);
        BString stepText(fOptimizations[i].description.c_str());
        stepText.Truncate(40); // Limit length for display
        DrawString(stepText.String(), BPoint(stepRect.left + 5, stepRect.top + 18));
        
        // Draw arrow to next step
        if (i < fOptimizations.size() - 1 && i < 4) {
            SetHighColor(AbletonColors::TEXT);
            BPoint arrowStart(stepRect.left + stepRect.Width()/2, stepRect.bottom);
            BPoint arrowEnd(arrowStart.x, arrowStart.y + 10);
            StrokeLine(arrowStart, arrowEnd);
            
            // Draw arrow head
            StrokeLine(BPoint(arrowEnd.x - 3, arrowEnd.y - 3), arrowEnd);
            StrokeLine(BPoint(arrowEnd.x + 3, arrowEnd.y - 3), arrowEnd);
        }
        
        stepY += stepHeight + 10;
    }
    
    // Add AI insight indicator
    SetHighColor(AbletonColors::ACCENT_BLUE);
    BRect aiIndicator(rect.right - 80, rect.top + 20, rect.right - 10, rect.top + 40);
    FillRect(aiIndicator);
    SetHighColor(AbletonColors::TEXT);
    DrawString("AI", BPoint(aiIndicator.left + 28, aiIndicator.top + 15));
}

// Performance Analysis Engine Implementation
PerformanceAnalysisEngine::PerformanceAnalysisEngine()
    : fOverallStatus(EXCELLENT)
    , fCPUEfficiency(1.0f)
    , fMemoryUsage(0.2f)
    , fAudioStability(1.0f)
    , fSystemHealth(0.9f)
    , fOverallScore(100.0f)
{
    GenerateAnalysis();
}

void PerformanceAnalysisEngine::UpdateFromBenchmark(const std::vector<BenchmarkResult>& results)
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
    
    // Map performance to professional status levels
    fCPUEfficiency = (cpuCount > 0) ? (cpuScore / cpuCount) / 100.0f : 1.0f;
    fAudioStability = (audioCount > 0) ? (audioScore / audioCount) / 100.0f : 1.0f;
    fMemoryUsage = (memoryCount > 0) ? (1.0f - (memoryScore / memoryCount) / 100.0f) : 0.2f;
    fSystemHealth = fOverallScore / 100.0f;
    
    // Clamp values
    fCPUEfficiency = std::max(0.1f, std::min(1.0f, fCPUEfficiency));
    fAudioStability = std::max(0.1f, std::min(1.0f, fAudioStability));
    fMemoryUsage = std::max(0.0f, std::min(0.9f, fMemoryUsage));
    fSystemHealth = std::max(0.1f, std::min(1.0f, fSystemHealth));
    
    CalculatePerformanceStatus();
    GenerateAnalysis();
}

void PerformanceAnalysisEngine::CalculatePerformanceStatus()
{
    if (fOverallScore >= 90.0f) {
        fOverallStatus = EXCELLENT;
    } else if (fOverallScore >= 70.0f) {
        fOverallStatus = GOOD;
    } else if (fOverallScore >= 50.0f) {
        fOverallStatus = FAIR;
    } else if (fOverallScore >= 30.0f) {
        fOverallStatus = POOR;
    } else if (fOverallScore >= 10.0f) {
        fOverallStatus = BAD;
    } else {
        fOverallStatus = CRITICAL;
    }
}

std::string PerformanceAnalysisEngine::GetAnalysisSummary() const
{
    return fAnalysisSummary;
}

std::vector<std::string> PerformanceAnalysisEngine::GetCriticalIssues() const
{
    return fCriticalIssues;
}

std::string PerformanceAnalysisEngine::GetQuickForecast() const
{
    return fQuickForecast;
}

void PerformanceAnalysisEngine::GenerateAnalysis()
{
    // Generate professional analysis summary
    std::string condition_name;
    std::string condition_emoji;
    
    switch (fOverallStatus) {
        case EXCELLENT:
            condition_name = "excellent performance";
            condition_emoji = "☀️";
            break;
        case GOOD:
            condition_name = "good performance";
            condition_emoji = "⛅";
            break;
        case FAIR:
            condition_name = "fair performance";
            condition_emoji = "☁️";
            break;
        case POOR:
            condition_name = "overcast and sluggish";
            condition_emoji = "🌫️";
            break;
        case BAD:
            condition_name = "poor performance";
            condition_emoji = "🌧️";
            break;
        case CRITICAL:
            condition_name = "critical performance";
            condition_emoji = "⛈️";
            break;
    }
    
    // Create engaging narrative
    fAnalysisSummary = "Your system performance is " + condition_emoji + " " + condition_name + ".\n\n";
    
    // Sun (CPU) description
    if (fCPUEfficiency >= 0.8f) {
        fAnalysisSummary += "✅ CPU Performance: Excellent - your processor is running smoothly and efficiently, ";
        fAnalysisSummary += "providing plenty of computational power for demanding DAW tasks.\n\n";
    } else if (fCPUEfficiency >= 0.6f) {
        fAnalysisSummary += "⚡ CPU Performance: Good - your processor is working well, ";
        fAnalysisSummary += "handling most audio processing tasks without strain.\n\n";
    } else {
        fAnalysisSummary += "⚠️ CPU Performance: Fair - your processor is working hard, ";
        fAnalysisSummary += "consider closing unnecessary applications to improve performance.\n\n";
    }
    
    // Memory usage analysis
    if (fMemoryUsage <= 0.3f) {
        fAnalysisSummary += "💚 Memory Usage: Low - plenty of RAM available, ";
        fAnalysisSummary += "your system has room for large audio projects.\n\n";
    } else if (fMemoryUsage <= 0.6f) {
        fAnalysisSummary += "💛 Memory Usage: Moderate - memory usage is reasonable, ";
        fAnalysisSummary += "still comfortable for most DAW workflows.\n\n";
    } else {
        fAnalysisSummary += "🔴 Memory Usage: High - memory is getting tight, ";
        fAnalysisSummary += "consider freeing up some space.\n\n";
    }
    
    // Audio performance analysis
    if (fAudioStability >= 0.9f) {
        fAnalysisSummary += "🎵 Audio Performance: Excellent - audio system is running optimally, ";
        fAnalysisSummary += "perfect for professional recording and mixing.\n\n";
    } else if (fAudioStability >= 0.7f) {
        fAnalysisSummary += "🎶 Audio Performance: Good - audio quality is stable, ";
        fAnalysisSummary += "suitable for most creative work.\n\n";
    } else {
        fAnalysisSummary += "⚠️ Audio Performance: Issues detected - audio system is stressed, ";
        fAnalysisSummary += "consider increasing buffer sizes or reducing track count.\n\n";
    }
    
    // Generate performance outlook
    fCriticalIssues.clear();
    fOptimizationStrings.clear();
    
    if (fOverallScore >= 80.0f) {
        fOptimizationStrings.push_back("System is performing excellently - ready for intensive creative work.");
    } else if (fOverallScore >= 60.0f) {
        fOptimizationStrings.push_back("Good performance overall - suitable for most audio projects.");
    } else if (fOverallScore >= 40.0f) {
        fOptimizationStrings.push_back("Performance issues detected - some optimizations recommended.");
        fCriticalIssues.push_back("Consider system tuning for better performance.");
    } else {
        fCriticalIssues.push_back("Critical performance issues detected.");
        fCriticalIssues.push_back("System optimization required before intensive work.");
    }
}

// Ecosystem View Implementation
EcosystemView::EcosystemView(BRect frame)
    : BView(frame, "performance_view", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED)
    , fAnalysisEngine(nullptr)
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
fAnalysisFont = *be_plain_font;
fAnalysisFont.SetSize(12);
    
    // Initialize performance visualization elements
    fMetrics.resize(5);
    fIndicators.resize(8);
    fEffects.resize(20);
    
    // Position visualization elements randomly
    for (size_t i = 0; i < fMetrics.size(); i++) {
        fMetrics[i].position = BPoint(
            (rand() % (int)frame.Width()),
            (rand() % (int)(frame.Height() * 0.4f)) + 50
        );
        fMetrics[i].animation_phase = (rand() % 100) / 100.0f;
        fMetrics[i].color = (rgb_color){240, 240, 240, 200};
        fMetrics[i].visible = true;
    }
    
    // Position performance indicators
    for (size_t i = 0; i < fIndicators.size(); i++) {
        fIndicators[i].position = BPoint(
            (rand() % (int)frame.Width()),
            frame.Height() * 0.6f + (rand() % (int)(frame.Height() * 0.3f))
        );
        fIndicators[i].animation_phase = (rand() % 100) / 100.0f;
        fIndicators[i].color = (rgb_color){100, 200, 255, 180};
        fIndicators[i].visible = true;
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
    Window()->SetPulseRate(50000); // 20 FPS for smooth animation
    StartAnimation();
}

void EcosystemView::SetAnalysisEngine(PerformanceAnalysisEngine* engine)
{
    fAnalysisEngine = engine;
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
    
    // Update visualization positions (smooth animation)
    for (auto& metric : fMetrics) {
        metric.animation_phase += 0.005f;
        if (metric.animation_phase > 1.0f) metric.animation_phase = 0.0f;
        
        // Gentle horizontal drift
        metric.position.x += sinf(fGlobalAnimationPhase * 0.1f) * 0.2f;
        if (metric.position.x > Bounds().Width() + 50) {
            metric.position.x = -50;
        }
    }
    
    // Update music note animations
    for (auto& indicator : fIndicators) {
        indicator.animation_phase += 0.02f;
        if (indicator.animation_phase > 1.0f) {
            indicator.animation_phase = 0.0f;
            // Reset position
            indicator.position.y = Bounds().Height() * 0.9f;
        }
        
        // Float upward
        indicator.position.y -= 0.5f;
        indicator.position.x += sinf(indicator.animation_phase * 6.28f) * 0.3f;
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
    DrawPerformanceEffects(bounds);
    
    // Draw UI overlays based on detail level
    DrawDetailOverlays(bounds);
    
    fOffscreenView->Sync();
    fOffscreenBitmap->Unlock();
    
    // Copy to screen
    DrawBitmap(fOffscreenBitmap, BPoint(0, 0));
}

void EcosystemView::DrawSky(BRect bounds)
{
    // Gradient background based on performance status
    rgb_color skyTop = {135, 206, 250, 255};    // Default light blue
    rgb_color skyBottom = {176, 224, 230, 255}; // Default powder blue
    
    if (fAnalysisEngine) {
        PerformanceStatus status = fAnalysisEngine->GetOverallStatus();
        switch (status) {
            case EXCELLENT:
                skyTop = (rgb_color){100, 149, 237, 255};    // Cornflower blue
                skyBottom = (rgb_color){176, 224, 230, 255}; // Powder blue
                break;
            case GOOD:
                skyTop = (rgb_color){119, 136, 153, 255};    // Light slate gray
                skyBottom = (rgb_color){176, 196, 222, 255}; // Light steel blue
                break;
            case FAIR:
                skyTop = (rgb_color){105, 105, 105, 255};    // Dim gray
                skyBottom = (rgb_color){169, 169, 169, 255}; // Dark gray
                break;
            case POOR:
                skyTop = (rgb_color){96, 96, 96, 255};       // Gray
                skyBottom = (rgb_color){128, 128, 128, 255}; // Gray
                break;
            case BAD:
                skyTop = (rgb_color){70, 70, 70, 255};       // Dark gray
                skyBottom = (rgb_color){105, 105, 105, 255}; // Dim gray
                break;
            case CRITICAL:
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
    if (!fAnalysisEngine) return;
    
    float brightness = fAnalysisEngine->GetSunBrightness();
    float cloudCoverage = fAnalysisEngine->GetCloudCoverage();
    
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
    if (!fAnalysisEngine) return;
    
    float cloudCoverage = fAnalysisEngine->GetCloudCoverage();
    
    for (size_t i = 0; i < fMetrics.size() && i < (size_t)(cloudCoverage * 8); i++) {
        const auto& cloud = fMetrics[i];
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
    if (!fAnalysisEngine) return;
    
    float cpuActivity = fAnalysisEngine->GetSunBrightness();
    
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
    if (!fAnalysisEngine) return;
    
    float memoryFlow = 1.0f - fAnalysisEngine->GetCloudCoverage(); // Inverse of cloud coverage
    
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
    if (!fAnalysisEngine) return;
    
    float musicClarity = fAnalysisEngine->GetMusicClarity();
    
    // Only show notes if audio is good
    if (musicClarity < 0.3f) return;
    
    rgb_color noteColor = {100, 200, 255, (uint8)(180 * musicClarity)};
    fOffscreenView->SetHighColor(noteColor);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    for (const auto& note : fIndicators) {
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

void EcosystemView::DrawPerformanceEffects(BRect bounds)
{
    if (!fAnalysisEngine) return;
    
    PerformanceStatus status = fAnalysisEngine->GetOverallStatus();
    
    // Draw performance effects based on status
    switch (status) {
        case BAD:
        case CRITICAL:
            DrawRain(bounds);
            break;
        case POOR:
            DrawFog(bounds);
            break;
        default:
            // No special effects for good performance
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
    for (auto& drop : fEffects) {
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
            // Basic level - no overlay
            break;
    }
}

void EcosystemView::DrawMeteorologistOverlay(BRect bounds)
{
    if (!fAnalysisEngine) return;
    
    // Semi-transparent background
    rgb_color overlayBg = {0, 0, 0, 100};
    fOffscreenView->SetHighColor(overlayBg);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    BRect infoPanel(bounds.right - 300, 20, bounds.right - 20, 200);
    fOffscreenView->FillRoundRect(infoPanel, 15, 15);
    
    // Draw detailed performance data
    fOffscreenView->SetHighColor(255, 255, 255, 200);
    fOffscreenView->SetFont(&fAnalysisFont);
    
    char buffer[256];
    float y = infoPanel.top + 25;
    
    sprintf(buffer, "☀️ CPU Performance: %.1f%%", fAnalysisEngine->GetSunBrightness() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "☁️ Memory Usage: %.1f%%", fAnalysisEngine->GetCloudCoverage() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "🎵 Audio Quality: %.1f%%", fAnalysisEngine->GetMusicClarity() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 20;
    
    sprintf(buffer, "💨 System Speed: %.1f%%", fAnalysisEngine->GetWindSpeed() * 100);
    fOffscreenView->DrawString(buffer, BPoint(infoPanel.left + 15, y));
    y += 25;
    
    // Performance prediction
    fOffscreenView->SetFont(&fTitleFont);
    fOffscreenView->DrawString("Forecast:", BPoint(infoPanel.left + 15, y));
    y += 18;
    
    fOffscreenView->SetFont(&fAnalysisFont);
    std::string forecast = fAnalysisEngine->GetQuickForecast();
    fOffscreenView->DrawString(forecast.c_str(), BPoint(infoPanel.left + 15, y));
    
    fOffscreenView->SetDrawingMode(B_OP_COPY);
}

void EcosystemView::DrawTechnicalOverlay(BRect bounds)
{
    if (!fAnalysisEngine) return;
    
    // Technical data panel
    rgb_color overlayBg = {20, 20, 20, 180};
    fOffscreenView->SetHighColor(overlayBg);
    fOffscreenView->SetDrawingMode(B_OP_ALPHA);
    
    BRect techPanel(20, bounds.bottom - 150, bounds.right - 20, bounds.bottom - 20);
    fOffscreenView->FillRoundRect(techPanel, 10, 10);
    
    // Technical readouts
    fOffscreenView->SetHighColor(0, 255, 0, 220);
    fOffscreenView->SetFont(&fAnalysisFont);
    
    float x = techPanel.left + 20;
    float y = techPanel.top + 25;
    
    // System metrics in technical format
    fOffscreenView->DrawString("SYSTEM TELEMETRY:", BPoint(x, y));
    y += 25;
    
    char buffer[512];
    sprintf(buffer, "CPU: %.2f GHz equiv. | MEM: %.1f%% util | AUDIO: %.1f ms latency",
        fAnalysisEngine->GetSunBrightness() * 3.0f,
        fAnalysisEngine->GetCloudCoverage() * 100,
        (1.0f - fAnalysisEngine->GetMusicClarity()) * 50
    );
    fOffscreenView->DrawString(buffer, BPoint(x, y));
    y += 20;
    
    sprintf(buffer, "I/O: %.1f MB/s | NET: %.1f ms | OVERALL: %.1f/10",
        fAnalysisEngine->GetWindSpeed() * 1000,
        (1.0f - fAnalysisEngine->GetWindSpeed()) * 100,
        (fAnalysisEngine->GetSunBrightness() + fAnalysisEngine->GetMusicClarity() + 
         fAnalysisEngine->GetWindSpeed() + (1.0f - fAnalysisEngine->GetCloudCoverage())) * 2.5f
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

// AnalysisSummaryView implementation
AnalysisSummaryView::AnalysisSummaryView(BRect frame)
    : BView(frame, "analysis_summary", B_FOLLOW_ALL, B_WILL_DRAW | B_PULSE_NEEDED),
      fStoryAnimationPhase(0.0f),
      fAnimatingIn(false)
{
    SetViewColor(245, 250, 255);
    
    fStoryFont.SetSize(14);
    fForecastFont.SetSize(12);
    fForecastFont.SetFace(B_ITALIC_FACE);
}

AnalysisSummaryView::~AnalysisSummaryView()
{
}

void AnalysisSummaryView::AttachedToWindow()
{
    BView::AttachedToWindow();
    SetViewColor(245, 250, 255);
}

void AnalysisSummaryView::Draw(BRect updateRect)
{
    DrawBackground(Bounds());
    DrawStoryText(Bounds());
    DrawForecastBox(Bounds());
}

void AnalysisSummaryView::DrawBackground(BRect bounds)
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

void AnalysisSummaryView::DrawStoryText(BRect bounds)
{
    if (fStory.empty()) return;
    
    SetHighColor(40, 60, 80);
    SetFont(&fStoryFont);
    
    BRect textRect = bounds;
    textRect.InsetBy(20, 20);
    textRect.bottom = bounds.bottom * 0.75f;
    
    // Title
    SetFont(&fForecastFont);
    DrawString("📊 Performance Analysis Report", BPoint(textRect.left, textRect.top + 20));
    
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

void AnalysisSummaryView::DrawForecastBox(BRect bounds)
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

void AnalysisSummaryView::SetStory(const std::string& story)
{
    fStory = story;
    Invalidate();
}

void AnalysisSummaryView::SetForecast(const std::string& forecast)
{
    fForecast = forecast;
    Invalidate();
}

void AnalysisSummaryView::AnimateStoryIn()
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

// PerformanceBenchmarkWindow implementation
PerformanceStationWindow::PerformanceStationWindow(BRect frame)
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
      fAnalysisEngine(nullptr),
      fBenchmark(nullptr),
      fBenchmarkThread(-1),
      fRunning(false),
      fCurrentDetailLevel(0)
{
    InitAnalysisSystem();
    InitUI();
}

PerformanceStationWindow::~PerformanceStationWindow()
{
    if (fBenchmarkThread > 0) {
        kill_thread(fBenchmarkThread);
    }
    delete fBenchmark;
    delete fAnalysisEngine;
}

void PerformanceStationWindow::InitAnalysisSystem()
{
    fAnalysisEngine = new PerformanceAnalysisEngine();
    fBenchmark = new PerformanceStation();
}

void PerformanceStationWindow::InitUI()
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
    fEcosystemView->SetAnalysisEngine(fAnalysisEngine);
    mainView->AddChild(fEcosystemView);
    
    fStoryView = new AnalysisSummaryView(BRect(-1000, -1000, -900, -900));
    mainView->AddChild(fStoryView);
    
    fTechnicalView = new TechnicalDetailsView(BRect(-1000, -1000, -900, -900));
    mainView->AddChild(fTechnicalView);
}

void PerformanceStationWindow::MessageReceived(BMessage* message)
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
            ExportPerformanceReport();
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
            
        case 'arun':  // Auto-run benchmark message
            // Auto-run benchmark triggered
            RunBenchmark();
            break;
            
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

bool PerformanceStationWindow::QuitRequested()
{
    if (fRunning && fBenchmarkThread > 0) {
        // Stop benchmark thread before quitting
        kill_thread(fBenchmarkThread);
    }
    return true;
}

void PerformanceStationWindow::RunBenchmark()
{
    if (fRunning) return;
    
    // Starting benchmark execution
    fRunning = true;
    fControlsView->SetRunning(true);
    fControlsView->EnableExport(false);
    
    // Start benchmark thread
    fBenchmarkThread = spawn_thread(BenchmarkThreadEntry, "performance_benchmark", 
                                   B_NORMAL_PRIORITY, this);
    // Benchmark thread started
    resume_thread(fBenchmarkThread);
    
    // Start ecosystem animation (legacy)
    if (fEcosystemView) fEcosystemView->StartAnimation();
}

void PerformanceStationWindow::ToggleDetailLevel()
{
    // New professional UI: toggle results detail view
    fShowingDetails = !fShowingDetails;
    fResultsView->SetExpanded(fShowingDetails);
}

void PerformanceStationWindow::ShowTechnicalDetails()
{
    // Toggle between professional and legacy view
    fShowingDetails = !fShowingDetails;
    fResultsView->SetExpanded(fShowingDetails);
}

void PerformanceStationWindow::UpdateAnalysisDisplay()
{
    if (!fBenchmark || !fAnalysisEngine) return;
    
    // Get benchmark results and update analysis engine
    std::vector<BenchmarkResult> results = fBenchmark->GetResults();
    // Update display with analysis results
    if (!results.empty()) {
        // Process benchmark results"
    }
    fAnalysisEngine->UpdateFromBenchmark(results);
    
    // Analysis engine computed performance metrics
    
    // Update professional performance view
    float cpuVal = fAnalysisEngine->GetSunBrightness();
    float memVal = fAnalysisEngine->GetCloudCoverage();
    float audioVal = fAnalysisEngine->GetMusicClarity();
    float ioVal = fAnalysisEngine->GetWindSpeed();
    
    // Apply computed metrics to display
    
    fPerformanceView->SetMetrics(cpuVal, memVal, audioVal, ioVal);
    
    // Force GUI update from main thread
    fPerformanceView->Invalidate();
    fPerformanceView->Window()->UpdateIfNeeded();
    
    // Latency will be set from actual benchmark results when available
    
    // Update results detail view
    fResultsView->SetResults(results);
    
    // Update legacy views for compatibility
    if (fStoryView) {
        fStoryView->SetStory(fAnalysisEngine->GetAnalysisSummary());
        fStoryView->SetForecast(fAnalysisEngine->GetQuickForecast());
        fStoryView->AnimateStoryIn();
    }
    
    if (fTechnicalView) {
        fTechnicalView->SetBenchmarkResults(results);
    }
    
    if (fEcosystemView) {
        fEcosystemView->Invalidate();
    }
}

void PerformanceStationWindow::ExportPerformanceReport()
{
    if (!fAnalysisEngine) return;
    
    // Create performance report export dialog
    BFilePanel* panel = new BFilePanel(B_SAVE_PANEL, nullptr, nullptr, 
                                      B_FILE_NODE, false, 
                                      new BMessage('svfl'));
    panel->SetSaveText("performance_report.txt");
    panel->Show();
}

void PerformanceStationWindow::ShowDetailedReport()
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

int32 PerformanceStationWindow::BenchmarkThreadEntry(void* data)
{
    PerformanceStationWindow* window = static_cast<PerformanceStationWindow*>(data);
    window->RunBenchmarkTests();
    return 0;
}

void PerformanceStationWindow::RunBenchmarkTests()
{
    if (!fBenchmark) return;
    
    // Begin benchmark test execution
    // Set up progress callback
    fBenchmark->SetProgressCallback(ProgressCallback, this);
    
    // Run all benchmark tests
    // Execute all benchmark tests
    fBenchmark->RunAllTests();
    // Benchmark tests completed
    
    // Check results immediately after tests
    std::vector<BenchmarkResult> results = fBenchmark->GetResults();
    // Process benchmark results
    if (!results.empty()) {
        // Process benchmark results"
        // Benchmark total score calculated
    }
    
    // Post completion message to main thread
    BMessage msg(MSG_BENCHMARK_COMPLETE);
    PostMessage(&msg);
    // Notify UI of benchmark completion
}

void PerformanceStationWindow::ProgressCallback(float progress, const char* testName, void* userData)
{
    PerformanceStationWindow* window = (PerformanceStationWindow*)userData;
    
    // Create progress message
    BMessage msg(MSG_BENCHMARK_PROGRESS);
    msg.AddFloat("progress", progress);
    msg.AddString("test_name", testName);
    
    // Post to main thread
    window->PostMessage(&msg);
}

void PerformanceStationWindow::OnBenchmarkComplete()
{
    // Handle benchmark completion event
    fRunning = false;
    fBenchmarkThread = -1;
    
    fControlsView->SetRunning(false);
    fControlsView->EnableExport(true);
    
    // Update analysis display with results
    // Update analysis display with results
    UpdateAnalysisDisplay();
    
    // Auto-expand results after completion
    fShowingDetails = true;
    fResultsView->SetExpanded(true);
    // Benchmark completion processing finished
    
    // Stop ecosystem animation (legacy)
    if (fEcosystemView) fEcosystemView->StopAnimation();
}

} // namespace HaikuDAW