/*
 * PerformanceBenchmarkWindow.h - VeniceDAW Performance Station Interface
 */

#ifndef PERFORMANCE_STATION_WINDOW_H
#define PERFORMANCE_STATION_WINDOW_H

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

// Performance status levels
enum PerformanceStatus {
    EXCELLENT = 0,      // Excellent performance (90-100%)
    GOOD,               // Good performance (70-90%)
    FAIR,               // Fair performance (50-70%)
    POOR,               // Poor performance (30-50%)
    BAD,                // Bad performance (10-30%)
    CRITICAL            // Critical performance (<10%)
};

// Performance visualization elements
struct VisualizationElement {
    BPoint position;
    float animation_phase;
    rgb_color color;
    float intensity;
    bool visible;
};

// Performance analysis engine for professional DAW metrics
class PerformanceAnalysisEngine {
public:
    PerformanceAnalysisEngine();
    
    void UpdateFromBenchmark(const std::vector<BenchmarkResult>& results);
    
    PerformanceStatus GetOverallStatus() const { return fOverallStatus; }
    float GetCPUEfficiency() const { return fCPUEfficiency; }
    float GetAudioStability() const { return fAudioStability; }
    float GetSystemHealth() const { return fSystemHealth; }
    float GetOverallScore() const { return fOverallScore; }
    
    std::string GetAnalysisSummary() const;
    std::vector<std::string> GetCriticalIssues() const;
    std::vector<std::string> GetOptimizationSuggestions() const;
    
    // Compatibility methods for existing code (mapped to new metrics)
    float GetSunBrightness() const { return fCPUEfficiency; }
    float GetCloudCoverage() const { return fMemoryUsage; }
    float GetMusicClarity() const { return fAudioStability; }
    float GetWindSpeed() const { return fSystemHealth; }
    std::string GetQuickForecast() const;
    
private:
    PerformanceStatus fOverallStatus;
    float fCPUEfficiency;      // 0.0-1.0 (CPU performance)
    float fMemoryUsage;        // 0.0-1.0 (Memory usage)
    float fAudioStability;     // 0.0-1.0 (Audio quality)
    float fSystemHealth;       // 0.0-1.0 (System responsiveness)
    float fOverallScore;
    
    void CalculatePerformanceStatus();
    void GenerateAnalysis();
    
    std::vector<std::string> fCriticalIssues;
    std::vector<std::string> fOptimizationStrings;
    std::string fAnalysisSummary;
    std::string fQuickForecast;
};

// Ableton-style color constants
namespace AbletonColors {
    const rgb_color BACKGROUND = {28, 28, 28, 255};      // #1C1C1C
    const rgb_color BACKGROUND_DARK = {20, 20, 20, 255}; // #141414 (Darker)
    const rgb_color BACKGROUND_LIGHTER = {36, 36, 36, 255}; // #242424 (Lighter)
    const rgb_color PANEL = {42, 42, 42, 255};           // #2A2A2A  
    const rgb_color BORDER = {64, 64, 64, 255};          // #404040
    const rgb_color TEXT = {200, 200, 200, 255};         // #C8C8C8
    const rgb_color TEXT_DIM = {128, 128, 128, 255};     // #808080 (Dimmed)
    const rgb_color ORANGE = {255, 107, 0, 255};         // #FF6B00 (Active)
    const rgb_color ACCENT_ORANGE = {255, 107, 0, 255};  // #FF6B00 (Active)
    const rgb_color BLUE = {0, 102, 204, 255};           // #0066CC (Selection)
    const rgb_color ACCENT_BLUE = {0, 102, 204, 255};    // #0066CC (Selection)
    const rgb_color GREEN = {0, 204, 102, 255};          // #00CC66 (Good)
    const rgb_color YELLOW = {255, 204, 0, 255};         // #FFCC00 (Warning)
    const rgb_color RED = {204, 0, 0, 255};              // #CC0000 (Critical)
    const rgb_color STATUS_SUCCESS = {0, 204, 102, 255}; // #00CC66 (Success)
    const rgb_color STATUS_ERROR = {204, 0, 0, 255};     // #CC0000 (Error)
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
    void DetectCPUTemperature();
    void DetectMesaVersion();
    void DetectDiskInfo();
    void DetectAudioDriverDetails();
    void DetectGraphicsDriverDetails();
    
