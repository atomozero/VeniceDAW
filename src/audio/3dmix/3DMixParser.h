/*
 * 3DMixParser.h - Complete BeOS 3dmix file parser
 * Handles BMessage serialization format from vintage BeOS projects
 */

#ifndef THREEDMIX_PARSER_H
#define THREEDMIX_PARSER_H

#include "3DMixFormat.h"
#ifdef __HAIKU__
	#include <storage/File.h>
	#include <app/Message.h>
	#include <support/DataIO.h>
#else
	// Cross-platform headers for syntax checking
	#include "../../testing/HaikuMockHeaders.h"
#endif
#include <vector>
#include <map>

namespace VeniceDAW {

// Custom status codes for 3dmix parsing
#define B_END_OF_DATA (B_GENERAL_ERROR_BASE + 0x1000)

/*
 * Advanced BMessage parser for 3dmix format
 */
class BMessageParser {
public:
	BMessageParser();
	~BMessageParser();

	// Main parsing entry point
	status_t ParseBMessageData(const uint8* data, size_t length, Track3DMix* track);

	// Individual type code handlers
	status_t ParseGNOLValue(const uint8* data, size_t length, int32* value);
	status_t ParseLOOBValue(const uint8* data, size_t length, bool* value);
	status_t ParseYLPRMessage(const uint8* data, size_t length, BMessage* message);
	status_t Parse1BOFFileRef(const uint8* data, size_t length, BString* filePath);

	// Advanced parameter extraction
	status_t ExtractTrackParameters(const BMessage& message, Track3DMix* track);
	status_t ExtractAudioFormat(const BMessage& message, AudioFormat3DMix* format);
	status_t ExtractPositionData(const BMessage& message, Coordinate3D* position);

	// Error handling
	const BString& GetLastError() const { return fLastError; }
	int32 GetErrorCount() const { return fErrorCount; }

private:
	// Internal parsing helpers
	status_t ReadTypeCode(const uint8** data, size_t* remaining, uint32* typeCode);
	status_t ReadDataLength(const uint8** data, size_t* remaining, int32* length);
	status_t ValidateTypeCode(uint32 typeCode);

	// Data extraction helpers
	float ExtractFloat(const uint8* data, bool littleEndian = true);
	int32 ExtractInt32(const uint8* data, bool littleEndian = true);
	bool ExtractBool(const uint8* data);

	// Known field mapping from reverse engineering
	struct FieldMapping {
		const char* fieldName;
		uint32 expectedType;
		void (BMessageParser::*handler)(const uint8*, Track3DMix*);
	};

	static const FieldMapping kKnownFields[];
	static const int32 kKnownFieldCount;

	// Field-specific handlers
	void HandleVolumeField(const uint8* data, Track3DMix* track);
	void HandleBalanceField(const uint8* data, Track3DMix* track);
	void HandleEnabledField(const uint8* data, Track3DMix* track);
	void HandlePositionXField(const uint8* data, Track3DMix* track);
	void HandlePositionYField(const uint8* data, Track3DMix* track);
	void HandlePositionZField(const uint8* data, Track3DMix* track);
	void HandleLoopStartField(const uint8* data, Track3DMix* track);
	void HandleLoopEndField(const uint8* data, Track3DMix* track);
	void HandleSampleRateField(const uint8* data, Track3DMix* track);

	// Error reporting
	void ReportError(const char* error);

	BString fLastError;
	int32 fErrorCount;
};

/*
 * Complete 3dmix file loader implementation
 */
class Legacy3DMixLoader {
public:
	Legacy3DMixLoader();
	~Legacy3DMixLoader();

	// Main loading interface
	status_t LoadProject(const char* filePath);
	status_t LoadProject(BFile* file);

	// Access to loaded project
	const Project3DMix& GetProject() const { return fProject; }
	Project3DMix* DetachProject(); // Transfer ownership

	// Error and validation
	const std::vector<ValidationResult>& GetValidationResults() const { return fValidationResults; }
	const BString& GetLastError() const { return fLastError; }
	bool HasErrors() const;
	bool HasWarnings() const;

