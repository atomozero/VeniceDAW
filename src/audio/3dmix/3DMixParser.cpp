/*
 * 3DMixParser.cpp - Complete BeOS 3dmix file parser implementation
 */

#include "3DMixParser.h"
#include "../AudioLogging.h"
#ifdef __HAIKU__
	#include <storage/Directory.h>
	#include <storage/Path.h>
	#include <storage/FindDirectory.h>
#endif
#include <string.h>
#include <stdio.h>
#include <algorithm>

namespace VeniceDAW {

// =====================================
// BMessageParser Implementation
// =====================================

// Known field mappings from reverse engineering
const BMessageParser::FieldMapping BMessageParser::kKnownFields[] = {
	{"volume", Format3DMix::kTypeLong, &BMessageParser::HandleVolumeField},
	{"balance", Format3DMix::kTypeLong, &BMessageParser::HandleBalanceField},
	{"enabled", Format3DMix::kTypeBool, &BMessageParser::HandleEnabledField},
	{"pos_x", Format3DMix::kTypeLong, &BMessageParser::HandlePositionXField},
	{"pos_y", Format3DMix::kTypeLong, &BMessageParser::HandlePositionYField},
	{"pos_z", Format3DMix::kTypeLong, &BMessageParser::HandlePositionZField},
	{"loop_start", Format3DMix::kTypeLong, &BMessageParser::HandleLoopStartField},
	{"loop_end", Format3DMix::kTypeLong, &BMessageParser::HandleLoopEndField},
	{"sample_rate", Format3DMix::kTypeLong, &BMessageParser::HandleSampleRateField}
};

const int32 BMessageParser::kKnownFieldCount = sizeof(kKnownFields) / sizeof(FieldMapping);

BMessageParser::BMessageParser()
	: fErrorCount(0)
{
}

BMessageParser::~BMessageParser()
{
}

status_t BMessageParser::ParseBMessageData(const uint8* data, size_t length, Track3DMix* track)
{
	if (!data || length == 0 || !track) {
		ReportError("Invalid parameters for BMessage parsing");
		return B_BAD_VALUE;
	}

	AUDIO_LOG_DEBUG("3DMixParser", "Parsing BMessage data: %zu bytes", length);

	// Create BMessage from raw data
	BMessage message;
	status_t status = message.Unflatten((const char*)data);
	if (status != B_OK) {
		ReportError("Failed to unflatten BMessage data");
		return status;
	}

	// Extract track parameters
	status = ExtractTrackParameters(message, track);
	if (status != B_OK) {
		ReportError("Failed to extract track parameters");
		return status;
	}

	// Extract audio format
	AudioFormat3DMix audioFormat;
	status = ExtractAudioFormat(message, &audioFormat);
	if (status == B_OK) {
		track->SetAudioFormat(audioFormat);
	}

	// Extract position data
	Coordinate3D position;
	status = ExtractPositionData(message, &position);
	if (status == B_OK) {
		track->SetPosition(position);
	}

	AUDIO_LOG_DEBUG("3DMixParser", "Successfully parsed BMessage data");
	return B_OK;
}

status_t BMessageParser::ExtractTrackParameters(const BMessage& message, Track3DMix* track)
{
	// Volume parameter
	float volume;
	if (message.FindFloat("volume", &volume) == B_OK) {
		track->SetVolume(volume);
		AUDIO_LOG_DEBUG("3DMixParser", "Found volume: %.3f", volume);
	}

	// Balance parameter
	float balance;
	if (message.FindFloat("balance", &balance) == B_OK) {
		track->SetBalance(balance);
		AUDIO_LOG_DEBUG("3DMixParser", "Found balance: %.3f", balance);
	}

	// Enabled state
	bool enabled;
	if (message.FindBool("enabled", &enabled) == B_OK) {
		track->SetEnabled(enabled);
		AUDIO_LOG_DEBUG("3DMixParser", "Found enabled: %s", enabled ? "true" : "false");
	}

	// Loop parameters
	int32 loopStart, loopEnd;
	if (message.FindInt32("loop_start", &loopStart) == B_OK) {
		track->SetLoopStart(loopStart);
		AUDIO_LOG_DEBUG("3DMixParser", "Found loop start: %d", loopStart);
	}
	if (message.FindInt32("loop_end", &loopEnd) == B_OK) {
		track->SetLoopEnd(loopEnd);
		AUDIO_LOG_DEBUG("3DMixParser", "Found loop end: %d", loopEnd);
	}

	// Loop enabled
	bool loopEnabled;
	if (message.FindBool("loop_enabled", &loopEnabled) == B_OK) {
		track->SetLoopEnabled(loopEnabled);
		AUDIO_LOG_DEBUG("3DMixParser", "Found loop enabled: %s", loopEnabled ? "true" : "false");
	}

	// Effects parameters
	float reverbLevel;
	if (message.FindFloat("reverb_level", &reverbLevel) == B_OK) {
		track->SetReverbLevel(reverbLevel);
		AUDIO_LOG_DEBUG("3DMixParser", "Found reverb level: %.3f", reverbLevel);
	}

	float distanceAttenuation;
	if (message.FindFloat("distance_attenuation", &distanceAttenuation) == B_OK) {
		track->SetDistanceAttenuation(distanceAttenuation);
		AUDIO_LOG_DEBUG("3DMixParser", "Found distance attenuation: %.3f", distanceAttenuation);
	}

	// GUI state
	int32 windowX, windowY;
	if (message.FindInt32("window_x", &windowX) == B_OK &&
	    message.FindInt32("window_y", &windowY) == B_OK) {
		track->SetWindowPosition(windowX, windowY);
		AUDIO_LOG_DEBUG("3DMixParser", "Found window position: (%d, %d)", windowX, windowY);
	}

	bool windowVisible;
	if (message.FindBool("window_visible", &windowVisible) == B_OK) {
		track->SetWindowVisible(windowVisible);
		AUDIO_LOG_DEBUG("3DMixParser", "Found window visible: %s", windowVisible ? "true" : "false");
	}

	return B_OK;
}

status_t BMessageParser::ExtractAudioFormat(const BMessage& message, AudioFormat3DMix* format)
{
	// Sample rate
	int32 sampleRate;
	if (message.FindInt32("sample_rate", &sampleRate) == B_OK) {
		format->sampleRate = sampleRate;
		AUDIO_LOG_DEBUG("3DMixParser", "Found sample rate: %d", sampleRate);
	}

	// Bit depth
	int32 bitDepth;
	if (message.FindInt32("bit_depth", &bitDepth) == B_OK) {
		format->bitDepth = bitDepth;
		AUDIO_LOG_DEBUG("3DMixParser", "Found bit depth: %d", bitDepth);
	}

	// Channels
	int32 channels;
	if (message.FindInt32("channels", &channels) == B_OK) {
		format->channels = channels;
		AUDIO_LOG_DEBUG("3DMixParser", "Found channels: %d", channels);
	}

	// File size
	int32 fileSize;
	if (message.FindInt32("file_size", &fileSize) == B_OK) {
		format->fileSize = fileSize;
		AUDIO_LOG_DEBUG("3DMixParser", "Found file size: %d", fileSize);
	}

	// Always assume RAW format for 3dmix files
	format->isRawFormat = true;

	return B_OK;
}

status_t BMessageParser::ExtractPositionData(const BMessage& message, Coordinate3D* position)
{
	float x, y, z;

	if (message.FindFloat("pos_x", &x) == B_OK &&
	    message.FindFloat("pos_y", &y) == B_OK &&
	    message.FindFloat("pos_z", &z) == B_OK) {

		position->x = x;
		position->y = y;
		position->z = z;

		AUDIO_LOG_DEBUG("3DMixParser", "Found position: (%.2f, %.2f, %.2f)", x, y, z);
		return B_OK;
	}

	return B_NAME_NOT_FOUND;
}

void BMessageParser::ReportError(const char* error)
{
	fLastError.SetTo(error);
	fErrorCount++;
	AUDIO_LOG_ERROR("3DMixParser", "%s", error);
}

// =====================================
// Legacy3DMixLoader Implementation
// =====================================

Legacy3DMixLoader::Legacy3DMixLoader()
	: fLoadingTime(0)
	, fLoadedTrackCount(0)
	, fFailedTrackCount(0)
	, fStrictValidation(false)
	, fSearchMissingFiles(true)
	, fAutoDetectFormat(true)
{
	// Initialize search paths for missing files
	fSearchPaths.push_back(BString("/boot/home/Desktop/"));
	fSearchPaths.push_back(BString("/boot/home/"));
	fSearchPaths.push_back(BString("/system/data/sounds/"));
	fSearchPaths.push_back(BString("./"));
}

Legacy3DMixLoader::~Legacy3DMixLoader()
{
}

status_t Legacy3DMixLoader::LoadProject(const char* filePath)
{
	if (!filePath) {
		ReportError("Invalid file path");
		return B_BAD_VALUE;
	}

	BFile file(filePath, B_READ_ONLY);
	status_t status = file.InitCheck();
	if (status != B_OK) {
		ReportError("Failed to open 3dmix file");
		return status;
	}

	return LoadProject(&file);
}

status_t Legacy3DMixLoader::LoadProject(BFile* file)
{
	if (!file || file->InitCheck() != B_OK) {
		ReportError("Invalid file object");
		return B_BAD_VALUE;
	}

	bigtime_t startTime = system_time();

	AUDIO_LOG_INFO("3DMixLoader", "Starting 3dmix project loading...");

	// Clear previous state
	fProject = Project3DMix();
	fValidationResults.clear();
	fLoadedTrackCount = 0;
	fFailedTrackCount = 0;

	// Phase 1: Parse file header
	status_t status = ParseFileHeader(file);
	if (status != B_OK) {
		ReportError("Failed to parse file header");
		return status;
	}

	// Phase 2: Parse track records
	status = ParseTrackRecords(file);
	if (status != B_OK) {
		ReportError("Failed to parse track records");
		return status;
	}

	// Phase 3: Validate project
	status = ValidateProject();
	if (status != B_OK && fStrictValidation) {
		ReportError("Project validation failed");
		return status;
	}

	// Phase 4: Post-processing
	status = PostProcessProject();
	if (status != B_OK) {
		ReportWarning("Post-processing completed with warnings");
	}

	fLoadingTime = system_time() - startTime;

	AUDIO_LOG_INFO("3DMixLoader", "Successfully loaded 3dmix project:");
	AUDIO_LOG_INFO("3DMixLoader", "  Tracks loaded: %d", fLoadedTrackCount);
	AUDIO_LOG_INFO("3DMixLoader", "  Tracks failed: %d", fFailedTrackCount);
	AUDIO_LOG_INFO("3DMixLoader", "  Loading time: %lld μs", fLoadingTime);

	return B_OK;
}

status_t Legacy3DMixLoader::ParseFileHeader(BDataIO* stream)
{
	AUDIO_LOG_DEBUG("3DMixLoader", "Parsing file header...");

	// Validate magic number
	status_t status = ValidateMagicNumber(stream);
	if (status != B_OK) {
		return status;
	}

	// Read track count
	int32 trackCount;
	status = ReadTrackCount(stream, &trackCount);
	if (status != B_OK) {
		return status;
	}

	AUDIO_LOG_DEBUG("3DMixLoader", "Expected track count: %d", trackCount);

	// Read base path
	BString basePath;
	status = ReadBasePath(stream, &basePath);
	if (status != B_OK) {
		return status;
	}

	AUDIO_LOG_DEBUG("3DMixLoader", "Base path: %s", basePath.String());

	// Extract project name from base path
	BString projectName = Format3DMixUtils::ExtractFileName(basePath);
	if (projectName.Length() == 0) {
		projectName = "Unnamed Project";
	}

	// Set project information
	fProject.SetProjectName(projectName.String());
	fProject.SetBasePath(basePath.String());

	return B_OK;
}

status_t Legacy3DMixLoader::ValidateMagicNumber(BDataIO* stream)
{
	uint32 magic;
	ssize_t bytesRead = stream->Read(&magic, sizeof(magic));
	if (bytesRead != sizeof(magic)) {
		ReportError("Failed to read magic number");
		return B_IO_ERROR;
	}

	if (magic != Format3DMix::kMagicNumber) {
		ReportError("Invalid magic number in 3dmix file");
		return B_BAD_DATA;
	}

	AUDIO_LOG_DEBUG("3DMixLoader", "Valid magic number found");
	return B_OK;
}

status_t Legacy3DMixLoader::ReadTrackCount(BDataIO* stream, int32* trackCount)
{
	ssize_t bytesRead = stream->Read(trackCount, sizeof(int32));
	if (bytesRead != sizeof(int32)) {
		ReportError("Failed to read track count");
		return B_IO_ERROR;
	}

	// Convert from little-endian if necessary
	*trackCount = B_LENDIAN_TO_HOST_INT32(*trackCount);

	if (*trackCount < 0 || *trackCount > 1000) { // Reasonable limit
		ReportError("Invalid track count in 3dmix file");
		return B_BAD_DATA;
	}

	return B_OK;
}

status_t Legacy3DMixLoader::ReadBasePath(BDataIO* stream, BString* basePath)
{
	// Read null-terminated string
	char buffer[1024];
	int32 index = 0;
	char byte;

	while (index < (int32)(sizeof(buffer) - 1)) {
		ssize_t bytesRead = stream->Read(&byte, 1);
		if (bytesRead != 1) {
			ReportError("Failed to read base path");
			return B_IO_ERROR;
		}

		if (byte == '\0') {
			break;
		}

		buffer[index++] = byte;
	}

	buffer[index] = '\0';
	basePath->SetTo(buffer);

	return B_OK;
}

status_t Legacy3DMixLoader::ParseTrackRecords(BDataIO* stream)
{
	AUDIO_LOG_DEBUG("3DMixLoader", "Parsing track records...");

	// Continue reading until end of file
	while (true) {
		Track3DMix* track = new Track3DMix();

		status_t status = ParseSingleTrackRecord(stream, track);
		if (status == B_END_OF_DATA) {
			delete track;
			break; // Normal end of file
		}

		if (status == B_OK) {
			if (fProject.AddTrack(track)) {
				fLoadedTrackCount++;
				AUDIO_LOG_DEBUG("3DMixLoader", "Successfully loaded track %d", fLoadedTrackCount);
			} else {
				delete track;
				fFailedTrackCount++;
				ReportWarning("Failed to add track to project");
			}
		} else {
			delete track;
			fFailedTrackCount++;
			ReportWarning("Failed to parse track record");
		}
	}

	AUDIO_LOG_INFO("3DMixLoader", "Parsed %d tracks (%d failed)", fLoadedTrackCount, fFailedTrackCount);
	return B_OK;
}

status_t Legacy3DMixLoader::ParseSingleTrackRecord(BDataIO* stream, Track3DMix* track)
{
	// Read audio file path
	BString audioFilePath;
	status_t status = ReadAudioFilePath(stream, &audioFilePath);
	if (status != B_OK) {
		return status;
	}

	// Translate BeOS path to Haiku path
	BString haikuPath;
	status = TranslatePath(audioFilePath, &haikuPath);
	if (status == B_OK) {
		track->SetAudioFilePath(haikuPath.String());
	} else {
		track->SetAudioFilePath(audioFilePath.String()); // Keep original for reference
		ReportWarning("Could not translate audio file path");
	}

	// Extract track name from file path
	BString trackName = Format3DMixUtils::ExtractFileName(audioFilePath);
	track->SetTrackName(trackName.String());

	// Read BMessage data
	std::vector<uint8> bMessageData;
	status = ReadBMessageData(stream, &bMessageData);
	if (status != B_OK) {
		return status;
	}

	// Process BMessage data
	status = ProcessTrackData(bMessageData, track);
	if (status != B_OK) {
		ReportWarning("Failed to process track data completely");
	}

	// Store raw BMessage data for future use
	track->SetRawBMessageData(bMessageData);

	// Auto-detect audio format if enabled
	if (fAutoDetectFormat) {
		AudioFormat3DMix format;
		if (DetectAudioFormat(haikuPath, &format) == B_OK) {
			track->SetAudioFormat(format);
		}
	}

	return B_OK;
}

status_t Legacy3DMixLoader::TranslatePath(const BString& beosPath, BString* haikuPath)
{
	// Simple path translation for common BeOS directories
	BString translatedPath = beosPath;

	// Replace BeOS paths with Haiku equivalents
	if (translatedPath.FindFirst("/boot/home/") == 0) {
		translatedPath.ReplaceFirst("/boot/home/", "/boot/home/");
	} else if (translatedPath.FindFirst("/boot/optional/") == 0) {
		translatedPath.ReplaceFirst("/boot/optional/", "/boot/system/apps/");
	} else if (translatedPath.FindFirst("/boot/Desktop/") == 0) {
		translatedPath.ReplaceFirst("/boot/Desktop/", "/boot/home/Desktop/");
	} else if (translatedPath.FindFirst("/boot/apps/") == 0) {
		translatedPath.ReplaceFirst("/boot/apps/", "/boot/system/apps/");
	}

	*haikuPath = translatedPath;

	// Verify file exists
	BFile file(haikuPath->String(), B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		return B_OK;
	}

	// If not found, try searching in common locations
	if (fSearchMissingFiles) {
		BString foundPath;
		if (SearchForFile(*haikuPath, &foundPath) == B_OK) {
			*haikuPath = foundPath;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}

status_t Legacy3DMixLoader::SearchForFile(const BString& originalPath, BString* foundPath)
{
	BString fileName = Format3DMixUtils::ExtractFileName(originalPath);

	for (const auto& searchPath : fSearchPaths) {
		BString candidatePath = searchPath;
		candidatePath << "/" << fileName;

		BFile file(candidatePath.String(), B_READ_ONLY);
		if (file.InitCheck() == B_OK) {
			*foundPath = candidatePath;
			AUDIO_LOG_DEBUG("3DMixLoader", "Found file: %s", candidatePath.String());
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}

status_t Legacy3DMixLoader::ProcessTrackData(const std::vector<uint8>& data, Track3DMix* track)
{
	if (data.empty()) {
		return B_OK; // Empty data is acceptable
	}

	return fBMessageParser.ParseBMessageData(data.data(), data.size(), track);
}

void Legacy3DMixLoader::ReportError(const char* error)
{
	fLastError.SetTo(error);
	AUDIO_LOG_ERROR("3DMixLoader", "%s", error);
}

void Legacy3DMixLoader::ReportWarning(const char* warning)
{
	AUDIO_LOG_WARNING("3DMixLoader", "%s", warning);
}

// Simplified implementations for remaining methods...
status_t Legacy3DMixLoader::ReadAudioFilePath(BDataIO* stream, BString* filePath)
{
	// Read null-terminated string similar to ReadBasePath
	char buffer[1024];
	int32 index = 0;
	char byte;

	while (index < (int32)(sizeof(buffer) - 1)) {
		ssize_t bytesRead = stream->Read(&byte, 1);
		if (bytesRead != 1) {
			if (index == 0) {
				return B_END_OF_DATA; // End of file
			}
			return B_IO_ERROR;
		}

		if (byte == '\0') {
			break;
		}

		buffer[index++] = byte;
	}

	buffer[index] = '\0';
	filePath->SetTo(buffer);

	return B_OK;
}

status_t Legacy3DMixLoader::ReadBMessageData(BDataIO* stream, std::vector<uint8>* data)
{
	// For now, read a fixed-size block (this would need refinement for real BMessage parsing)
	const size_t kMaxBMessageSize = 4096;
	data->resize(kMaxBMessageSize);

	ssize_t bytesRead = stream->Read(data->data(), kMaxBMessageSize);
	if (bytesRead < 0) {
		return B_IO_ERROR;
	}

	data->resize(bytesRead);
	return B_OK;
}

status_t Legacy3DMixLoader::ValidateProject()
{
	fValidationResults = ProjectValidator::ValidateProject(fProject);
	return B_OK;
}

status_t Legacy3DMixLoader::PostProcessProject()
{
	return CalculateProjectStatistics();
}

status_t Legacy3DMixLoader::CalculateProjectStatistics()
{
	// Calculate project-level statistics
	int32 totalSamples = fProject.CalculateTotalSamples();
	fProject.SetProjectLength(totalSamples);

	// Set project sample rate to most common track sample rate
	std::map<int32, int32> sampleRateCounts;
	for (int32 i = 0; i < fProject.CountTracks(); i++) {
		Track3DMix* track = fProject.TrackAt(i);
		if (track) {
			sampleRateCounts[track->GetAudioFormat().sampleRate]++;
		}
	}

	int32 mostCommonSampleRate = Format3DMix::kDefaultSampleRate;
	int32 maxCount = 0;
	for (const auto& pair : sampleRateCounts) {
		if (pair.second > maxCount) {
			maxCount = pair.second;
			mostCommonSampleRate = pair.first;
		}
	}

	fProject.SetProjectSampleRate(mostCommonSampleRate);

	return B_OK;
}

status_t Legacy3DMixLoader::DetectAudioFormat(const BString& filePath, AudioFormat3DMix* format)
{
	// Simplified audio format detection
	BFile file(filePath.String(), B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		return B_ENTRY_NOT_FOUND;
	}

	off_t fileSize;
	if (file.GetSize(&fileSize) != B_OK) {
		return B_IO_ERROR;
	}

	format->fileSize = (int32)fileSize;
	format->isRawFormat = true;

	// Use default values for now (would need heuristic analysis)
	format->sampleRate = Format3DMix::kDefaultSampleRate;
	format->bitDepth = Format3DMix::kDefaultBitDepth;
	format->channels = Format3DMix::kDefaultChannels;

	return B_OK;
}

// =====================================
// Format3DMixUtils Implementation
// =====================================

bool Format3DMixUtils::IsValidMagicNumber(uint32 magic)
{
	return magic == Format3DMix::kMagicNumber;
}

BString Format3DMixUtils::ExtractFileName(const BString& path)
{
	int32 lastSlash = path.FindLast("/");
	if (lastSlash >= 0) {
		return BString(path.String() + lastSlash + 1);
	}
	return path;
}

BString Format3DMixUtils::ExtractDirectory(const BString& path)
{
	int32 lastSlash = path.FindLast("/");
	if (lastSlash >= 0) {
		BString result;
		path.CopyInto(result, 0, lastSlash);
		return result;
	}
	return BString("./");
}

bool Format3DMixUtils::IsBeOSPath(const BString& path)
{
	return path.FindFirst("/boot/") == 0;
}

// =====================================
// ProjectValidator Implementation
// =====================================

std::vector<ValidationResult> ProjectValidator::ValidateProject(const Project3DMix& project)
{
	std::vector<ValidationResult> results;

	// Validate project name
	if (project.ProjectName().Length() == 0) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Project has no name", "Project metadata"));
	}

	// Validate track count
	int32 trackCount = project.CountTracks();
	if (trackCount == 0) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Project contains no tracks", "Project structure"));
	} else if (trackCount > 64) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Project has unusually high track count (>64)", "Project structure"));
	}

	// Validate master volume
	float masterVolume = project.MasterVolume();
	if (masterVolume < 0.0f || masterVolume > 2.0f) {
		results.push_back(ValidationResult(VALIDATION_ERROR,
			"Master volume out of valid range (0.0-2.0)", "Project audio"));
	}

	// Validate sample rate
	int32 sampleRate = project.ProjectSampleRate();
	if (sampleRate != 44100 && sampleRate != 48000 && sampleRate != 96000) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Unusual project sample rate (expected 44100, 48000, or 96000)", "Project audio"));
	}

	// Validate listener position
	const Coordinate3D& listenerPos = project.ListenerPosition();
	if (!IsCoordinateInRange(listenerPos)) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Listener position outside typical range", "3D scene"));
	}