    // System information
    int fCPUCores;
    int fTotalRAM;
    BString fCPUType;
    BString fCPUModel;
    BString fHaikuVersion;
    BString fHaikuRevision;
    float fCPUTemperature;
    BString fMesaVersion;
    BString fDiskInfo;
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

// PHASE 2: Smart Layout Engine for Performance Details
struct BarLayout {
    float nameWidth;      // Calculated from longest name
    float barWidth;       // Remaining space for progress bar
    float valueWidth;     // Width for value + unit (e.g., "2.3ms")
    float statusWidth;    // Width for status text
    float totalWidth;     // Total available width
    
    BarLayout() : nameWidth(0), barWidth(0), valueWidth(0), statusWidth(0), totalWidth(0) {}
};

struct CategoryGroup {
    PerformanceCategory category;
    std::vector<BenchmarkResult> results;
    BRect bounds;
    bool expanded;
    float groupScore;
    std::string title;
    
    CategoryGroup(PerformanceCategory cat) : 
        category(cat), expanded(false), groupScore(0.0f) {}
};

// PHASE 5: AI Analytics Structures
struct PerformanceSnapshot {
    bigtime_t timestamp;
    std::vector<BenchmarkResult> results;
    float overallScore;
    std::string systemConfig;      // Hardware configuration hash
    std::string workload;          // Current workload description
};

struct PredictionModel {
    std::vector<float> weights;    // Neural network weights (simplified)
    float bias;
    float accuracy;                // Model accuracy (0.0-1.0)
    bigtime_t lastTrained;         // When model was last updated
    int trainingDataCount;         // Number of samples used for training
    
    PredictionModel() : bias(0.0f), accuracy(0.0f), lastTrained(0), trainingDataCount(0) {}
};

struct PerformanceAnomaly {
    std::string testName;
    float expectedValue;
    float actualValue;
    float deviationPercent;
    std::string severity;          // "minor", "moderate", "critical"
    std::string possibleCause;
    bigtime_t detectedAt;
};

struct OptimizationSuggestion {
    std::string category;
    std::string description;
    std::string priority;          // "Critical", "High", "Medium", "Low", "Info"
    float impactScore;             // Expected improvement (0.0-100.0)
    std::string difficulty;        // "easy", "moderate", "advanced"
    std::vector<std::string> steps;
    bool implemented;
    
    OptimizationSuggestion() : impactScore(0.0f), implemented(false) {}
};

struct PerformanceCorrelation {
    std::string metric1;
    std::string metric2;
    float correlationCoeff;        // -1.0 to 1.0
    float significance;            // Statistical significance
    std::string relationship;      // "positive", "negative", "none"
    
    PerformanceCorrelation() : correlationCoeff(0.0f), significance(0.0f) {}
};

// Professional results display with smart layout
class ResultsDetailView : public BView {
public:
    ResultsDetailView(BRect frame);
    virtual ~ResultsDetailView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void MouseDown(BPoint where) override;
    virtual void MouseMoved(BPoint where, uint32 code, const BMessage* message) override;
    virtual void FrameResized(float width, float height) override;
    
    void SetResults(const std::vector<BenchmarkResult>& results);
    void SetExpanded(bool expanded);
    
private:
    // PHASE 2: Smart Layout Engine
    BarLayout CalculateOptimalLayout(BRect bounds, const std::vector<BenchmarkResult>& results);
    void OrganizeResultsByCategory();
    void DrawCategoryHeader(BRect rect, const CategoryGroup& group);
    void DrawEnhancedResultBar(BRect rect, const BenchmarkResult& result, const BarLayout& layout);
    void DrawTechnicalDetails(BRect rect, const BenchmarkResult& result);
    
    // Enhanced drawing methods
    void DrawProfessionalBar(BRect barRect, float fillRatio, rgb_color color);
    void DrawValueWithUnit(BPoint position, float value, const std::string& unit);
    void DrawDAWStatus(BPoint position, const BenchmarkResult& result);
    void DrawTrendIndicator(BPoint position, const TrendData& trend);
    
    // Layout and interaction
    BRect GetCategoryHeaderRect(int categoryIndex, BRect bounds);
    BRect GetResultBarRect(int categoryIndex, int resultIndex, BRect bounds);
    int GetClickedCategory(BPoint where);
    int GetClickedResult(BPoint where);
    
