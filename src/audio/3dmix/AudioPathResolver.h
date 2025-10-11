/*
 * AudioPathResolver.h - Intelligent audio file path resolution for 3dmix import
 * Handles BeOS→Haiku path translation and missing file recovery
 */

#ifndef AUDIO_PATH_RESOLVER_H
#define AUDIO_PATH_RESOLVER_H

#include "3DMixFormat.h"
#ifdef __HAIKU__
	#include <storage/Entry.h>
	#include <storage/Directory.h>
	#include <storage/File.h>
	#include <storage/Path.h>
	#include <support/List.h>
#else
	// Cross-platform headers for syntax checking
	#include "../../testing/HaikuMockHeaders.h"
#endif
#include <vector>
#include <map>

namespace VeniceDAW {

/*
 * File search strategies for missing audio files
 */
enum file_search_strategy {
	SEARCH_EXACT_PATH = 0,			// Only check exact path
	SEARCH_FILENAME_ONLY,			// Search by filename in common directories
	SEARCH_FUZZY_MATCHING,			// Fuzzy filename matching
	SEARCH_CONTENT_ANALYSIS,		// Audio content fingerprinting
	SEARCH_USER_GUIDED,				// Interactive user selection
	SEARCH_COMPREHENSIVE			// All strategies combined
};

/*
 * Audio file resolution result
 */
struct AudioFileResolution {
	BString originalPath;			// Original BeOS path
	BString resolvedPath;			// Found Haiku path
	BString alternativePath;		// Alternative if exact match not found
	bool wasFound;					// True if file was located
	bool requiresConversion;		// True if format conversion needed
	float confidenceScore;			// 0.0-1.0 match confidence
	BString searchMethod;			// How the file was found

	AudioFileResolution()
		: wasFound(false), requiresConversion(false), confidenceScore(0.0f) {}
};

/*
 * BeOS to Haiku path translation patterns
 */
struct PathTranslationRule {
	BString beosPattern;			// BeOS path pattern to match
	BString haikuReplacement;		// Haiku replacement pattern
	bool isRegexPattern;			// True if using regex matching
	int32 priority;					// Higher priority rules checked first

	PathTranslationRule(const char* pattern, const char* replacement, int32 prio = 0)
		: beosPattern(pattern), haikuReplacement(replacement), isRegexPattern(false), priority(prio) {}
};

/*
 * Audio format detection result for RAW files
 */
struct AudioFormatDetection {
	int32 sampleRate;
	int32 bitDepth;
	int32 channels;
	float confidence;				// 0.0-1.0 detection confidence
	BString detectionMethod;		// How format was detected

	AudioFormatDetection()
		: sampleRate(44100), bitDepth(16), channels(2), confidence(0.0f) {}
};

/*
 * Comprehensive audio file path resolver
 */
class AudioPathResolver {
public:
	AudioPathResolver();
	~AudioPathResolver();

	// Configuration
	void SetSearchStrategy(file_search_strategy strategy) { fSearchStrategy = strategy; }
	void SetSearchTimeout(bigtime_t timeoutUs) { fSearchTimeout = timeoutUs; }
	void SetMaxSearchDepth(int32 depth) { fMaxSearchDepth = depth; }

	// Path translation rules management
	void AddTranslationRule(const PathTranslationRule& rule);
	void LoadDefaultTranslationRules();
	void ClearTranslationRules();

	// Search directory management
	void AddSearchDirectory(const char* path);
	void AddSearchDirectories(const std::vector<BString>& paths);
	void LoadDefaultSearchDirectories();
	void ClearSearchDirectories();

	// Main resolution interface
	AudioFileResolution ResolveAudioFile(const BString& beosPath);
	std::vector<AudioFileResolution> ResolveProjectFiles(const Project3DMix& project);
	bool UpdateProjectPaths(Project3DMix* project);

	// Individual resolution strategies
	AudioFileResolution ResolveByExactPath(const BString& beosPath);
	AudioFileResolution ResolveByTranslation(const BString& beosPath);
	AudioFileResolution ResolveByFilenameSearch(const BString& beosPath);
	AudioFileResolution ResolveByFuzzyMatching(const BString& beosPath);
	AudioFileResolution ResolveByContentAnalysis(const BString& beosPath);

	// Audio format detection for RAW files
	AudioFormatDetection DetectAudioFormat(const BString& filePath);
	AudioFormatDetection AnalyzeRawAudioFile(const BString& filePath);
	bool ConvertAudioFormat(const BString& sourcePath, const BString& targetPath,
	                       const AudioFormatDetection& format);

	// File validation and verification
	bool IsValidAudioFile(const BString& filePath);
	bool IsRawAudioFile(const BString& filePath);
	bool IsSupportedAudioFormat(const BString& filePath);
	off_t GetFileSize(const BString& filePath);

	// Cache management for performance
	void EnableResultCaching(bool enable) { fCacheResults = enable; }
	void ClearResolverCache();
	int32 GetCacheHitCount() const { return fCacheHits; }
	int32 GetCacheMissCount() const { return fCacheMisses; }

	// Statistics and reporting
	struct ResolverStatistics {
		int32 totalResolutions;
		int32 successfulResolutions;
		int32 exactMatches;
		int32 translatedMatches;
		int32 fuzzyMatches;
		int32 failedResolutions;
		bigtime_t totalSearchTime;
		bigtime_t averageSearchTime;
	};
	ResolverStatistics GetStatistics() const { return fStats; }
	void ResetStatistics();