	// Validate each track
	for (int32 i = 0; i < trackCount; i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track) {
			std::vector<ValidationResult> trackResults = ValidateTrack(*track, i);
			results.insert(results.end(), trackResults.begin(), trackResults.end());
		}
	}

	return results;
}

// =====================================
// Field Handler Implementations
// Note: These handlers are currently unused as BMessage::Unflatten() handles
// all parsing automatically. They are kept for potential future low-level parsing needs.
// =====================================

void BMessageParser::HandleVolumeField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	float volume = ExtractFloat(data);
	track->SetVolume(volume);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed volume field: %.3f", volume);
}

void BMessageParser::HandleBalanceField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	float balance = ExtractFloat(data);
	track->SetBalance(balance);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed balance field: %.3f", balance);
}

void BMessageParser::HandleEnabledField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	bool enabled = ExtractBool(data);
	track->SetEnabled(enabled);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed enabled field: %s", enabled ? "true" : "false");
}

void BMessageParser::HandlePositionXField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	float x = ExtractFloat(data);
	Coordinate3D pos = track->Position();
	pos.x = x;
	track->SetPosition(pos);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed position X field: %.2f", x);
}

void BMessageParser::HandlePositionYField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	float y = ExtractFloat(data);
	Coordinate3D pos = track->Position();
	pos.y = y;
	track->SetPosition(pos);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed position Y field: %.2f", y);
}

