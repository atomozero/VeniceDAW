/*
 * 3DMixFormat.h - BeOS 3dmix file format support for VeniceDAW
 * Complete implementation of legacy BeOS 3dmix project import
 */

#ifndef THREEDMIX_FORMAT_H
#define THREEDMIX_FORMAT_H

#ifdef __HAIKU__
	#include <OS.h>
	#include <support/String.h>
	#include <support/List.h>
	#include <app/Message.h>
	#include <media/MediaDefs.h>
#else
	// Cross-platform headers for syntax checking
	#include "../../testing/HaikuMockHeaders.h"
#endif
#include <vector>
#include <string>

namespace VeniceDAW {

// Forward declarations
class CoordinateSystemMapper;
class AudioPathResolver;

/*
 * 3dmix file format constants from BeOS protocol analysis
 */
class Format3DMix {
public:
	// Magic numbers and type codes
	static const uint32 kMagicNumber = 'MAST';
	static const uint32 kTypeLong = 'GNOL';		// int32 values
	static const uint32 kTypeBool = 'LOOB';		// boolean values
	static const uint32 kTypeReply = 'YLPR';		// reply messages
	static const uint32 kTypeFileRef = '1BOF';		// file references

	// Coordinate system constants
	static constexpr float kMaxCoordinate = 12.0f;
	static constexpr float kMinCoordinate = -12.0f;

	// Audio format constants
	static const int32 kDefaultSampleRate = 44100;
	static const int32 kDefaultBitDepth = 16;
	static const int32 kDefaultChannels = 2;

	// Path constants
	static const char* kBeOSHomePath;
	static const char* kBeOSOptionalPath;
	static const char* kBeOSDesktopPath;
	static const char* kBeOSAppsPath;
};

/*
 * 3D coordinate representation with conversion support
 */
struct Coordinate3D {
	float x, y, z;

	Coordinate3D() : x(0.0f), y(0.0f), z(0.0f) {}
	Coordinate3D(float x, float y, float z) : x(x), y(y), z(z) {}

	// Validation for BeOS coordinate range
	bool IsValidBeOSCoordinate() const;

	// Magnitude calculation
	float Magnitude() const;

	// Normalization
	void Normalize();
	Coordinate3D Normalized() const;
};

/*
 * Spherical coordinate representation for modern audio positioning
 */
struct SphericalCoordinate {
	float radius;		// Distance from center (0.0-1.0 normalized)
	float azimuth;		// Horizontal angle in degrees (-180 to +180)
	float elevation;	// Vertical angle in degrees (-90 to +90)

	SphericalCoordinate() : radius(0.0f), azimuth(0.0f), elevation(0.0f) {}
	SphericalCoordinate(float r, float az, float el) : radius(r), azimuth(az), elevation(el) {}

	// Convert to Cartesian
	Coordinate3D ToCartesian() const;

	// Create from Cartesian
	static SphericalCoordinate FromCartesian(const Coordinate3D& coord);
};

/*
 * Audio format information for 3dmix tracks
 */
struct AudioFormat3DMix {
	int32 sampleRate;
	int32 bitDepth;
	int32 channels;
	int32 fileSize;
	bool isRawFormat;

	AudioFormat3DMix()
		: sampleRate(Format3DMix::kDefaultSampleRate)
		, bitDepth(Format3DMix::kDefaultBitDepth)
		, channels(Format3DMix::kDefaultChannels)
		, fileSize(0)
		, isRawFormat(true)
	{}

	// Convert to modern media_format
	media_format ToMediaFormat() const;

	// Validate format parameters
	bool IsValid() const;

	// Calculate duration in seconds
	float CalculateDuration() const;
};

/*
 * Complete track information from 3dmix file
 */
class Track3DMix {
public:
	Track3DMix();
	~Track3DMix();

	// File information
	const BString& AudioFilePath() const { return fAudioFilePath; }
	void SetAudioFilePath(const char* path) { fAudioFilePath.SetTo(path); }

	const BString& TrackName() const { return fTrackName; }
	void SetTrackName(const char* name) { fTrackName.SetTo(name); }

	// Audio parameters
	float Volume() const { return fVolume; }
	void SetVolume(float volume) { fVolume = volume; }

	float Balance() const { return fBalance; }
	void SetBalance(float balance) { fBalance = balance; }

	bool IsEnabled() const { return fEnabled; }
	void SetEnabled(bool enabled) { fEnabled = enabled; }

	// 3D positioning (BeOS coordinates)
	const Coordinate3D& Position() const { return fPosition; }
	void SetPosition(const Coordinate3D& pos) { fPosition = pos; }
	void SetPosition(float x, float y, float z) { fPosition = Coordinate3D(x, y, z); }

	// Modern spherical coordinates (converted)
	SphericalCoordinate GetSphericalPosition() const;
	void SetSphericalPosition(const SphericalCoordinate& spherical);

	// Playback control
	int32 StartPosition() const { return fStartPosition; }
	void SetStartPosition(int32 pos) { fStartPosition = pos; }

	int32 EndPosition() const { return fEndPosition; }
	void SetEndPosition(int32 pos) { fEndPosition = pos; }

	int32 LoopStart() const { return fLoopStart; }
	void SetLoopStart(int32 pos) { fLoopStart = pos; }

	int32 LoopEnd() const { return fLoopEnd; }
	void SetLoopEnd(int32 pos) { fLoopEnd = pos; }

	bool IsLoopEnabled() const { return fLoopEnabled; }
	void SetLoopEnabled(bool enabled) { fLoopEnabled = enabled; }