	// Error handling and reporting
	const BString& GetLastError() const { return fLastError; }
	std::vector<BString> GetSearchLog() const { return fSearchLog; }
	void ClearSearchLog();

	// Interactive user guidance
	bool PromptForMissingFile(const BString& originalPath, BString* selectedPath);
	std::vector<BString> SuggestAlternativeFiles(const BString& originalPath);

private:
	// Core path translation
	BString TranslatePath(const BString& beosPath);
	BString ApplyTranslationRules(const BString& beosPath);
	BString NormalizePath(const BString& path);

	// File system operations
	bool SearchInDirectory(const BString& directory, const BString& filename, BString* foundPath);
	bool RecursiveFileSearch(const BString& directory, const BString& filename,
	                        BString* foundPath, int32 currentDepth = 0);
	std::vector<BString> ListAudioFiles(const BString& directory);

	// Fuzzy matching algorithms
	float CalculateStringDistance(const BString& str1, const BString& str2);
	float CalculateFilenameScore(const BString& originalName, const BString& candidateName);
	std::vector<BString> FindSimilarFilenames(const BString& targetName, const std::vector<BString>& candidates);

	// Audio content analysis
	AudioFormatDetection AnalyzeAudioHeader(const BString& filePath);
	AudioFormatDetection AnalyzeAudioContent(const BString& filePath);
	float CalculateAudioSimilarity(const BString& file1, const BString& file2);

	// RAW audio format detection heuristics
	bool TryDetectFormat(const uint8* data, size_t length, AudioFormatDetection* result);
	bool TryCommonFormats(const uint8* data, size_t length, AudioFormatDetection* result);
	bool AnalyzeAudioStatistics(const uint8* data, size_t length, AudioFormatDetection* result);

	// Cache management
	BString GenerateCacheKey(const BString& beosPath);
	bool GetCachedResult(const BString& cacheKey, AudioFileResolution* result);
	void CacheResult(const BString& cacheKey, const AudioFileResolution& result);

	// Utility functions
	BString ExtractFilename(const BString& path);
	BString ExtractDirectory(const BString& path);
	BString ExtractExtension(const BString& path);
	BString RemoveExtension(const BString& filename);

	// Error reporting
	void ReportError(const char* error);
	void AddToSearchLog(const char* logEntry);

	// Configuration
	file_search_strategy fSearchStrategy;
	bigtime_t fSearchTimeout;
	int32 fMaxSearchDepth;
	bool fCacheResults;
	bool fVerboseLogging;

	// Translation rules and search paths
	std::vector<PathTranslationRule> fTranslationRules;
	std::vector<BString> fSearchDirectories;

	// Cache
	std::map<BString, AudioFileResolution> fResolverCache;
	int32 fCacheHits;
	int32 fCacheMisses;

	// Statistics
	ResolverStatistics fStats;

	// Error handling
	BString fLastError;
	std::vector<BString> fSearchLog;

	// Known audio file extensions
	static const char* kAudioExtensions[];
	static const int32 kAudioExtensionCount;

	// Common sample rates for detection
	static const int32 kCommonSampleRates[];
	static const int32 kCommonSampleRateCount;
};

/*
 * Audio file format converter for legacy formats
 */
class AudioFormatConverter {
public:
	AudioFormatConverter();
	~AudioFormatConverter();

	// RAW to WAV conversion
	status_t ConvertRawToWav(const BString& rawPath, const BString& wavPath,
	                        const AudioFormatDetection& format);

	// Batch conversion
	std::vector<BString> ConvertProjectAudioFiles(const Project3DMix& project,
	                                              const BString& outputDirectory);

	// Format validation
	bool IsConversionNeeded(const BString& filePath);
	AudioFormatDetection GetOptimalFormat(const AudioFormatDetection& detected);

	// Progress callback
	typedef void (*ConversionCallback)(const char* currentFile, float progress, void* userData);
	void SetProgressCallback(ConversionCallback callback, void* userData);

private:
	status_t WriteWavHeader(BFile* file, const AudioFormatDetection& format, int32 dataSize);
	status_t ConvertAudioData(BFile* input, BFile* output, const AudioFormatDetection& format);

	ConversionCallback fProgressCallback;
	void* fCallbackUserData;
};

/*
 * Project-level audio file management utilities
 */
class ProjectAudioManager {
public:
	// Validate all audio files in project
	static std::vector<ValidationResult> ValidateProjectAudio(const Project3DMix& project);

	// Create project-relative paths
	static bool MakePathsRelative(Project3DMix* project, const BString& projectDirectory);

	// Copy audio files to project directory
	static bool CopyAudioFilesToProject(Project3DMix* project, const BString& projectDirectory);

	// Generate project audio report
	static BString GenerateAudioReport(const Project3DMix& project);

	// Calculate total audio data size
	static off_t CalculateProjectAudioSize(const Project3DMix& project);

	// Check for missing files
	static std::vector<BString> FindMissingAudioFiles(const Project3DMix& project);

private:
	static bool CopyFile(const BString& sourcePath, const BString& destPath);
	static BString MakeRelativePath(const BString& absolutePath, const BString& basePath);
};

} // namespace VeniceDAW

#endif // AUDIO_PATH_RESOLVER_H