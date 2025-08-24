/*
 * WeatherBenchmarkWindow.h - VeniceDAW Performance Station Interface
 */

#ifndef WEATHER_BENCHMARK_WINDOW_H
#define WEATHER_BENCHMARK_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <StatusBar.h>
#include <GroupLayout.h>
#include <CardLayout.h>
#include <Bitmap.h>
#include <Font.h>
#include <vector>
#include <map>
#include <string>
#include "../benchmark/PerformanceStation.h"

namespace HaikuDAW {

// Weather conditions based on performance
enum WeatherCondition {
    SUNNY = 0,          // Excellent performance (90-100%)
    PARTLY_CLOUDY,      // Good performance (70-90%)
    CLOUDY,             // Fair performance (50-70%)
    OVERCAST,           // Poor performance (30-50%)
    RAINY,              // Bad performance (10-30%)
    STORMY              // Critical performance (<10%)
};

// Performance ecosystem elements
struct EcosystemElement {
    BPoint position;
    float animation_phase;
    rgb_color color;
    float intensity;
    bool visible;
};

// Weather-based performance metaphors
class WeatherMetaphorEngine {
public:
    WeatherMetaphorEngine();
    
    void UpdateFromBenchmark(const std::vector<BenchmarkResult>& results);
    
    WeatherCondition GetOverallWeather() const { return fOverallCondition; }
    float GetSunBrightness() const { return fSunBrightness; }
    float GetCloudCoverage() const { return fCloudCoverage; }
    float GetMusicClarity() const { return fMusicClarity; }
    float GetWindSpeed() const { return fWindSpeed; }
    
    std::string GetWeatherStory() const;
    std::string GetQuickForecast() const;
    
private:
    WeatherCondition fOverallCondition;
    float fSunBrightness;      // 0.0-1.0 (CPU performance)
    float fCloudCoverage;      // 0.0-1.0 (Memory usage)
    float fMusicClarity;       // 0.0-1.0 (Audio quality)
    float fWindSpeed;          // 0.0-1.0 (System responsiveness)
    float fOverallScore;
    
    void CalculateWeatherCondition();
    void GenerateStory();
    
    std::string fWeatherStory;
    std::string fQuickForecast;
};

// Ableton-style color constants
namespace AbletonColors {
    const rgb_color BACKGROUND = {28, 28, 28, 255};      // #1C1C1C
    const rgb_color PANEL = {42, 42, 42, 255};           // #2A2A2A  
    const rgb_color BORDER = {64, 64, 64, 255};          // #404040
    const rgb_color TEXT = {200, 200, 200, 255};         // #C8C8C8
    const rgb_color ORANGE = {255, 107, 0, 255};         // #FF6B00 (Active)
    const rgb_color BLUE = {0, 102, 204, 255};           // #0066CC (Selection)
    const rgb_color GREEN = {0, 204, 102, 255};          // #00CC66 (Good)
    const rgb_color YELLOW = {255, 204, 0, 255};         // #FFCC00 (Warning)
    const rgb_color RED = {204, 0, 0, 255};              // #CC0000 (Critical)
}

// Professional performance meter view
class PerformanceMeterView : public BView {
public:
    PerformanceMeterView(BRect frame);
    virtual ~PerformanceMeterView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    
    void SetMetrics(float cpu, float memory, float audio, float io);
    void SetLatency(float latencyMs);
    void SetProgress(float progress, const char* currentTest = nullptr);
    
private:
    void DrawMeter(BRect rect, const char* label, float value, rgb_color color, const char* status);
    void DrawStatusOverview(BRect bounds);
    void DrawProgressBar(BRect bounds);
    
    // Driver detection methods
    void DetectAudioDriver();
    void DetectGraphicsDriver();
    void DetectCPUInfo();
    
    float fCPU, fMemory, fAudio, fIO;
    float fLatency;
    float fProgress;
    BString fCurrentTest;
    
    // System information
    int fCPUCores;
    int fTotalRAM;
    BString fCPUType;
    BString fCPUModel;
    BString fHaikuVersion;
    BString fAudioDriver;
    BString fGraphicsDriver;
};

// PC Information display panel
class PCInfoView : public BView {
public:
    PCInfoView(BRect frame);
    virtual ~PCInfoView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    
    void UpdateSystemInfo();
    
private:
    void DrawSystemInfo(BRect bounds);
    void DetectSystemInfo();
    void DetectAudioDriverDetails();
    void DetectGraphicsDriverDetails();
    
    // System information
    int fCPUCores;
    int fTotalRAM;
    BString fCPUType;
    BString fCPUModel;
    BString fHaikuVersion;
    BString fHaikuRevision;
    BString fAudioDriver;
    BString fAudioDevice;
    BString fGraphicsDriver;
    BString fGraphicsCard;
    BString fNetworkAdapter;
    BString fStorageInfo;
    float fLatency;
};

// Professional benchmark controls panel  
class BenchmarkControlsView : public BView {
public:
    BenchmarkControlsView(BRect frame);
    virtual ~BenchmarkControlsView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    
    void SetRunning(bool running);
    void EnableExport(bool enabled);
    
private:
    BButton* fRunFullButton;
    BButton* fQuickTestButton; 
    BButton* fDetailsButton;
    BButton* fExportButton;
    BButton* fDetailedReportButton;
};

// Professional results display
class ResultsDetailView : public BView {
public:
    ResultsDetailView(BRect frame);
    virtual ~ResultsDetailView();
    
    virtual void Draw(BRect updateRect) override;
    
    void SetResults(const std::vector<BenchmarkResult>& results);
    void SetExpanded(bool expanded);
    
private:
    void DrawResultBar(BRect rect, const std::string& testName, float score, const char* status);
    rgb_color GetStatusColor(float score);
    const char* GetStatusText(float score);
    
