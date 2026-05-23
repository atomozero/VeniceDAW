/*
 * AudioPathResolver.cpp - Intelligent audio file path resolution implementation
 */

#include "AudioPathResolver.h"
#include "../AudioLogging.h"
#include <storage/Path.h>
#include <storage/FindDirectory.h>
#include <app/Roster.h>
#include <media/MediaFile.h>
#include <media/MediaTrack.h>
#include <algorithm>
#include <cstring>

namespace VeniceDAW {

// Static arrays for audio format detection
const char* AudioPathResolver::kAudioExtensions[] = {
	"wav", "aiff", "aif", "flac", "ogg", "mp3", "m4a", "raw", "pcm", "au", "snd"
};

const int32 AudioPathResolver::kAudioExtensionCount =
	sizeof(kAudioExtensions) / sizeof(kAudioExtensions[0]);

const int32 AudioPathResolver::kCommonSampleRates[] = {
	8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000
};

const int32 AudioPathResolver::kCommonSampleRateCount =
	sizeof(kCommonSampleRates) / sizeof(kCommonSampleRates[0]);

// =====================================
// AudioPathResolver Implementation
// =====================================

AudioPathResolver::AudioPathResolver()
	: fSearchStrategy(SEARCH_COMPREHENSIVE)
	, fSearchTimeout(5000000) // 5 seconds
	, fMaxSearchDepth(3)
	, fCacheResults(true)
	, fVerboseLogging(false)
	, fCacheHits(0)
	, fCacheMisses(0)
{
	LoadDefaultTranslationRules();
	LoadDefaultSearchDirectories();
	ResetStatistics();

	AUDIO_LOG_DEBUG("AudioPathResolver", "Initialized with comprehensive search strategy");
}

AudioPathResolver::~AudioPathResolver()
{
	ClearResolverCache();
}

void AudioPathResolver::LoadDefaultTranslationRules()
{
	// Common BeOS to Haiku path translations (ordered by priority)
	AddTranslationRule(PathTranslationRule("/boot/home/", "/boot/home/", 100));
	AddTranslationRule(PathTranslationRule("/boot/Desktop/", "/boot/home/Desktop/", 90));
	AddTranslationRule(PathTranslationRule("/boot/optional/", "/boot/system/apps/", 80));
	AddTranslationRule(PathTranslationRule("/boot/apps/", "/boot/system/apps/", 70));
	AddTranslationRule(PathTranslationRule("/boot/beos/", "/boot/system/", 60));
	AddTranslationRule(PathTranslationRule("/boot/var/", "/var/", 50));
	AddTranslationRule(PathTranslationRule("/boot/tmp/", "/tmp/", 40));

	// Media-specific paths
	AddTranslationRule(PathTranslationRule("/boot/optional/sound/", "/boot/system/data/sounds/", 85));
	AddTranslationRule(PathTranslationRule("/boot/home/music/", "/boot/home/Music/", 75));

	AUDIO_LOG_DEBUG("AudioPathResolver", "Loaded %zu translation rules", fTranslationRules.size());
}

void AudioPathResolver::LoadDefaultSearchDirectories()
{
	// Standard search directories for audio files
	fSearchDirectories.clear();

	// User directories
	fSearchDirectories.push_back(BString("/boot/home/"));
	fSearchDirectories.push_back(BString("/boot/home/Desktop/"));
	fSearchDirectories.push_back(BString("/boot/home/Music/"));
	fSearchDirectories.push_back(BString("/boot/home/Documents/"));

	// System audio directories
	fSearchDirectories.push_back(BString("/boot/system/data/sounds/"));
	fSearchDirectories.push_back(BString("/boot/system/apps/"));

	// Current directory and common project locations
	fSearchDirectories.push_back(BString("./"));
	fSearchDirectories.push_back(BString("../"));
	fSearchDirectories.push_back(BString("./audio/"));
	fSearchDirectories.push_back(BString("./samples/"));

	AUDIO_LOG_DEBUG("AudioPathResolver", "Loaded %zu search directories", fSearchDirectories.size());
}

void AudioPathResolver::AddTranslationRule(const PathTranslationRule& rule)
{
	fTranslationRules.push_back(rule);

	// Sort by priority (higher priority first)
	std::sort(fTranslationRules.begin(), fTranslationRules.end(),
	         [](const PathTranslationRule& a, const PathTranslationRule& b) {
	             return a.priority > b.priority;
	         });
}

void AudioPathResolver::AddSearchDirectory(const char* path)
{
	if (path && strlen(path) > 0) {
		BString pathStr(path);
		if (std::find(fSearchDirectories.begin(), fSearchDirectories.end(), pathStr) == fSearchDirectories.end()) {
			fSearchDirectories.push_back(pathStr);
			AUDIO_LOG_DEBUG("AudioPathResolver", "Added search directory: %s", path);
		}
	}
}

AudioFileResolution AudioPathResolver::ResolveAudioFile(const BString& beosPath)
{
	bigtime_t startTime = system_time();
	AddToSearchLog(BString().SetToFormat("Resolving: %s", beosPath.String()).String());

	// Check cache first
	BString cacheKey = GenerateCacheKey(beosPath);
	AudioFileResolution cached;
	if (fCacheResults && GetCachedResult(cacheKey, &cached)) {
		fCacheHits++;
		return cached;
	}
	fCacheMisses++;

	AudioFileResolution result;
	result.originalPath = beosPath;

	// Strategy 1: Try exact path
	result = ResolveByExactPath(beosPath);
	if (result.wasFound) {
		result.searchMethod = "Exact Path";
		goto resolution_complete;
	}

	// Strategy 2: Try path translation
	result = ResolveByTranslation(beosPath);
	if (result.wasFound) {
		result.searchMethod = "Path Translation";
		goto resolution_complete;
	}

	// Strategy 3: Try filename search
	if (fSearchStrategy >= SEARCH_FILENAME_ONLY) {
		result = ResolveByFilenameSearch(beosPath);
		if (result.wasFound) {
			result.searchMethod = "Filename Search";
			goto resolution_complete;
		}
	}

	// Strategy 4: Try fuzzy matching
	if (fSearchStrategy >= SEARCH_FUZZY_MATCHING) {
		result = ResolveByFuzzyMatching(beosPath);
		if (result.wasFound) {
			result.searchMethod = "Fuzzy Matching";
			goto resolution_complete;
		}
	}

	// Strategy 5: Try content analysis
	if (fSearchStrategy >= SEARCH_CONTENT_ANALYSIS) {
		result = ResolveByContentAnalysis(beosPath);
		if (result.wasFound) {
			result.searchMethod = "Content Analysis";
			goto resolution_complete;
		}
	}

resolution_complete:
	// Update statistics
	fStats.totalResolutions++;
	if (result.wasFound) {
		fStats.successfulResolutions++;
		if (result.searchMethod == "Exact Path") fStats.exactMatches++;
		else if (result.searchMethod == "Path Translation") fStats.translatedMatches++;
		else if (result.searchMethod == "Fuzzy Matching") fStats.fuzzyMatches++;
	} else {
		fStats.failedResolutions++;
	}

	bigtime_t searchTime = system_time() - startTime;
	fStats.totalSearchTime += searchTime;
	fStats.averageSearchTime = fStats.totalSearchTime / fStats.totalResolutions;

	// Cache result
	if (fCacheResults) {
		CacheResult(cacheKey, result);
	}

	// Log result
	if (result.wasFound) {
		AUDIO_LOG_INFO("AudioPathResolver", "Resolved '%s' → '%s' (method: %s, confidence: %.2f)",
		               beosPath.String(), result.resolvedPath.String(),
		               result.searchMethod.String(), result.confidenceScore);
	} else {
		AUDIO_LOG_WARNING("AudioPathResolver", "Failed to resolve: %s", beosPath.String());
	}

	return result;
}

AudioFileResolution AudioPathResolver::ResolveByExactPath(const BString& beosPath)
{
	AudioFileResolution result;
	result.originalPath = beosPath;

	BEntry entry(beosPath.String());
	if (entry.Exists() && entry.IsFile()) {
		result.resolvedPath = beosPath;
		result.wasFound = true;
		result.confidenceScore = 1.0f;
		AddToSearchLog("Found exact path match");
	}

	return result;
}

AudioFileResolution AudioPathResolver::ResolveByTranslation(const BString& beosPath)
{
	AudioFileResolution result;
	result.originalPath = beosPath;

	BString translatedPath = TranslatePath(beosPath);
	if (translatedPath != beosPath) {
		BEntry entry(translatedPath.String());
		if (entry.Exists() && entry.IsFile()) {
			result.resolvedPath = translatedPath;
			result.wasFound = true;
			result.confidenceScore = 0.9f;
			AddToSearchLog(BString().SetToFormat("Found via translation: %s", translatedPath.String()).String());
		}
	}

	return result;
}

AudioFileResolution AudioPathResolver::ResolveByFilenameSearch(const BString& beosPath)
{
	AudioFileResolution result;
	result.originalPath = beosPath;

	BString filename = ExtractFilename(beosPath);
	if (filename.Length() == 0) {
		return result;
	}

	AddToSearchLog(BString().SetToFormat("Searching for filename: %s", filename.String()).String());

	// Search in all configured directories
	for (const auto& searchDir : fSearchDirectories) {
		BString foundPath;
		if (SearchInDirectory(searchDir, filename, &foundPath)) {
			result.resolvedPath = foundPath;
			result.wasFound = true;
			result.confidenceScore = 0.8f;
			AddToSearchLog(BString().SetToFormat("Found in directory: %s", searchDir.String()).String());
			break;
		}
	}

	return result;
}

AudioFileResolution AudioPathResolver::ResolveByFuzzyMatching(const BString& beosPath)
{
	AudioFileResolution result;
	result.originalPath = beosPath;

	BString targetFilename = RemoveExtension(ExtractFilename(beosPath));
	if (targetFilename.Length() == 0) {
		return result;
	}

	AddToSearchLog(BString().SetToFormat("Fuzzy matching for: %s", targetFilename.String()).String());

	float bestScore = 0.0f;
	BString bestMatch;

	// Search in all directories for similar files
	for (const auto& searchDir : fSearchDirectories) {
		std::vector<BString> audioFiles = ListAudioFiles(searchDir);

		for (const auto& candidateFile : audioFiles) {
			BString candidateName = RemoveExtension(ExtractFilename(candidateFile));
			float score = CalculateFilenameScore(targetFilename, candidateName);

			if (score > bestScore && score > 0.6f) { // Minimum threshold
				bestScore = score;
				bestMatch = candidateFile;
			}
		}
	}

	if (bestScore > 0.6f) {
		result.resolvedPath = bestMatch;
		result.wasFound = true;
		result.confidenceScore = bestScore;
		AddToSearchLog(BString().SetToFormat("Fuzzy match found: %s (score: %.2f)",
		                                     bestMatch.String(), bestScore).String());
	}

	return result;
}

AudioFileResolution AudioPathResolver::ResolveByContentAnalysis(const BString& beosPath)
{
	AudioFileResolution result;
	result.originalPath = beosPath;

	// This would be implemented for advanced content matching
	// For now, return empty result
	AddToSearchLog("Content analysis not yet implemented");

	return result;
}

std::vector<AudioFileResolution> AudioPathResolver::ResolveProjectFiles(const Project3DMix& project)
{
	std::vector<AudioFileResolution> results;
	results.reserve(project.CountTracks());

	AUDIO_LOG_INFO("AudioPathResolver", "Resolving audio files for %d tracks", project.CountTracks());

	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track) {
			AudioFileResolution resolution = ResolveAudioFile(track->AudioFilePath());
			results.push_back(resolution);
		}
	}

	// Summary statistics
	int32 resolved = 0;
	for (const auto& res : results) {
		if (res.wasFound) resolved++;
	}

	AUDIO_LOG_INFO("AudioPathResolver", "Resolution complete: %d/%zu files found",
	               resolved, results.size());

	return results;
}