    // PHASE 3: Interactive Details Methods
    void ShowResultDetails(int resultIndex);
    void HideResultDetails();
    void AnimateDetailPanel(bool show);
    void DrawDetailPanel(BRect bounds);
    void DrawTechnicalMetrics(BRect rect, const BenchmarkResult& result);
    void DrawPerformanceGraph(BRect rect, const TrendData& trend);
    void DrawBottleneckAnalysis(BRect rect, const BenchmarkResult& result);
    void DrawOptimizationSuggestions(BRect rect, const BenchmarkResult& result);
    void DrawSparklineGraph(BRect rect, const std::vector<float>& data, rgb_color color);
    void DrawSystemContext(BRect rect, const BenchmarkResult& result);
    
    // Mouse interaction helpers
    bool IsPointInDetailPanel(BPoint where);
    void HandleDetailPanelClick(BPoint where);
    
    // Animation and timing
    void UpdateDetailAnimation();
    float GetAnimationProgress();
    
    // Enhanced color system
    rgb_color GetDAWStatusColor(const BenchmarkResult& result);
    const char* GetDAWStatusText(const BenchmarkResult& result);
    
    // PHASE 4: Professional Polish & Export Features
    void ShowTooltip(BPoint where, const std::string& text);
    void HideTooltip();
    void UpdateTooltip(BPoint mousePos);
    std::string GetContextualTooltip(BPoint where);
    
    // Export and reporting
    void ExportDetailedReport(const std::string& format);
    void GenerateHTMLReport(const std::string& filename);
    void GenerateCSVReport(const std::string& filename);
    std::string GenerateTextSummary();
    
    // Keyboard shortcuts
    virtual void KeyDown(const char* bytes, int32 numBytes) override;
    void HandleKeyboardShortcut(uint32 key, uint32 modifiers);
    
    // Performance profiles
    void SavePerformanceProfile(const std::string& name);
    void LoadPerformanceProfile(const std::string& name);
    std::vector<std::string> GetAvailableProfiles();
    
    // Accessibility support
    std::string GetAccessibilityDescription(BPoint where);
    void AnnounceStatusChange(const std::string& status);
    
    // PHASE 5: Advanced Analytics & AI-Powered Insights
    void RunPredictiveAnalysis();
    void UpdatePerformanceForecasting();
    std::string GenerateAIInsights();
    void AnalyzePerformancePatterns();
    void DetectAnomalies();
    
    // Smart optimization engine  
    std::vector<OptimizationSuggestion> GenerateSmartRecommendations();
    float CalculateOptimizationPotential();
    std::string PredictBottlenecks();
    
    // Historical analysis
    void SavePerformanceHistory();
    void LoadPerformanceHistory();
    void AnalyzeHistoricalTrends();
    
    // Advanced visualization
    void DrawCorrelationMatrix(BRect rect);
    void DrawPerformanceHeatMap(BRect rect);
    void DrawPredictionGraph(BRect rect);
    void DrawPredictionGraphs(BRect rect);
    void DrawOptimizationFlowChart(BRect rect);
    
    // AI algorithm helpers
    float CalculateCorrelation(const std::vector<float>& x, const std::vector<float>& y);
    float CalculateTrendSlope(const std::vector<float>& values);
    
    // Data management
    std::vector<BenchmarkResult> fResults;
    std::vector<CategoryGroup> fCategoryGroups;
    bool fExpanded;
    BarLayout fCurrentLayout;
    
    // PHASE 3: Interactive Details
    int fSelectedResult;          // Index of currently selected result (-1 = none)
    bool fShowingDetails;         // Whether detail panel is visible
    BRect fDetailPanelRect;       // Bounds of detail panel
    float fDetailPanelHeight;     // Current height of detail panel
    bool fAnimatingDetail;        // Animation state
    bigtime_t fDetailAnimStart;   // Animation start time
    
    // Fonts for professional typography
    BFont fHeaderFont;
    BFont fValueFont;      // Monospace for numbers
    BFont fStatusFont;     // For status indicators
    BFont fDetailFont;     // For detailed technical information
    BFont fMonoFont;       // For technical data display
    
    // PHASE 4: Professional Polish & Export Features
    bool fShowingTooltip;         // Tooltip visibility state
    BRect fTooltipRect;           // Tooltip bounds
    std::string fTooltipText;     // Current tooltip text
    BPoint fLastMousePos;         // Last mouse position for tooltip tracking
    bigtime_t fTooltipShowTime;   // When tooltip was shown
    