    std::vector<BenchmarkResult> fResults;
    bool fExpanded;
};

// Legacy view - keeping for compatibility but will be redesigned
class EcosystemView : public BView {
public:
    EcosystemView(BRect frame);
    virtual ~EcosystemView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    virtual void MouseDown(BPoint where) override;
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* message) override;
    virtual void FrameResized(float width, float height) override;
    virtual void Pulse() override;
    
    void SetWeatherEngine(WeatherMetaphorEngine* engine);
    void SetDetailLevel(int level); // 0=Weather, 1=Meteorologist, 2=Technical
    void StartAnimation();
    void StopAnimation();
    
private:
    void DrawSky(BRect bounds);
    void DrawSun(BRect bounds);
    void DrawClouds(BRect bounds);
    void DrawMountains(BRect bounds);
    void DrawCity(BRect bounds);
    void DrawRiver(BRect bounds);
    void DrawMusicNotes(BRect bounds);
    void DrawWeatherEffects(BRect bounds);
    void DrawRain(BRect bounds);
    void DrawFog(BRect bounds);
    void DrawDetailOverlays(BRect bounds);
    void DrawMeteorologistOverlay(BRect bounds);
    void DrawTechnicalOverlay(BRect bounds);
    
    void UpdateAnimations();
    BRect GetElementRect(const char* element) const;
    void ShowElementDetails(const char* element);
    
    WeatherMetaphorEngine* fWeatherEngine;
    int fDetailLevel;
    bool fAnimating;
    bigtime_t fAnimationStart;
    float fGlobalAnimationPhase;
    
    // Ecosystem elements
    std::vector<EcosystemElement> fClouds;
    std::vector<EcosystemElement> fMusicNotes;
    std::vector<EcosystemElement> fRaindrops;
    BPoint fSunPosition;
    
    // Interaction
    BString fHoveredElement;
    BPoint fLastMousePos;
    
    // Drawing resources
    BBitmap* fOffscreenBitmap;
    BView* fOffscreenView;
    BFont fWeatherFont;
    BFont fTitleFont;
};

// Story display view for weather narratives
class WeatherStoryView : public BView {
public:
    WeatherStoryView(BRect frame);
    virtual ~WeatherStoryView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    
    void SetStory(const std::string& story);
    void SetForecast(const std::string& forecast);
    void AnimateStoryIn();
    
private:
    void DrawBackground(BRect bounds);
    void DrawStoryText(BRect bounds);
    void DrawForecastBox(BRect bounds);
    
    std::string fStory;
    std::string fForecast;
    float fStoryAnimationPhase;
    bool fAnimatingIn;
    
    BFont fStoryFont;
    BFont fForecastFont;
};

// Technical details panel (expandable)
class TechnicalDetailsView : public BView {
public:
    TechnicalDetailsView(BRect frame);
    virtual ~TechnicalDetailsView();
    
    virtual void Draw(BRect updateRect) override;
    
    void SetBenchmarkResults(const std::vector<BenchmarkResult>& results);
    void SetExpanded(bool expanded);
    void AnimateExpansion(bool expand);
    
private:
    void DrawTechnicalData(BRect bounds);
    void DrawPerformanceGraphs(BRect bounds);
    
    std::vector<BenchmarkResult> fResults;
    bool fExpanded;
    float fExpansionPhase;
    
    BFont fTechnicalFont;
    BFont fHeaderFont;
};

// Main weather benchmark window
class WeatherBenchmarkWindow : public BWindow {
public:
    WeatherBenchmarkWindow(BRect frame);
    virtual ~WeatherBenchmarkWindow();
    
    virtual void MessageReceived(BMessage* message) override;
    virtual bool QuitRequested() override;
    
private:
    void InitUI();
    void InitWeatherSystem();
    void RunBenchmark();
    void UpdateWeatherDisplay();
    void ToggleDetailLevel();
    void ShowTechnicalDetails();
    void ExportWeatherReport();
    void ShowDetailedReport();
    
    // UI Components - Professional Layout
    PerformanceMeterView* fPerformanceView;
    BenchmarkControlsView* fControlsView;
    ResultsDetailView* fResultsView;
    PCInfoView* fPCInfoView;
    
    // Legacy components (will be phased out)
    EcosystemView* fEcosystemView;
    WeatherStoryView* fStoryView;
    TechnicalDetailsView* fTechnicalView;
    
    BGroupLayout* fMainLayout;
    bool fShowingDetails;
    
    // Weather system
    WeatherMetaphorEngine* fWeatherEngine;
    PerformanceStation* fBenchmark;
    thread_id fBenchmarkThread;
    bool fRunning;
    int fCurrentDetailLevel;
    
    // Benchmark execution
    static int32 BenchmarkThreadEntry(void* data);
    void RunBenchmarkTests();
    void OnBenchmarkComplete();
    
    // Progress callback
    static void ProgressCallback(float progress, const char* testName, void* userData);
};

// Message constants
enum {
    MSG_RUN_WEATHER_BENCHMARK = 'rwbm',
    MSG_TOGGLE_DETAIL_LEVEL = 'tdlv',
    MSG_SHOW_TECHNICAL = 'shtc',
    MSG_EXPORT_WEATHER_REPORT = 'ewrp',
    MSG_DETAILED_REPORT = 'drpt',
    MSG_ECOSYSTEM_CLICKED = 'eclk',
    MSG_BENCHMARK_PROGRESS = 'bprg',
    MSG_BENCHMARK_COMPLETE = 'bcmp'
};

} // namespace HaikuDAW

#endif // WEATHER_BENCHMARK_WINDOW_H