bool AudioPathResolver::UpdateProjectPaths(Project3DMix* project)
{
	if (!project) {
		return false;
	}

	bool allResolved = true;
	int32 updatedCount = 0;

	for (int32 i = 0; i < project->CountTracks(); i++) {
		Track3DMix* track = project->TrackAt(i);
		if (track) {
			AudioFileResolution resolution = ResolveAudioFile(track->AudioFilePath());
			if (resolution.wasFound) {
				track->SetAudioFilePath(resolution.resolvedPath.String());
				updatedCount++;
			} else {
				allResolved = false;
			}
		}
	}

	AUDIO_LOG_INFO("AudioPathResolver", "Updated %d track paths", updatedCount);
	return allResolved;
}

// =====================================
// Utility Functions
// =====================================

BString AudioPathResolver::TranslatePath(const BString& beosPath)
{
	BString result = beosPath;

	// Apply translation rules in priority order
	for (const auto& rule : fTranslationRules) {
		if (result.FindFirst(rule.beosPattern) == 0) {
			result.ReplaceFirst(rule.beosPattern.String(), rule.haikuReplacement.String());
			break;
		}
	}

	return NormalizePath(result);
}

BString AudioPathResolver::NormalizePath(const BString& path)
{
	BPath normalizedPath(path.String());
	if (normalizedPath.InitCheck() == B_OK) {
		return BString(normalizedPath.Path());
	}
	return path;
}

