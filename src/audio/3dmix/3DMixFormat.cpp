/*
 * 3DMixFormat.cpp - BeOS 3dmix file format implementation
 */

#include "3DMixFormat.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../AudioLogging.h"

namespace VeniceDAW {

// Static constants
const char* Format3DMix::kBeOSHomePath = "/boot/home/";
const char* Format3DMix::kBeOSOptionalPath = "/boot/optional/";
const char* Format3DMix::kBeOSDesktopPath = "/boot/Desktop/";
const char* Format3DMix::kBeOSAppsPath = "/boot/apps/";

// =====================================
// Coordinate3D Implementation
// =====================================

bool Coordinate3D::IsValidBeOSCoordinate() const
{
	return (x >= Format3DMix::kMinCoordinate && x <= Format3DMix::kMaxCoordinate &&
	        y >= Format3DMix::kMinCoordinate && y <= Format3DMix::kMaxCoordinate &&
	        z >= Format3DMix::kMinCoordinate && z <= Format3DMix::kMaxCoordinate);
}

float Coordinate3D::Magnitude() const
{
	return sqrtf(x*x + y*y + z*z);
}

void Coordinate3D::Normalize()
{
	float mag = Magnitude();
	if (mag > 0.0f) {
		x /= mag;
		y /= mag;
		z /= mag;
	}
}

Coordinate3D Coordinate3D::Normalized() const
{
	Coordinate3D result = *this;
	result.Normalize();
	return result;
}

// =====================================
// SphericalCoordinate Implementation
// =====================================

Coordinate3D SphericalCoordinate::ToCartesian() const
{
	float radiansAzimuth = azimuth * (M_PI / 180.0f);
	float radiansElevation = elevation * (M_PI / 180.0f);

	float x = radius * cosf(radiansElevation) * cosf(radiansAzimuth);
	float y = radius * sinf(radiansElevation);
	float z = radius * cosf(radiansElevation) * sinf(radiansAzimuth);

	return Coordinate3D(x, y, z);
}

SphericalCoordinate SphericalCoordinate::FromCartesian(const Coordinate3D& coord)
{
	SphericalCoordinate result;

	result.radius = coord.Magnitude();

	if (result.radius > 0.0f) {
		result.elevation = asinf(coord.y / result.radius) * (180.0f / M_PI);
		result.azimuth = atan2f(coord.z, coord.x) * (180.0f / M_PI);
	} else {
		result.elevation = 0.0f;
		result.azimuth = 0.0f;
	}

	return result;
}

// =====================================
// AudioFormat3DMix Implementation
// =====================================

media_format AudioFormat3DMix::ToMediaFormat() const
{
	media_format format = {}; // Zero-initialize in C++11 style

	format.type = B_MEDIA_RAW_AUDIO;
	format.u.raw_audio.format = (bitDepth == 32) ? media_raw_audio_format::B_AUDIO_FLOAT :
	                           (bitDepth == 24) ? media_raw_audio_format::B_AUDIO_INT :
	                           (bitDepth == 16) ? media_raw_audio_format::B_AUDIO_SHORT :
	                                            media_raw_audio_format::B_AUDIO_UCHAR;

	format.u.raw_audio.frame_rate = sampleRate;
	format.u.raw_audio.channel_count = channels;
	format.u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;

	// Calculate buffer size
	format.u.raw_audio.buffer_size = sampleRate * channels * (bitDepth / 8) / 10; // 100ms buffer

	return format;
}

bool AudioFormat3DMix::IsValid() const
{
	return (sampleRate > 0 && sampleRate <= 192000 &&
	        bitDepth >= 8 && bitDepth <= 32 &&
	        channels >= 1 && channels <= 8 &&
	        fileSize >= 0);
}

float AudioFormat3DMix::CalculateDuration() const
{
	if (sampleRate <= 0 || channels <= 0 || bitDepth <= 0)
		return 0.0f;

	int32 bytesPerSample = (bitDepth + 7) / 8; // Round up to nearest byte
	int32 totalSamples = fileSize / (channels * bytesPerSample);
	return (float)totalSamples / sampleRate;
}

// =====================================
// Track3DMix Implementation
// =====================================

Track3DMix::Track3DMix()
	: fAudioFilePath("")
	, fTrackName("")
	, fVolume(1.0f)
	, fBalance(0.0f)
	, fEnabled(true)
	, fPosition(0.0f, 0.0f, 0.0f)
	, fStartPosition(0)
	, fEndPosition(0)
	, fLoopStart(0)
	, fLoopEnd(0)
	, fLoopEnabled(false)
	, fAudioFormat()
	, fReverbLevel(0.0f)
	, fDistanceAttenuation(1.0f)
	, fDopplerShift(0.0f)
	, fWindowX(100)
	, fWindowY(100)
	, fWindowVisible(true)
	, fRawBMessageData()
{
}

Track3DMix::~Track3DMix()
{
}

SphericalCoordinate Track3DMix::GetSphericalPosition() const
{
	// Convert from BeOS coordinate system to normalized spherical
	Coordinate3D normalizedPos = fPosition;

	// Normalize from BeOS range (-12.0 to +12.0) to (-1.0 to +1.0)
	normalizedPos.x = fmaxf(-1.0f, fminf(1.0f, normalizedPos.x / Format3DMix::kMaxCoordinate));
	normalizedPos.y = fmaxf(-1.0f, fminf(1.0f, normalizedPos.y / Format3DMix::kMaxCoordinate));
	normalizedPos.z = fmaxf(-1.0f, fminf(1.0f, normalizedPos.z / Format3DMix::kMaxCoordinate));

	return SphericalCoordinate::FromCartesian(normalizedPos);
}

void Track3DMix::SetSphericalPosition(const SphericalCoordinate& spherical)
{
	// Convert from spherical to BeOS Cartesian coordinates
	Coordinate3D cartesian = spherical.ToCartesian();

	// Scale to BeOS coordinate range
	fPosition.x = cartesian.x * Format3DMix::kMaxCoordinate;
	fPosition.y = cartesian.y * Format3DMix::kMaxCoordinate;
	fPosition.z = cartesian.z * Format3DMix::kMaxCoordinate;
}

bool Track3DMix::IsValid() const
{
	return (fAudioFilePath.Length() > 0 &&
	        fVolume >= 0.0f && fVolume <= 10.0f &&
	        fBalance >= -1.0f && fBalance <= 1.0f &&
	        fPosition.IsValidBeOSCoordinate() &&
	        fAudioFormat.IsValid());
}

void Track3DMix::PrintToStream() const
{
	AUDIO_LOG_INFO("3DMix", "Track: %s", fTrackName.String());
	AUDIO_LOG_INFO("3DMix", "  File: %s", fAudioFilePath.String());
	AUDIO_LOG_INFO("3DMix", "  Volume: %.3f, Balance: %.3f, Enabled: %s",
	               fVolume, fBalance, fEnabled ? "Yes" : "No");
	AUDIO_LOG_INFO("3DMix", "  Position: (%.2f, %.2f, %.2f)",
	               fPosition.x, fPosition.y, fPosition.z);
	AUDIO_LOG_INFO("3DMix", "  Format: %dHz, %d-bit, %d channels",
	               fAudioFormat.sampleRate, fAudioFormat.bitDepth, fAudioFormat.channels);

	if (fLoopEnabled) {
		AUDIO_LOG_INFO("3DMix", "  Loop: %d - %d samples", fLoopStart, fLoopEnd);
	}
}

// =====================================
// Project3DMix Implementation
// =====================================

Project3DMix::Project3DMix()
	: fProjectName("")
	, fBasePath("")
	, fTracks()
	, fMasterVolume(1.0f)
	, fMasterEnabled(true)
	, fListenerPosition(0.0f, 0.0f, 0.0f)
	, fListenerOrientationYaw(0.0f)
	, fListenerOrientationPitch(0.0f)
	, fProjectSampleRate(Format3DMix::kDefaultSampleRate)
	, fProjectLength(0)
	, fFormatVersion(1)
	, fCreatedWithVersion("")
{
}

Project3DMix::~Project3DMix()
{
	MakeEmpty();
}

Track3DMix* Project3DMix::TrackAt(int32 index) const
{
	return static_cast<Track3DMix*>(fTracks.ItemAt(index));
}

bool Project3DMix::AddTrack(Track3DMix* track)
{
	if (!track || !track->IsValid())
		return false;

	return fTracks.AddItem(track);
}

bool Project3DMix::RemoveTrack(int32 index)
{
	Track3DMix* track = TrackAt(index);
	if (!track)
		return false;

	if (fTracks.RemoveItem(index)) {
		delete track;
		return true;
	}

	return false;
}

void Project3DMix::MakeEmpty()
{
	for (int32 i = 0; i < fTracks.CountItems(); i++) {
		Track3DMix* track = TrackAt(i);
		delete track;
	}
	fTracks.MakeEmpty();
}

bool Project3DMix::IsValid() const
{
	if (fProjectName.Length() == 0 || fBasePath.Length() == 0)
		return false;

	if (fProjectSampleRate <= 0 || fProjectSampleRate > 192000)
		return false;

	// Validate all tracks
	for (int32 i = 0; i < CountTracks(); i++) {
		Track3DMix* track = TrackAt(i);
		if (!track || !track->IsValid())
			return false;
	}

	return true;
}

int32 Project3DMix::CalculateTotalSamples() const
{
	int32 totalSamples = 0;

	for (int32 i = 0; i < CountTracks(); i++) {
		Track3DMix* track = TrackAt(i);
		if (track) {
			const AudioFormat3DMix& format = track->GetAudioFormat();
			int32 bytesPerSample = (format.bitDepth + 7) / 8;
			int32 trackSamples = format.fileSize / (format.channels * bytesPerSample);
			totalSamples = fmaxf(totalSamples, trackSamples);
		}
	}

	return totalSamples;
}

float Project3DMix::CalculateTotalDuration() const
{
	if (fProjectSampleRate <= 0)
		return 0.0f;

	return (float)CalculateTotalSamples() / fProjectSampleRate;
}

void Project3DMix::PrintToStream() const
{
	AUDIO_LOG_INFO("3DMix", "Project: %s", fProjectName.String());
	AUDIO_LOG_INFO("3DMix", "  Base Path: %s", fBasePath.String());
	AUDIO_LOG_INFO("3DMix", "  Tracks: %d", CountTracks());
	AUDIO_LOG_INFO("3DMix", "  Sample Rate: %d Hz", fProjectSampleRate);
	AUDIO_LOG_INFO("3DMix", "  Duration: %.2f seconds", CalculateTotalDuration());
	AUDIO_LOG_INFO("3DMix", "  Master Volume: %.3f (%s)",
	               fMasterVolume, fMasterEnabled ? "Enabled" : "Disabled");
	AUDIO_LOG_INFO("3DMix", "  Listener Position: (%.2f, %.2f, %.2f)",
	               fListenerPosition.x, fListenerPosition.y, fListenerPosition.z);

	for (int32 i = 0; i < CountTracks(); i++) {
		Track3DMix* track = TrackAt(i);
		if (track) {
			AUDIO_LOG_INFO("3DMix", "Track %d:", i + 1);
			track->PrintToStream();
		}
	}
}

} // namespace VeniceDAW