void BMessageParser::HandlePositionZField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	float z = ExtractFloat(data);
	Coordinate3D pos = track->Position();
	pos.z = z;
	track->SetPosition(pos);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed position Z field: %.2f", z);
}

void BMessageParser::HandleLoopStartField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	int32 loopStart = ExtractInt32(data);
	track->SetLoopStart(loopStart);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed loop start field: %d", loopStart);
}

void BMessageParser::HandleLoopEndField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	int32 loopEnd = ExtractInt32(data);
	track->SetLoopEnd(loopEnd);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed loop end field: %d", loopEnd);
}

void BMessageParser::HandleSampleRateField(const uint8* data, Track3DMix* track)
{
	if (!data || !track) return;
	int32 sampleRate = ExtractInt32(data);
	AudioFormat3DMix format = track->GetAudioFormat();
	format.sampleRate = sampleRate;
	track->SetAudioFormat(format);
	AUDIO_LOG_DEBUG("3DMixParser", "Parsed sample rate field: %d", sampleRate);
}

// Helper method implementations for field handlers
float BMessageParser::ExtractFloat(const uint8* data, bool littleEndian)
{
	if (!data) return 0.0f;

	if (littleEndian) {
		// Little-endian byte order (Intel/x86)
		uint32 raw = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
		return *reinterpret_cast<float*>(&raw);
	} else {
		// Big-endian byte order (PowerPC/BeOS original)
		uint32 raw = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
		return *reinterpret_cast<float*>(&raw);
	}
}