bool AudioPathResolver::SearchInDirectory(const BString& directory, const BString& filename, BString* foundPath)
{
	BDirectory dir(directory.String());
	if (dir.InitCheck() != B_OK) {
		return false;
	}

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsFile()) {
			continue;
		}

		char name[B_FILE_NAME_LENGTH];
		if (entry.GetName(name) == B_OK && filename.ICompare(name) == 0) {
			BPath path;
			if (entry.GetPath(&path) == B_OK) {
				*foundPath = BString(path.Path());
				return true;
			}
		}
	}

	return false;
}

std::vector<BString> AudioPathResolver::ListAudioFiles(const BString& directory)
{
	std::vector<BString> audioFiles;

	BDirectory dir(directory.String());
	if (dir.InitCheck() != B_OK) {
		return audioFiles;
	}

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsFile()) {
			continue;
		}

		BPath path;
		if (entry.GetPath(&path) == B_OK) {
			BString filePath(path.Path());
			if (IsValidAudioFile(filePath)) {
				audioFiles.push_back(filePath);
			}
		}
	}

	return audioFiles;
}

float AudioPathResolver::CalculateFilenameScore(const BString& originalName, const BString& candidateName)
{
	// Simple Levenshtein distance-based scoring
	return 1.0f - (CalculateStringDistance(originalName, candidateName) /
	              fmaxf(originalName.Length(), candidateName.Length()));
}

