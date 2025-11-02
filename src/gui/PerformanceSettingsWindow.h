/*
 * PerformanceSettingsWindow.h - Performance quality settings for VeniceDAW
 * Allows users to optimize rendering for their system capabilities
 */

#ifndef PERFORMANCE_SETTINGS_WINDOW_H
#define PERFORMANCE_SETTINGS_WINDOW_H

#include <interface/Window.h>
#include <interface/View.h>
#include <interface/Button.h>
#include <interface/RadioButton.h>
#include <interface/CheckBox.h>
#include <interface/StringView.h>
#include <interface/GroupLayout.h>
#include <interface/LayoutBuilder.h>
#include <support/String.h>

namespace HaikuDAW {

// Forward declaration
class Mixer3DView;

/*
 * Performance quality levels for rendering optimizations
 */
enum class PerformanceQuality {
    LOW,      // Minimum quality for legacy systems (512MB RAM, Pentium 4)
    MEDIUM,   // Balanced quality for mid-range systems (2GB RAM, Core 2 Duo)
    HIGH,     // Maximum quality for modern systems (4GB+ RAM, Core i3+)
    CUSTOM    // User-defined custom settings
};

/*
 * Performance settings configuration
 * Controls all rendering optimizations dynamically
 */
struct PerformanceSettings {
    PerformanceQuality quality;

    // Waveform rendering settings
    bool enableWaveformCache;       // BBitmap caching for timeline
    int waveformQualityLevel;       // 0=blocks only, 1=medium, 2=full detail

    // 3D rendering settings
    bool enableParticles;           // Audio particle system
    bool enableGlow;                // Glow halos around tracks
    bool enableShadows;             // Drop shadows (future)
    int maxVisibleTracks;           // Frustum culling limit

    // LOD settings
    float lodDistanceClose;         // Distance threshold for LOD 0->1 (default: 15.0)
    float lodDistanceFar;           // Distance threshold for LOD 1->2 (default: 25.0)

    // Frame rate target
    int targetFPS;                  // 15, 30, or 60 FPS

    // Constructor with defaults (HIGH quality)
    PerformanceSettings()
        : quality(PerformanceQuality::HIGH)
        , enableWaveformCache(true)
        , waveformQualityLevel(2)
        , enableParticles(true)
        , enableGlow(true)
        , enableShadows(false)
        , maxVisibleTracks(100)
        , lodDistanceClose(15.0f)
        , lodDistanceFar(25.0f)
        , targetFPS(60)
    {}

    // Preset factory methods
    static PerformanceSettings CreateLowQuality();
    static PerformanceSettings CreateMediumQuality();
    static PerformanceSettings CreateHighQuality();
};

/*
 * Performance Settings Window
 * Modal dialog for adjusting rendering quality
 */
class PerformanceSettingsWindow : public BWindow {
public:
    PerformanceSettingsWindow(Mixer3DView* targetView);
    virtual ~PerformanceSettingsWindow();

    // BWindow interface
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

private:
    void CreateUI();
    void ApplySettings();
    void LoadCurrentSettings();
    void UpdateUIFromSettings();

    // Auto-detection
    void DetectSystemCapabilities();
    BString GetSystemInfo();

    Mixer3DView* fTargetView;
    PerformanceSettings fSettings;

    // UI components
    BRadioButton* fLowQualityRadio;
    BRadioButton* fMediumQualityRadio;
    BRadioButton* fHighQualityRadio;
    BRadioButton* fCustomQualityRadio;

    BCheckBox* fWaveformCacheCheck;
    BCheckBox* fParticlesCheck;
    BCheckBox* fGlowCheck;

    BStringView* fSystemInfoView;
    BStringView* fRecommendationView;

    BButton* fApplyButton;
    BButton* fCancelButton;
    BButton* fDetectButton;

    // Message constants
    enum {
        MSG_QUALITY_LOW = 'qlow',
        MSG_QUALITY_MEDIUM = 'qmed',
        MSG_QUALITY_HIGH = 'qhgh',
        MSG_QUALITY_CUSTOM = 'qcst',
        MSG_TOGGLE_CACHE = 'tcch',
        MSG_TOGGLE_PARTICLES = 'tpar',
        MSG_TOGGLE_GLOW = 'tglw',
        MSG_APPLY = 'aply',
        MSG_CANCEL = 'cncl',
        MSG_DETECT = 'dtct'
    };
};

} // namespace HaikuDAW

#endif // PERFORMANCE_SETTINGS_WINDOW_H