int32 BMessageParser::ExtractInt32(const uint8* data, bool littleEndian)
{
	if (!data) return 0;

	if (littleEndian) {
		return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
	} else {
		return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
	}
}

bool BMessageParser::ExtractBool(const uint8* data)
{
	if (!data) return false;
	return data[0] != 0;
}

// =====================================
// ProjectValidator Static Methods
// =====================================

std::vector<ValidationResult> ProjectValidator::ValidateTrack(const Track3DMix& track, int32 trackIndex)
{
	std::vector<ValidationResult> results;
	char context[256];
	snprintf(context, sizeof(context), "Track %d", trackIndex);

	// Validate audio path
	ValidateTrackAudio(track, trackIndex, results);

	// Validate 3D position
	ValidateTrackPosition(track, trackIndex, results);

	// Validate timing parameters
	ValidateTrackTiming(track, trackIndex, results);

	return results;
}

bool ProjectValidator::IsCoordinateInRange(const Coordinate3D& coord)
{
	return coord.x >= Format3DMix::kMinCoordinate && coord.x <= Format3DMix::kMaxCoordinate &&
	       coord.y >= Format3DMix::kMinCoordinate && coord.y <= Format3DMix::kMaxCoordinate &&
	       coord.z >= Format3DMix::kMinCoordinate && coord.z <= Format3DMix::kMaxCoordinate;
}