	// Audio format
	const AudioFormat3DMix& GetAudioFormat() const { return fAudioFormat; }
	void SetAudioFormat(const AudioFormat3DMix& format) { fAudioFormat = format; }

	// Effects parameters (normalized 0.0-1.0)
	float ReverbLevel() const { return fReverbLevel; }
	void SetReverbLevel(float level) { fReverbLevel = level; }

	float DistanceAttenuation() const { return fDistanceAttenuation; }
	void SetDistanceAttenuation(float attenuation) { fDistanceAttenuation = attenuation; }

	float DopplerShift() const { return fDopplerShift; }
	void SetDopplerShift(float shift) { fDopplerShift = shift; }

	// GUI state
	int32 WindowX() const { return fWindowX; }
	int32 WindowY() const { return fWindowY; }
	void SetWindowPosition(int32 x, int32 y) { fWindowX = x; fWindowY = y; }

	bool IsWindowVisible() const { return fWindowVisible; }
	void SetWindowVisible(bool visible) { fWindowVisible = visible; }

	// Raw BMessage data for advanced parsing
	const std::vector<uint8>& GetRawBMessageData() const { return fRawBMessageData; }
	void SetRawBMessageData(const std::vector<uint8>& data) { fRawBMessageData = data; }

	// Validation
	bool IsValid() const;

	// Debug information
	void PrintToStream() const;

private:
	// File information
	BString fAudioFilePath;
	BString fTrackName;

	// Audio parameters
	float fVolume;
	float fBalance;
	bool fEnabled;

	// 3D positioning (original BeOS coordinates)
	Coordinate3D fPosition;

	// Playback control
	int32 fStartPosition;
	int32 fEndPosition;
	int32 fLoopStart;
	int32 fLoopEnd;
	bool fLoopEnabled;

	// Audio format
	AudioFormat3DMix fAudioFormat;

	// Effects parameters
	float fReverbLevel;
	float fDistanceAttenuation;
	float fDopplerShift;

	// GUI state
	int32 fWindowX, fWindowY;
	bool fWindowVisible;

	// Raw BMessage data for future extensibility
	std::vector<uint8> fRawBMessageData;
};

/*
 * Complete 3dmix project representation
 */
class Project3DMix {
public:
	Project3DMix();
	~Project3DMix();

	// Project information
	const BString& ProjectName() const { return fProjectName; }
	void SetProjectName(const char* name) { fProjectName.SetTo(name); }

	const BString& BasePath() const { return fBasePath; }
	void SetBasePath(const char* path) { fBasePath.SetTo(path); }

	// Track management
	int32 CountTracks() const { return fTracks.CountItems(); }
	Track3DMix* TrackAt(int32 index) const;
	bool AddTrack(Track3DMix* track);
	bool RemoveTrack(int32 index);
	void MakeEmpty();

	// Project-level parameters
	float MasterVolume() const { return fMasterVolume; }
	void SetMasterVolume(float volume) { fMasterVolume = volume; }

	bool IsMasterEnabled() const { return fMasterEnabled; }
	void SetMasterEnabled(bool enabled) { fMasterEnabled = enabled; }

	// 3D scene parameters (listener position)
	const Coordinate3D& ListenerPosition() const { return fListenerPosition; }
	void SetListenerPosition(const Coordinate3D& pos) { fListenerPosition = pos; }

	float ListenerOrientationYaw() const { return fListenerOrientationYaw; }
	float ListenerOrientationPitch() const { return fListenerOrientationPitch; }
	void SetListenerOrientation(float yaw, float pitch) {
		fListenerOrientationYaw = yaw;
		fListenerOrientationPitch = pitch;
	}

	// Timing and sync
	int32 ProjectSampleRate() const { return fProjectSampleRate; }
	void SetProjectSampleRate(int32 sampleRate) { fProjectSampleRate = sampleRate; }

	int32 ProjectLength() const { return fProjectLength; }
	void SetProjectLength(int32 length) { fProjectLength = length; }

	// Version and compatibility
	uint32 FormatVersion() const { return fFormatVersion; }
	void SetFormatVersion(uint32 version) { fFormatVersion = version; }

	const BString& CreatedWithVersion() const { return fCreatedWithVersion; }
	void SetCreatedWithVersion(const char* version) { fCreatedWithVersion.SetTo(version); }

	// Validation
	bool IsValid() const;

	// Statistics
	int32 CalculateTotalSamples() const;
	float CalculateTotalDuration() const;

	// Debug information
	void PrintToStream() const;

private:
	// Project information
	BString fProjectName;
	BString fBasePath;

	// Track collection
	BList fTracks;

	// Project-level parameters
	float fMasterVolume;
	bool fMasterEnabled;

	// 3D scene parameters
	Coordinate3D fListenerPosition;
	float fListenerOrientationYaw;
	float fListenerOrientationPitch;

	// Timing and sync
	int32 fProjectSampleRate;
	int32 fProjectLength;

	// Version and compatibility
	uint32 fFormatVersion;
	BString fCreatedWithVersion;
};

/*
 * Validation result for project import
 */
enum validation_level {
	VALIDATION_WARNING = 0,
	VALIDATION_ERROR,
	VALIDATION_CRITICAL
};

struct ValidationResult {
	validation_level level;
	BString message;
	BString context;

	ValidationResult(validation_level lvl, const char* msg, const char* ctx = nullptr)
		: level(lvl), message(msg), context(ctx ? ctx : "") {}
};

} // namespace VeniceDAW

#endif // THREEDMIX_FORMAT_H