	// Loading statistics
	int32 GetLoadedTrackCount() const;
	int32 GetFailedTrackCount() const;
	bigtime_t GetLoadingTime() const { return fLoadingTime; }

private:
	// Core parsing phases
	status_t ParseFileHeader(BDataIO* stream);
	status_t ParseTrackRecords(BDataIO* stream);
	status_t ParseSingleTrackRecord(BDataIO* stream, Track3DMix* track);

	// File format validation
	status_t ValidateMagicNumber(BDataIO* stream);
	status_t ReadTrackCount(BDataIO* stream, int32* trackCount);
	status_t ReadBasePath(BDataIO* stream, BString* basePath);

	// Track parsing pipeline
	status_t ReadAudioFilePath(BDataIO* stream, BString* filePath);
	status_t ReadBMessageData(BDataIO* stream, std::vector<uint8>* data);
	status_t ProcessTrackData(const std::vector<uint8>& data, Track3DMix* track);

	// Path handling (BeOS → Haiku)
	status_t TranslatePath(const BString& beosPath, BString* haikuPath);
	status_t ValidateFilePath(const BString& filePath);
	status_t SearchForFile(const BString& originalPath, BString* foundPath);

	// Audio format detection for RAW files
	status_t DetectAudioFormat(const BString& filePath, AudioFormat3DMix* format);
	status_t AnalyzeRawAudioFile(const BString& filePath, AudioFormat3DMix* format);
	status_t EstimateSampleRate(const uint8* audioData, size_t length, int32* sampleRate);

	// Validation pipeline
	status_t ValidateProject();
	void AddValidationResult(validation_level level, const char* message, const char* context = nullptr);

	// Post-processing
	status_t PostProcessProject();
	status_t CalculateProjectStatistics();

	// Error handling
	void ReportError(const char* error);
	void ReportWarning(const char* warning);

	// Internal state
	Project3DMix fProject;
	BMessageParser fBMessageParser;
	std::vector<ValidationResult> fValidationResults;
	BString fLastError;
	bigtime_t fLoadingTime;
	int32 fLoadedTrackCount;
	int32 fFailedTrackCount;

	// Configuration
	bool fStrictValidation;
	bool fSearchMissingFiles;
	bool fAutoDetectFormat;

	// Path search directories
	std::vector<BString> fSearchPaths;
};

/*
 * Project validation utilities
 */
class ProjectValidator {
public:
	static std::vector<ValidationResult> ValidateProject(const Project3DMix& project);
	static std::vector<ValidationResult> ValidateTrack(const Track3DMix& track, int32 trackIndex);
	static bool IsCoordinateInRange(const Coordinate3D& coord);
	static bool IsPathValid(const BString& path);
	static bool IsAudioFormatSupported(const AudioFormat3DMix& format);

private:
	static void ValidateTrackAudio(const Track3DMix& track, int32 trackIndex,
	                               std::vector<ValidationResult>& results);
	static void ValidateTrackPosition(const Track3DMix& track, int32 trackIndex,
	                                  std::vector<ValidationResult>& results);
	static void ValidateTrackTiming(const Track3DMix& track, int32 trackIndex,
	                                std::vector<ValidationResult>& results);
};

/*
 * Utility functions for 3dmix format support
 */
class Format3DMixUtils {
public:
	// Magic number and format detection
	static bool IsValidMagicNumber(uint32 magic);
	static bool IsPointerFile(const char* filePath);
	static status_t ResolvePointerFile(const char* pointerPath, BString* realPath);

	// Endianness handling
	static uint32 SwapInt32(uint32 value);
	static uint16 SwapInt16(uint16 value);
	static float SwapFloat(float value);

	// Path utilities
	static bool IsBeOSPath(const BString& path);
	static BString ExtractFileName(const BString& path);
	static BString ExtractDirectory(const BString& path);

	// Audio format utilities
	static bool IsRawAudioFile(const BString& filePath);
	static int32 CalculateFrameSize(const AudioFormat3DMix& format);
	static int32 CalculateBufferSize(const AudioFormat3DMix& format, float durationSeconds);

	// Coordinate conversion utilities
	static bool IsValidBeOSCoordinate(float value);
	static float ClampBeOSCoordinate(float value);
	static Coordinate3D ClampBeOSPosition(const Coordinate3D& position);
};

} // namespace VeniceDAW

#endif // THREEDMIX_PARSER_H