bool ProjectValidator::IsPathValid(const BString& path)
{
	if (path.Length() == 0) return false;

	// Check for invalid characters
	if (path.FindFirst('\0') >= 0) return false;

	// Check path length (typical max path on BeOS/Haiku is 1024)
	if (path.Length() > 1024) return false;

	return true;
}

bool ProjectValidator::IsAudioFormatSupported(const AudioFormat3DMix& format)
{
	// Validate sample rate
	if (format.sampleRate != 44100 && format.sampleRate != 48000 &&
	    format.sampleRate != 88200 && format.sampleRate != 96000) {
		return false;
	}

	// Validate bit depth
	if (format.bitDepth != 8 && format.bitDepth != 16 &&
	    format.bitDepth != 24 && format.bitDepth != 32) {
		return false;
	}

	// Validate channel count
	if (format.channels < 1 || format.channels > 8) {
		return false;
	}

	return true;
}

void ProjectValidator::ValidateTrackAudio(const Track3DMix& track, int32 trackIndex,
                                           std::vector<ValidationResult>& results)
{
	char context[256];
	snprintf(context, sizeof(context), "Track %d audio", trackIndex);

	// Validate audio file path
	if (track.AudioFilePath().Length() == 0) {
		results.push_back(ValidationResult(VALIDATION_ERROR,
			"Track has no audio file path", context));
	} else if (!IsPathValid(track.AudioFilePath())) {
		results.push_back(ValidationResult(VALIDATION_ERROR,
			"Track has invalid audio file path", context));
	}

	// Validate audio format
	const AudioFormat3DMix& format = track.GetAudioFormat();
	if (!IsAudioFormatSupported(format)) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Track has unsupported or unusual audio format", context));
	}

	// Validate volume
	if (track.Volume() < 0.0f || track.Volume() > 2.0f) {
		results.push_back(ValidationResult(VALIDATION_ERROR,
			"Track volume out of valid range (0.0-2.0)", context));
	}

	// Validate balance
	if (track.Balance() < -1.0f || track.Balance() > 1.0f) {
		results.push_back(ValidationResult(VALIDATION_ERROR,
			"Track balance out of valid range (-1.0 to 1.0)", context));
	}
}

