/*
 * 3DMixProjectImporter.h - Complete 3dmix project import into VeniceDAW
 * Integrates all 3dmix components with VeniceDAW's audio engine and 3D mixer
 */

#ifndef THREEDMIX_PROJECT_IMPORTER_H
#define THREEDMIX_PROJECT_IMPORTER_H

#include "3DMixFormat.h"
#include "3DMixParser.h"
#include "CoordinateSystemMapper.h"
#include "AudioPathResolver.h"
#include "../AudioLogging.h"
#ifdef __HAIKU__
	#include <support/String.h>
	#include <Window.h>
	#include <Message.h>
#else
	// Cross-platform headers for syntax checking
	#include "../../testing/HaikuMockHeaders.h"
#endif
#include <vector>

// Custom status code for partial operations
#ifndef B_PARTIAL_ERROR
#define B_PARTIAL_ERROR (B_GENERAL_ERROR_BASE + 0x1001)
#endif

// Forward declarations for VeniceDAW integration
namespace VeniceDAW {
	class MixerWindow;
	class Mixer3DWindow;
	class TSoundView;
	class SimpleHaikuEngine;
}

namespace VeniceDAW {

/*
 * Import operation configuration
 */
struct ImportConfiguration {
	// File handling
	bool resolveAudioPaths;				// Attempt to find missing audio files
	bool convertRawAudio;				// Convert RAW files to WAV
	bool copyAudioToProject;			// Copy audio files to project directory
	bool createProjectDirectory;		// Create dedicated project directory

	// Coordinate conversion
	coordinate_conversion_mode coordMode;	// Coordinate conversion strategy
	spatialization_standard spatialStd;	// Target spatialization standard
	bool optimizeForBinaural;			// Optimize positions for HRTF
	bool preserveOriginalPositions;		// Keep original BeOS coordinates as backup

	// Audio processing
	bool normalizeAudioLevels;			// Normalize track volumes
	bool resampleToProjectRate;			// Resample all audio to project sample rate
	bool enableLooping;					// Preserve loop regions
	bool enableEffects;					// Import effect parameters

	// Integration
	bool openIn3DMixer;					// Open result in 3D mixer window
	bool addToCurrentProject;			// Add to existing project vs. new project
	bool updateExistingTracks;			// Update existing tracks if name matches

	ImportConfiguration()
		: resolveAudioPaths(true)
		, convertRawAudio(true)
		, copyAudioToProject(false)
		, createProjectDirectory(false)
		, coordMode(CONVERSION_SPHERICAL)
		, spatialStd(STANDARD_GENERIC_3D)
		, optimizeForBinaural(false)
		, preserveOriginalPositions(true)
		, normalizeAudioLevels(false)
		, resampleToProjectRate(false)
		, enableLooping(true)
		, enableEffects(true)
		, openIn3DMixer(true)
		, addToCurrentProject(false)
		, updateExistingTracks(false)
	{}
};

/*
 * Import operation result
 */
struct ImportResult {
	bool success;						// Overall import success
	BString projectName;				// Imported project name
	BString projectPath;				// Project file path
	int32 tracksImported;				// Number of tracks successfully imported
	int32 tracksSkipped;				// Number of tracks skipped
	int32 audioFilesResolved;			// Number of audio files found
	int32 audioFilesConverted;			// Number of audio files converted
	bigtime_t importTime;				// Total import time
	BString errorMessage;				// Error message if failed
	std::vector<ValidationResult> validationResults;	// Validation issues

	ImportResult()
		: success(false), tracksImported(0), tracksSkipped(0)
		, audioFilesResolved(0), audioFilesConverted(0), importTime(0)
	{}
};

/*
 * VeniceDAW track integration information
 */
struct VeniceTrackMapping {
	int32 originalTrackIndex;			// Index in 3dmix project
	int32 veniceTrackIndex;				// Index in VeniceDAW project
	BString trackName;					// Track name
	BString audioFilePath;				// Final audio file path
	AudioSphericalCoordinate position;	// Final 3D position
	bool wasCreated;					// True if new track was created
	bool wasUpdated;					// True if existing track was updated

