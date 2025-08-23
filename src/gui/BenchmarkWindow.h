/*
 * BenchmarkWindow.h - GUI Window for Performance Benchmark
 */

#ifndef BENCHMARK_WINDOW_H
#define BENCHMARK_WINDOW_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <StringView.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StatusBar.h>
#include <TabView.h>
#include <Bitmap.h>
#include <Message.h>
#include <vector>
#include <map>
#include "../benchmark/PerformanceStation.h"

namespace HaikuDAW {

// CPU test worker data structure
struct CPUWorkData {
    int workSize;
    float result;
    int threadId;
};

// Message commands
enum {
    MSG_RUN_ALL_TESTS = 'rall',
    MSG_RUN_AUDIO_TEST = 'raud',
    MSG_RUN_3D_TEST = 'r3dt',
    MSG_RUN_MEMORY_TEST = 'rmem',
    MSG_RUN_SYSTEM_TEST = 'rsys',
    MSG_STOP_TEST = 'stop',
    MSG_EXPORT_RESULTS = 'expr',
    MSG_CLEAR_RESULTS = 'clrr',
    MSG_SHOW_HISTORY = 'hist',
    MSG_TEST_UPDATE = 'tupt',
    MSG_TEST_COMPLETE = 'tcmp'
};

// Custom view for drawing performance graphs
class BenchmarkGraphView : public BView {
public:
    BenchmarkGraphView(BRect frame, const char* name);
    virtual ~BenchmarkGraphView();
    
    virtual void Draw(BRect updateRect) override;
    virtual void AttachedToWindow() override;
    virtual void FrameResized(float width, float height) override;
    
    void SetData(const std::vector<BenchmarkResult>& results);
    void SetCategoryData(const std::map<std::string, float>& scores);
    void UpdateProgress(float progress);
    
private:
    void DrawBarChart(BRect bounds);
    void DrawCategoryPie(BRect bounds);
    void DrawProgressBar(BRect rect, const char* label, float value, float max);
    
    // Modern drawing methods
    void DrawHeader(BRect bounds);
    void DrawPlaceholder(BRect area, const char* text);
    void DrawModernBarChart(BRect area);
    void DrawModernPieChart(BRect area);
    void DrawPerformanceMeter(BRect area);
    void DrawLiveStats(BRect area);
    void DrawModernProgressBar(BRect rect, float percentage);
    rgb_color GetScoreGradientColor(float score);
    rgb_color GetCategoryColor(int index);
    
    std::vector<BenchmarkResult> fResults;
    std::map<std::string, float> fCategoryScores;
    float fProgress;
    BBitmap* fOffscreenBitmap;
    BView* fOffscreenView;
};

// Main benchmark window
class BenchmarkWindow : public BWindow {
public:
    BenchmarkWindow(BRect frame);
    virtual ~BenchmarkWindow();
    
    virtual void MessageReceived(BMessage* message) override;
    virtual bool QuitRequested() override;
    
private:
    void InitUI();
    void RunBenchmark(uint32 testType);
    void UpdateResults();
    void ExportResults();
    void ExportTXT();
    void ExportHTML();
    void ExportCSV();
    void ClearResults();
    void SaveBenchmarkHistory();
    void ShowBenchmarkHistory();
    
    // Benchmark execution methods
    static int32 BenchmarkThreadEntry(void* data);
    void RunBenchmarkTests();
    void RunCategoryBenchmark(const char* category);
    static int32 CategoryBenchmarkThreadEntry(void* data);
    void RunCategoryTests();
    
    // Audio tests
    void TestAudioEngineSimple();
    void TestAudioLatency();
    void TestSineGeneration();
    void TestBufferProcessing();
    
    // Memory tests
    void TestMemoryBandwidth();
    void TestMemoryPatterns();
    
    // CPU/System tests
    void TestCPUScaling();
    void TestRealtimePerformance();
    static int32 CPUWorkerThread(void* data);
    
    // 3D tests
    void Test3DSimulation();
    
    // UI Elements
    BTabView* fTabView;
    BenchmarkGraphView* fGraphView;
    BListView* fResultsList;
    BStatusBar* fProgressBar;
    BStringView* fStatusText;
    BStringView* fSystemInfoView;
    BButton* fRunAllButton;
    BButton* fStopButton;
    BButton* fExportButton;
    BButton* fHistoryButton;
    
    // Benchmark components
    PerformanceStation* fBenchmark;
    thread_id fBenchmarkThread;
    bool fRunning;
    std::string fCurrentCategory;
    
    // Category buttons
    BButton* fAudioButton;
    BButton* f3DButton;
    BButton* fMemoryButton;
    BButton* fSystemButton;
};

// Result detail view
class BenchmarkDetailView : public BView {
public:
    BenchmarkDetailView(BRect frame, const char* name);
    virtual ~BenchmarkDetailView();
    
    virtual void Draw(BRect updateRect) override;
    void SetResult(const BenchmarkResult& result);
    
private:
    void DrawGauge(BPoint center, float radius, float value, float max);
    void DrawSpeedometer(BRect bounds, float score);
    
    BenchmarkResult fResult;
    rgb_color GetScoreColor(float score);
};

} // namespace HaikuDAW

#endif // BENCHMARK_WINDOW_H