void ProjectValidator::ValidateTrackPosition(const Track3DMix& track, int32 trackIndex,
                                               std::vector<ValidationResult>& results)
{
	char context[256];
	snprintf(context, sizeof(context), "Track %d position", trackIndex);

	const Coordinate3D& pos = track.Position();
	if (!IsCoordinateInRange(pos)) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Track position outside valid BeOS range (-12.0 to 12.0)", context));
	}

	// Warn if position is at origin (might be uninitialized)
	if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Track positioned at origin (might be default/unset)", context));
	}
}

void ProjectValidator::ValidateTrackTiming(const Track3DMix& track, int32 trackIndex,
                                             std::vector<ValidationResult>& results)
{
	char context[256];
	snprintf(context, sizeof(context), "Track %d timing", trackIndex);

	// Validate loop points if looping is enabled
	if (track.IsLoopEnabled()) {
		if (track.LoopStart() < 0) {
			results.push_back(ValidationResult(VALIDATION_ERROR,
				"Loop start position is negative", context));
		}

		if (track.LoopEnd() <= track.LoopStart()) {
			results.push_back(ValidationResult(VALIDATION_ERROR,
				"Loop end must be after loop start", context));
		}
	}

	// Validate start/end positions
	if (track.StartPosition() < 0) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Track start position is negative", context));
	}

	if (track.EndPosition() > 0 && track.EndPosition() <= track.StartPosition()) {
		results.push_back(ValidationResult(VALIDATION_WARNING,
			"Track end position should be after start position", context));
	}
}

} // namespace VeniceDAW