	VeniceTrackMapping()
		: originalTrackIndex(-1), veniceTrackIndex(-1)
		, wasCreated(false), wasUpdated(false)
	{}
};

/*
 * Complete 3dmix project importer for VeniceDAW
 */
class ThreeDMixProjectImporter {
public:
	ThreeDMixProjectImporter();
	~ThreeDMixProjectImporter();

	// Configuration
	void SetImportConfiguration(const ImportConfiguration& config) { fConfig = config; }
	const ImportConfiguration& GetConfiguration() const { return fConfig; }

	// Main import interface
	ImportResult ImportProject(const char* filePath);
	ImportResult ImportProject(const char* filePath, const ImportConfiguration& config);

	// Import to specific VeniceDAW components
	ImportResult ImportToMixer(const char* filePath, MixerWindow* mixerWindow);
	ImportResult ImportTo3DMixer(const char* filePath, Mixer3DWindow* mixer3D);
	ImportResult ImportToEngine(const char* filePath, SimpleHaikuEngine* engine);

	// Advanced import modes
	ImportResult MergeWithCurrentProject(const char* filePath);
	ImportResult ImportAsNewProject(const char* filePath, const char* projectDirectory);
	ImportResult ImportSelectedTracks(const char* filePath, const std::vector<int32>& trackIndices);

	// Interactive import workflow
	bool ShowImportDialog(const char* filePath, ImportConfiguration* config);
	std::vector<BString> PreviewImport(const char* filePath);
	bool ConfirmOverwriteExisting(const std::vector<BString>& conflictingTracks);

	// Track mapping and integration
	std::vector<VeniceTrackMapping> GetTrackMappings() const { return fTrackMappings; }
	VeniceTrackMapping GetTrackMapping(int32 originalIndex) const;
	bool UpdateTrackMapping(int32 originalIndex, const VeniceTrackMapping& mapping);

	// Progress monitoring
	typedef void (*ImportProgressCallback)(const char* operation, float progress, void* userData);
	void SetProgressCallback(ImportProgressCallback callback, void* userData);

	// Error handling and validation
	const ImportResult& GetLastResult() const { return fLastResult; }
	std::vector<ValidationResult> ValidateBeforeImport(const char* filePath);
	bool HasUnresolvedIssues() const;

	// Project analysis
	struct ProjectAnalysis {
		BString projectName;
		int32 trackCount;
		float totalDuration;
		off_t totalAudioSize;
		int32 missingFiles;
		BString formatSummary;
		std::vector<BString> requiredFeatures;
	};
	ProjectAnalysis AnalyzeProject(const char* filePath);

	// Cleanup and resource management
	void CleanupTemporaryFiles();
	void ResetImporter();

private:
	// Core import pipeline
	ImportResult ExecuteImportPipeline(const char* filePath);
	status_t LoadLegacyProject(const char* filePath, Project3DMix* project);
	status_t ResolveAudioFiles(Project3DMix* project);
	status_t ConvertCoordinates(Project3DMix* project);
	status_t ProcessAudioFiles(Project3DMix* project);
	status_t IntegrateWithVeniceDAW(Project3DMix* project);

	// VeniceDAW integration helpers
	status_t CreateVeniceDAWTracks(const Project3DMix& project);
	status_t UpdateExistingTracks(const Project3DMix& project);
	status_t Setup3DPositions(const Project3DMix& project);
	status_t ConfigureAudioEngine(const Project3DMix& project);

	// Track creation and management
	int32 CreateNewTrack(const Track3DMix& legacyTrack);
	bool UpdateExistingTrack(int32 veniceIndex, const Track3DMix& legacyTrack);
	bool FindMatchingTrack(const BString& trackName, int32* veniceIndex);