float AudioPathResolver::CalculateStringDistance(const BString& str1, const BString& str2)
{
	// Simplified Levenshtein distance calculation
	int32 len1 = str1.Length();
	int32 len2 = str2.Length();

	if (len1 == 0) return len2;
	if (len2 == 0) return len1;

	// For performance, use a simple character difference count
	int32 differences = abs(len1 - len2);
	int32 minLen = fminf(len1, len2);

	for (int32 i = 0; i < minLen; i++) {
		if (str1[i] != str2[i]) {
			differences++;
		}
	}

	return differences;
}

bool AudioPathResolver::IsValidAudioFile(const BString& filePath)
{
	BString extension = ExtractExtension(filePath);
	extension.ToLower();

	for (int32 i = 0; i < kAudioExtensionCount; i++) {
		if (extension == kAudioExtensions[i]) {
			return true;
		}
	}

	// Also check if it's a file with no extension (could be RAW)
	return extension.Length() == 0;
}

BString AudioPathResolver::ExtractFilename(const BString& path)
{
	int32 lastSlash = path.FindLast("/");
	if (lastSlash >= 0) {
		return BString(path.String() + lastSlash + 1);
	}
	return path;
}

BString AudioPathResolver::ExtractDirectory(const BString& path)
{
	int32 lastSlash = path.FindLast("/");
	if (lastSlash >= 0) {
		BString result;
		path.CopyInto(result, 0, lastSlash);
		return result;
	}
	return BString("./");
}

BString AudioPathResolver::ExtractExtension(const BString& path)
{
	int32 lastDot = path.FindLast(".");
	if (lastDot >= 0) {
		return BString(path.String() + lastDot + 1);
	}
	return BString("");
}

BString AudioPathResolver::RemoveExtension(const BString& filename)
{
	int32 lastDot = filename.FindLast(".");
	if (lastDot >= 0) {
		BString result;
		filename.CopyInto(result, 0, lastDot);
		return result;
	}
	return filename;
}

// =====================================
// Cache Management
// =====================================

BString AudioPathResolver::GenerateCacheKey(const BString& beosPath)
{
	// Simple cache key based on original path
	return beosPath;
}

bool AudioPathResolver::GetCachedResult(const BString& cacheKey, AudioFileResolution* result)
{
	auto it = fResolverCache.find(cacheKey);
	if (it != fResolverCache.end()) {
		*result = it->second;
		return true;
	}
	return false;
}

void AudioPathResolver::CacheResult(const BString& cacheKey, const AudioFileResolution& result)
{
	fResolverCache[cacheKey] = result;
}

void AudioPathResolver::ClearResolverCache()
{
	fResolverCache.clear();
	fCacheHits = 0;
	fCacheMisses = 0;
}

// =====================================
// Error Handling
// =====================================

void AudioPathResolver::ReportError(const char* error)
{
	fLastError.SetTo(error);
	AUDIO_LOG_ERROR("AudioPathResolver", "%s", error);
}