    // Export and profiles
    std::string fLastExportPath;  // Last export directory
    std::map<std::string, std::vector<BenchmarkResult>> fSavedProfiles;
    
    // Keyboard state
    uint32 fLastModifiers;        // Last modifier keys state
    
    // Visual polish
    bool fHighlightMode;          // Enhanced visual feedback
    int fHoveredCategory;         // Currently hovered category (-1 = none)
    int fHoveredResult;           // Currently hovered result (-1 = none)
    
    // Accessibility
    std::string fLastAnnouncedStatus; // Last status announced to screen reader
    
    // PHASE 5: Advanced Analytics & AI-Powered Insights
    std::vector<PerformanceSnapshot> fHistoricalData;    // Historical performance data
    PredictionModel fPerformanceModel;                   // AI prediction model
    std::vector<PerformanceAnomaly> fDetectedAnomalies;  // Detected anomalies
    std::vector<OptimizationSuggestion> fOptimizations; // AI-generated suggestions
    std::vector<PerformanceCorrelation> fCorrelations;  // Discovered correlations
    
    // AI analysis state
    bool fAIAnalysisEnabled;          // AI features enabled
    bool fAnalysisInProgress;         // Analysis currently running
    bigtime_t fLastAnalysisTime;      // When analysis was last run
    float fSystemLearningProgress;    // How much the AI has learned (0.0-1.0)
    
    // Advanced visualization state  
    bool fShowingHeatMap;             // Heat map visualization enabled
    bool fShowingCorrelations;        // Correlation matrix visible
    bool fShowingPredictions;         // Prediction graphs visible
    int fVisualizationMode;           // 0=basic, 1=advanced, 2=expert
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
    
    void SetAnalysisEngine(PerformanceAnalysisEngine* engine);
    void SetDetailLevel(int level); // 0=Basic, 1=Advanced, 2=Expert
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
    void DrawPerformanceEffects(BRect bounds);
    void DrawRain(BRect bounds);
    void DrawFog(BRect bounds);
    void DrawDetailOverlays(BRect bounds);
    void DrawMeteorologistOverlay(BRect bounds);
    void DrawTechnicalOverlay(BRect bounds);
    
    void UpdateAnimations();
    BRect GetElementRect(const char* element) const;
    void ShowElementDetails(const char* element);
    
    PerformanceAnalysisEngine* fAnalysisEngine;
    int fDetailLevel;
    bool fAnimating;
    bigtime_t fAnimationStart;
    float fGlobalAnimationPhase;
    
    // Ecosystem elements
    std::vector<VisualizationElement> fMetrics;
    std::vector<VisualizationElement> fIndicators;
    std::vector<VisualizationElement> fEffects;
    BPoint fSunPosition;
    
    // Interaction
    BString fHoveredElement;
    BPoint fLastMousePos;
    
    // Drawing resources
    BBitmap* fOffscreenBitmap;
    BView* fOffscreenView;
    BFont fAnalysisFont;
    BFont fTitleFont;
};

// Analysis summary view for performance insights  
class AnalysisSummaryView : public BView {
public:
    AnalysisSummaryView(BRect frame);
    virtual ~AnalysisSummaryView();
    
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

// Main performance benchmark window
class PerformanceStationWindow : public BWindow {
public:
    PerformanceStationWindow(BRect frame);
    virtual ~PerformanceStationWindow();
    
    virtual void MessageReceived(BMessage* message) override;
    virtual bool QuitRequested() override;
    
private:
    void InitUI();
    void InitAnalysisSystem();
    void RunBenchmark();
    void UpdateAnalysisDisplay();
    void ToggleDetailLevel();
    void ShowTechnicalDetails();
    void ExportPerformanceReport();
    void ShowDetailedReport();
    
    // UI Components - Professional Layout
    PerformanceMeterView* fPerformanceView;
    BenchmarkControlsView* fControlsView;
    ResultsDetailView* fResultsView;
    PCInfoView* fPCInfoView;
    
    // Analysis components
    EcosystemView* fEcosystemView;
    AnalysisSummaryView* fStoryView;
    TechnicalDetailsView* fTechnicalView;
    
    BGroupLayout* fMainLayout;
    bool fShowingDetails;
    
    // Analysis system
    PerformanceAnalysisEngine* fAnalysisEngine;
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

#endif // PERFORMANCE_STATION_WINDOW_H