	// Audio file processing
	status_t ProcessTrackAudio(Track3DMix* track);
	status_t ConvertRawAudioFile(const BString& rawPath, const BString& wavPath,
	                            const AudioFormatDetection& format);
	status_t NormalizeAudioLevel(const BString& filePath);
	status_t ResampleAudioFile(const BString& filePath, int32 targetSampleRate);

	// 3D positioning integration
	status_t Apply3DPosition(int32 veniceTrackIndex, const AudioSphericalCoordinate& position);
	status_t UpdateMixer3DVisualization();
	status_t SetupSpatialAudioProcessing();

	// Project directory management
	status_t CreateProjectDirectory(const BString& projectName, BString* projectPath);
	status_t CopyAudioFilesToProject(Project3DMix* project, const BString& projectPath);
	status_t CreateProjectFile(const Project3DMix& project, const BString& projectPath);

	// Validation and error handling
	status_t ValidateImportRequirements();
	status_t ValidateLegacyProject(const Project3DMix& project);
	status_t ValidateAudioFiles(const Project3DMix& project);
	status_t ValidateVeniceDAWIntegration();

	// Progress reporting
	void ReportProgress(const char* operation, float progress);
	void ReportError(const char* error);
	void ReportWarning(const char* warning);

	// Utility functions
	BString GenerateUniqueTrackName(const BString& baseName);
	BString GenerateProjectDirectory(const BString& projectName);
	bool IsTrackNameTaken(const BString& trackName);

	// Component instances
	Legacy3DMixLoader fLoader;
	CoordinateSystemMapper fCoordinateMapper;
	AudioPathResolver fPathResolver;
	AudioFormatConverter fFormatConverter;

	// Configuration and state
	ImportConfiguration fConfig;
	ImportResult fLastResult;
	std::vector<VeniceTrackMapping> fTrackMappings;
	std::vector<BString> fTemporaryFiles;

	// VeniceDAW integration targets
	MixerWindow* fTargetMixer;
	Mixer3DWindow* fTarget3DMixer;
	SimpleHaikuEngine* fTargetEngine;
	TSoundView* fTargetSoundView;

	// Progress callback
	ImportProgressCallback fProgressCallback;
	void* fProgressUserData;

	// Internal statistics
	struct ImportStatistics {
		bigtime_t parseTime;
		bigtime_t resolveTime;
		bigtime_t convertTime;
		bigtime_t integrationTime;
		int32 totalOperations;
		int32 completedOperations;
	};
	ImportStatistics fStats;
};

// Forward declarations for UI classes (implemented in separate files)
class ThreeDMixImportDialog;
class ThreeDMixImportProgress;

/*
 * Utility functions for 3dmix integration
 */
class ThreeDMixIntegrationUtils {
public:
	// File format detection
	static bool IsThreeDMixFile(const char* filePath);
	static bool IsThreeDMixPointerFile(const char* filePath);
	static BString ResolvePointerFile(const char* pointerPath);

	// VeniceDAW compatibility
	static bool IsVeniceDAWRunning();
	static MixerWindow* GetActiveMixerWindow();
	static Mixer3DWindow* GetActive3DMixerWindow();
	static SimpleHaikuEngine* GetAudioEngine();

	// Project management
	static BString GetDefaultProjectsDirectory();
	static BString GenerateProjectName(const char* sourceFile);
	static bool CreateProjectBackup(const char* projectPath);

	// Audio format support
	static std::vector<BString> GetSupportedAudioFormats();
	static bool CanConvertAudioFormat(const BString& filePath);
	static BString GetPreferredAudioFormat();

	// System integration
	static bool RegisterFileType();
	static bool AddToRecentFiles(const char* filePath);
	static bool ShowInTracker(const char* filePath);
};

} // namespace VeniceDAW

#endif // THREEDMIX_PROJECT_IMPORTER_H