void AudioPathResolver::AddToSearchLog(const char* logEntry)
{
	if (fVerboseLogging) {
		fSearchLog.push_back(BString(logEntry));
		AUDIO_LOG_DEBUG("AudioPathResolver", "%s", logEntry);
	}
}

void AudioPathResolver::ResetStatistics()
{
	fStats = ResolverStatistics();
}

// =====================================
// Audio Format Detection (Simplified)
// =====================================

AudioFormatDetection AudioPathResolver::DetectAudioFormat(const BString& filePath)
{
	AudioFormatDetection result;

	if (!IsValidAudioFile(filePath)) {
		return result;
	}

	// For RAW files, use heuristic analysis
	if (IsRawAudioFile(filePath)) {
		return AnalyzeRawAudioFile(filePath);
	}

	// For other formats, try to use BMediaFile
	BEntry entry(filePath.String());
	entry_ref ref;
	if (entry.GetRef(&ref) == B_OK) {
		BMediaFile mediaFile(&ref);
		if (mediaFile.InitCheck() == B_OK) {
			int32 trackCount = mediaFile.CountTracks();
			if (trackCount > 0) {
				BMediaTrack* track = mediaFile.TrackAt(0);
				if (track) {
					media_format format;
					if (track->DecodedFormat(&format) == B_OK &&
					    format.type == B_MEDIA_RAW_AUDIO) {
						result.sampleRate = (int32)format.u.raw_audio.frame_rate;
						result.channels = format.u.raw_audio.channel_count;

						// Determine bit depth from format
						switch (format.u.raw_audio.format) {
							case media_raw_audio_format::B_AUDIO_UCHAR:
								result.bitDepth = 8;
								break;
							case media_raw_audio_format::B_AUDIO_SHORT:
								result.bitDepth = 16;
								break;
							case media_raw_audio_format::B_AUDIO_INT:
								result.bitDepth = 24;
								break;
							case media_raw_audio_format::B_AUDIO_FLOAT:
								result.bitDepth = 32;
								break;
							default:
								result.bitDepth = 16;
								break;
						}

						result.confidence = 1.0f;
						result.detectionMethod = "BMediaFile";
					}
					mediaFile.ReleaseTrack(track);
				}
			}
		}
	}

	return result;
}

AudioFormatDetection AudioPathResolver::AnalyzeRawAudioFile(const BString& filePath)
{
	AudioFormatDetection result;

	BFile file(filePath.String(), B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		return result;
	}

	off_t fileSize;
	if (file.GetSize(&fileSize) != B_OK || fileSize == 0) {
		return result;
	}

	// Use common format assumptions for RAW files
	result.sampleRate = 44100;  // Most common
	result.bitDepth = 16;       // Most common
	result.channels = 2;        // Assume stereo
	result.confidence = 0.5f;   // Low confidence for assumptions
	result.detectionMethod = "RAW Format Assumption";

	// Could implement more sophisticated analysis here
	// (frequency analysis, amplitude distribution, etc.)

	return result;
}

bool AudioPathResolver::IsRawAudioFile(const BString& filePath)
{
	BString extension = ExtractExtension(filePath);
	extension.ToLower();

	return (extension == "raw" || extension == "pcm" || extension.Length() == 0);
}

// =====================================
// AudioFormatConverter Implementation
// =====================================

AudioFormatConverter::AudioFormatConverter()
	: fProgressCallback(nullptr), fCallbackUserData(nullptr)
{
}

AudioFormatConverter::~AudioFormatConverter()
{
}

status_t AudioFormatConverter::ConvertRawToWav(const BString& rawPath, const BString& wavPath,
                                               const AudioFormatDetection& format)
{
	if (format.confidence < 0.1f) {
		return B_BAD_VALUE;
	}

	BFile rawFile(rawPath.String(), B_READ_ONLY);
	if (rawFile.InitCheck() != B_OK) {
		return rawFile.InitCheck();
	}

	BFile wavFile(wavPath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (wavFile.InitCheck() != B_OK) {
		return wavFile.InitCheck();
	}

	// Get RAW file size
	off_t rawSize;
	if (rawFile.GetSize(&rawSize) != B_OK) {
		return B_ERROR;
	}

	// Write WAV header
	status_t result = WriteWavHeader(&wavFile, format, (int32)rawSize);
	if (result != B_OK) {
		return result;
	}

	// Convert audio data
	return ConvertAudioData(&rawFile, &wavFile, format);
}

status_t AudioFormatConverter::WriteWavHeader(BFile* file, const AudioFormatDetection& format, int32 dataSize)
{
	// WAV header structure
	struct WavHeader {
		char chunkID[4];        // "RIFF"
		uint32 chunkSize;       // File size - 8
		char format[4];         // "WAVE"
		char subchunk1ID[4];    // "fmt "
		uint32 subchunk1Size;   // 16 for PCM
		uint16 audioFormat;     // 1 for PCM
		uint16 numChannels;     // 1 = mono, 2 = stereo
		uint32 sampleRate;      // Sample rate
		uint32 byteRate;        // SampleRate * NumChannels * BitsPerSample/8
		uint16 blockAlign;      // NumChannels * BitsPerSample/8
		uint16 bitsPerSample;   // Bits per sample
		char subchunk2ID[4];    // "data"
		uint32 subchunk2Size;   // Data size
	};

	WavHeader header;
	memset(&header, 0, sizeof(header));

	// Fill header
	memcpy(header.chunkID, "RIFF", 4);
	header.chunkSize = dataSize + sizeof(WavHeader) - 8;
	memcpy(header.format, "WAVE", 4);
	memcpy(header.subchunk1ID, "fmt ", 4);
	header.subchunk1Size = 16;
	header.audioFormat = 1;  // PCM
	header.numChannels = format.channels;
	header.sampleRate = format.sampleRate;
	header.bitsPerSample = format.bitDepth;
	header.byteRate = format.sampleRate * format.channels * format.bitDepth / 8;
	header.blockAlign = format.channels * format.bitDepth / 8;
	memcpy(header.subchunk2ID, "data", 4);
	header.subchunk2Size = dataSize;

	ssize_t written = file->Write(&header, sizeof(header));
	return (written == sizeof(header)) ? B_OK : B_ERROR;
}

status_t AudioFormatConverter::ConvertAudioData(BFile* input, BFile* output, const AudioFormatDetection& /* format */)
{
	const size_t bufferSize = 4096;
	uint8 buffer[bufferSize];

	off_t totalRead = 0;
	off_t fileSize;
	input->GetSize(&fileSize);

	while (totalRead < fileSize) {
		ssize_t bytesRead = input->Read(buffer, bufferSize);
		if (bytesRead <= 0) {
			break;
		}

		ssize_t bytesWritten = output->Write(buffer, bytesRead);
		if (bytesWritten != bytesRead) {
			return B_ERROR;
		}

		totalRead += bytesRead;

		// Progress callback
		if (fProgressCallback) {
			float progress = (float)totalRead / fileSize;
			fProgressCallback("Converting audio data", progress, fCallbackUserData);
		}
	}

	return B_OK;
}

std::vector<BString> AudioFormatConverter::ConvertProjectAudioFiles(const Project3DMix& project,
                                                                    const BString& outputDirectory)
{
	std::vector<BString> convertedFiles;

	for (int32 i = 0; i < project.CountTracks(); i++) {
		const Track3DMix* track = project.TrackAt(i);
		if (!track) continue;

		BString audioPath = track->AudioFilePath();
		if (IsConversionNeeded(audioPath)) {
			BString outputPath = outputDirectory;
			outputPath << "/" << track->TrackName() << ".wav";

			AudioPathResolver resolver;
			AudioFormatDetection format = resolver.DetectAudioFormat(audioPath);

			if (ConvertRawToWav(audioPath, outputPath, format) == B_OK) {
				convertedFiles.push_back(outputPath);
			}
		} else {
			convertedFiles.push_back(audioPath);
		}
	}

	return convertedFiles;
}

bool AudioFormatConverter::IsConversionNeeded(const BString& filePath)
{
	AudioPathResolver resolver;
	return resolver.IsRawAudioFile(filePath);
}

AudioFormatDetection AudioFormatConverter::GetOptimalFormat(const AudioFormatDetection& detected)
{
	AudioFormatDetection optimal = detected;

	// Ensure minimum quality standards
	if (optimal.sampleRate < 22050) {
		optimal.sampleRate = 44100;
	}
	if (optimal.bitDepth < 16) {
		optimal.bitDepth = 16;
	}
	if (optimal.channels < 1) {
		optimal.channels = 2;
	}

	return optimal;
}

void AudioFormatConverter::SetProgressCallback(ConversionCallback callback, void* userData)
{
	fProgressCallback = callback;
	fCallbackUserData = userData;
}

} // namespace VeniceDAW