/*
 * CoordinateSystemMapper.cpp - Advanced coordinate conversion implementation
 */

#include "CoordinateSystemMapper.h"
#include "../AudioLogging.h"
#include <math.h>
#include <algorithm>
#include <random>

namespace VeniceDAW {

// =====================================
// AudioSphericalCoordinate Implementation
// =====================================

bool AudioSphericalCoordinate::IsValid() const
{
	return (radius >= 0.0f && radius <= 1.0f &&
	        azimuth >= -180.0f && azimuth <= 180.0f &&
	        elevation >= -90.0f && elevation <= 90.0f &&
	        distance >= 0.0f &&
	        spread >= 0.0f && spread <= 1.0f &&
	        focus >= 0.0f && focus <= 1.0f);
}

SphericalCoordinate AudioSphericalCoordinate::ToSphericalCoordinate() const
{
	return SphericalCoordinate(radius, azimuth, elevation);
}

float AudioSphericalCoordinate::CalculateAttenuation(float maxDistance) const
{
	if (distance <= 0.0f || maxDistance <= 0.0f) {
		return 1.0f;
	}

	// Inverse square law with configurable rolloff
	float normalizedDistance = distance / maxDistance;
	return 1.0f / (1.0f + normalizedDistance * normalizedDistance);
}

float AudioSphericalCoordinate::CalculateDelayMs(float speedOfSound) const
{
	if (speedOfSound <= 0.0f) {
		return 0.0f;
	}

	// Calculate delay in milliseconds
	return (distance / speedOfSound) * 1000.0f;
}

// =====================================
// CoordinateSystemMapper Implementation
// =====================================

CoordinateSystemMapper::CoordinateSystemMapper()
	: fConversionMode(CONVERSION_SPHERICAL)
	, fSpatialization(STANDARD_GENERIC_3D)
	, fListenerYaw(0.0f)
	, fListenerPitch(0.0f)
	, fListenerRoll(0.0f)
	, fListenerPosition(0.0f, 0.0f, 0.0f)
	, fWorkspaceWidth(24.0f)
	, fWorkspaceHeight(24.0f)
	, fWorkspaceDepth(24.0f)
	, fMaxAudibleDistance(10.0f)
	, fMinAudibleDistance(0.1f)
	, fReferenceDistance(1.0f)
	, fRolloffFactor(1.0f)
	, fUseFastMath(true)
	, fCacheResults(false)
	, fUseApproximations(false)
{
	ResetStatistics();
	AUDIO_LOG_DEBUG("CoordinateMapper", "Initialized with spherical conversion mode");
}

CoordinateSystemMapper::~CoordinateSystemMapper()
{
}

void CoordinateSystemMapper::SetListenerOrientation(float yaw, float pitch, float roll)
{
	fListenerYaw = NormalizeAngle(yaw);
	fListenerPitch = ClampAngle(pitch, -90.0f, 90.0f);
	fListenerRoll = NormalizeAngle(roll);

	AUDIO_LOG_DEBUG("CoordinateMapper", "Listener orientation set: yaw=%.1f°, pitch=%.1f°, roll=%.1f°",
	                fListenerYaw, fListenerPitch, fListenerRoll);
}

void CoordinateSystemMapper::SetWorkspaceSize(float width, float height, float depth)
{
	fWorkspaceWidth = fmaxf(1.0f, width);
	fWorkspaceHeight = fmaxf(1.0f, height);
	fWorkspaceDepth = fmaxf(1.0f, depth);

	AUDIO_LOG_DEBUG("CoordinateMapper", "Workspace size set: %.1f × %.1f × %.1f",
	                fWorkspaceWidth, fWorkspaceHeight, fWorkspaceDepth);
}

AudioSphericalCoordinate CoordinateSystemMapper::ConvertFromBeOS(const Coordinate3D& beosCoord)
{
	AudioSphericalCoordinate result;

	// Validate input
	if (!beosCoord.IsValidBeOSCoordinate()) {
		AUDIO_LOG_WARNING("CoordinateMapper", "BeOS coordinate out of range: (%.2f, %.2f, %.2f)",
		                  beosCoord.x, beosCoord.y, beosCoord.z);
	}

	// Apply conversion based on mode
	switch (fConversionMode) {
		case CONVERSION_DIRECT_SCALE:
			result = DirectScaleConversion(beosCoord);
			break;

		case CONVERSION_SPHERICAL:
			result = SphericalConversion(beosCoord);
			break;

		case CONVERSION_CYLINDRICAL:
			result = CylindricalConversion(beosCoord);
			break;

		case CONVERSION_NORMALIZED_CUBE:
			result = NormalizedCubeConversion(beosCoord);
			break;

		case CONVERSION_AMBISONICS:
			result = AmbisonicsConversion(beosCoord);
			break;

		default:
			result = SphericalConversion(beosCoord);
			break;
	}

	// Apply audio-specific optimizations
	ApplyDistanceModel(&result);
	ApplySpreadCalculation(&result);
	ApplySpatializationHints(&result);

	// Validate and clamp result
	result = ClampToValidRange(result);

	// Update statistics
	UpdateStatistics(result);

	AUDIO_LOG_DEBUG("CoordinateMapper", "Converted BeOS(%.2f,%.2f,%.2f) → Spherical(r=%.3f, az=%.1f°, el=%.1f°)",
	                beosCoord.x, beosCoord.y, beosCoord.z, result.radius, result.azimuth, result.elevation);

	return result;
}

Coordinate3D CoordinateSystemMapper::ConvertToBeOS(const AudioSphericalCoordinate& sphericalCoord)
{
	// Convert spherical back to Cartesian
	Coordinate3D cartesian = sphericalCoord.ToSphericalCoordinate().ToCartesian();

	// Scale to BeOS coordinate range
	cartesian.x *= Format3DMix::kMaxCoordinate;
	cartesian.y *= Format3DMix::kMaxCoordinate;
	cartesian.z *= Format3DMix::kMaxCoordinate;

	// Clamp to valid BeOS range
	cartesian.x = fmaxf(Format3DMix::kMinCoordinate, fminf(Format3DMix::kMaxCoordinate, cartesian.x));
	cartesian.y = fmaxf(Format3DMix::kMinCoordinate, fminf(Format3DMix::kMaxCoordinate, cartesian.y));
	cartesian.z = fmaxf(Format3DMix::kMinCoordinate, fminf(Format3DMix::kMaxCoordinate, cartesian.z));

	return cartesian;
}

std::vector<AudioSphericalCoordinate> CoordinateSystemMapper::ConvertProjectTracks(const Project3DMix& project)
{
	std::vector<AudioSphericalCoordinate> results;
	results.reserve(project.CountTracks());

	AUDIO_LOG_INFO("CoordinateMapper", "Converting %d tracks from BeOS coordinates", project.CountTracks());

	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track) {
			AudioSphericalCoordinate spherical = ConvertFromBeOS(track->Position());
			results.push_back(spherical);
		}
	}

	AUDIO_LOG_INFO("CoordinateMapper", "Successfully converted %zu track positions", results.size());
	return results;
}

void CoordinateSystemMapper::ConvertTrackPositions(Project3DMix* project)
{
	if (!project) {
		return;
	}

	for (int32 i = 0; i < project->CountTracks(); i++) {
		Track3DMix* track = project->TrackAt(i);
		if (track) {
			// Convert to spherical and back to get optimized coordinates
			AudioSphericalCoordinate spherical = ConvertFromBeOS(track->Position());
			track->SetSphericalPosition(spherical.ToSphericalCoordinate());
		}
	}
}

// =====================================
// Core Conversion Algorithms
// =====================================

AudioSphericalCoordinate CoordinateSystemMapper::DirectScaleConversion(const Coordinate3D& beosCoord)
{
	// Simple scaling approach (fast but less flexible)
	Coordinate3D normalized = NormalizeBeOSCoordinate(beosCoord);
	SphericalCoordinate spherical = SphericalCoordinate::FromCartesian(normalized);

	AudioSphericalCoordinate result;
	result.radius = spherical.radius;
	result.azimuth = spherical.azimuth;
	result.elevation = spherical.elevation;
	result.distance = spherical.radius * fMaxAudibleDistance;

	return result;
}

AudioSphericalCoordinate CoordinateSystemMapper::SphericalConversion(const Coordinate3D& beosCoord)
{
	// Advanced spherical conversion with audio optimization
	Coordinate3D normalized = NormalizeBeOSCoordinate(beosCoord);

	// Apply listener transformation
	Coordinate3D listenerRelative = ApplyListenerTransform(normalized);

	// Transform to audio space
	Coordinate3D audioSpace = TransformToAudioSpace(listenerRelative);

	// Convert to spherical coordinates
	float distance = audioSpace.Magnitude();
	float azimuth = 0.0f;
	float elevation = 0.0f;

	if (distance > 0.0001f) { // Avoid division by zero
		elevation = asinf(audioSpace.y / distance) * (180.0f / M_PI);
		azimuth = atan2f(audioSpace.z, audioSpace.x) * (180.0f / M_PI);
	}

	// Create result with extended audio parameters
	AudioSphericalCoordinate result;
	result.radius = fminf(1.0f, distance);
	result.azimuth = NormalizeAngle(azimuth);
	result.elevation = ClampAngle(elevation, -90.0f, 90.0f);
	result.distance = distance * fMaxAudibleDistance;

	// Set audio-specific parameters based on position
	result.spread = CalculateSpreadFromDistance(result.distance);
	result.focus = CalculateFocusFromElevation(result.elevation);
	result.requiresHRTF = (result.distance < fMaxAudibleDistance * 0.8f);

	return result;
}

AudioSphericalCoordinate CoordinateSystemMapper::CylindricalConversion(const Coordinate3D& beosCoord)
{
	// Cylindrical coordinates for rotation-heavy applications
	Coordinate3D normalized = NormalizeBeOSCoordinate(beosCoord);

	float rho = sqrtf(normalized.x * normalized.x + normalized.z * normalized.z); // Radial distance
	float phi = atan2f(normalized.z, normalized.x) * (180.0f / M_PI); // Azimuth angle
	float height = normalized.y; // Height (elevation)

	AudioSphericalCoordinate result;
	result.radius = fminf(1.0f, sqrtf(rho * rho + height * height));
	result.azimuth = NormalizeAngle(phi);
	result.elevation = asinf(height / fmaxf(0.0001f, result.radius)) * (180.0f / M_PI);
	result.distance = result.radius * fMaxAudibleDistance;

	return result;
}

AudioSphericalCoordinate CoordinateSystemMapper::AmbisonicsConversion(const Coordinate3D& beosCoord)
{
	// Optimized conversion for Ambisonics encoding
	AudioSphericalCoordinate spherical = SphericalConversion(beosCoord);

	// Ambisonics-specific optimizations
	spherical.spread = 0.0f; // Point sources for Ambisonics
	spherical.focus = 1.0f; // Maximum focus
	spherical.isOmnidirectional = false; // Directional sources
	spherical.requiresHRTF = false; // Ambisonics handles spatialization

	return spherical;
}

// =====================================
// Coordinate Space Transformations
// =====================================

Coordinate3D CoordinateSystemMapper::NormalizeBeOSCoordinate(const Coordinate3D& beosCoord)
{
	// Normalize from BeOS range (-12.0 to +12.0) to (-1.0 to +1.0)
	Coordinate3D normalized;
	normalized.x = fmaxf(-1.0f, fminf(1.0f, beosCoord.x / Format3DMix::kMaxCoordinate));
	normalized.y = fmaxf(-1.0f, fminf(1.0f, beosCoord.y / Format3DMix::kMaxCoordinate));
	normalized.z = fmaxf(-1.0f, fminf(1.0f, beosCoord.z / Format3DMix::kMaxCoordinate));
	return normalized;
}

Coordinate3D CoordinateSystemMapper::ApplyListenerTransform(const Coordinate3D& worldCoord)
{
	// Apply listener orientation transformation
	float yawRad = DegToRad(fListenerYaw);
	float pitchRad = DegToRad(fListenerPitch);

	// Rotation around Y-axis (yaw)
	float cosYaw = cosf(yawRad);
	float sinYaw = sinf(yawRad);

	float x1 = worldCoord.x * cosYaw - worldCoord.z * sinYaw;
	float z1 = worldCoord.x * sinYaw + worldCoord.z * cosYaw;

	// Rotation around X-axis (pitch)
	float cosPitch = cosf(pitchRad);
	float sinPitch = sinf(pitchRad);

	Coordinate3D result;
	result.x = x1;
	result.y = worldCoord.y * cosPitch - z1 * sinPitch;
	result.z = worldCoord.y * sinPitch + z1 * cosPitch;

	return result;
}

Coordinate3D CoordinateSystemMapper::TransformToAudioSpace(const Coordinate3D& normalizedCoord)
{
	// Transform to workspace-relative coordinates
	Coordinate3D result = normalizedCoord;

	// Apply workspace scaling
	result.x *= (fWorkspaceWidth / 24.0f);
	result.y *= (fWorkspaceHeight / 24.0f);
	result.z *= (fWorkspaceDepth / 24.0f);

	return result;
}

// =====================================
// Audio-Specific Optimizations
// =====================================

void CoordinateSystemMapper::ApplyDistanceModel(AudioSphericalCoordinate* coord)
{
	// Apply distance-based modifications
	if (coord->distance < fMinAudibleDistance) {
		coord->distance = fMinAudibleDistance;
		coord->radius = fMinAudibleDistance / fMaxAudibleDistance;
	}

	// Calculate realistic attenuation
	coord->distance = fminf(coord->distance, fMaxAudibleDistance);
	coord->radius = coord->distance / fMaxAudibleDistance;
}

void CoordinateSystemMapper::ApplySpreadCalculation(AudioSphericalCoordinate* coord)
{
	// Calculate source spread based on distance
	coord->spread = CalculateSpreadFromDistance(coord->distance);
}

float CoordinateSystemMapper::CalculateSpreadFromDistance(float distance)
{
	// Closer sources have tighter spread, distant sources spread more
	float normalizedDistance = distance / fMaxAudibleDistance;
	return fminf(1.0f, normalizedDistance * 0.5f);
}

float CoordinateSystemMapper::CalculateFocusFromElevation(float elevation)
{
	// Sources at ear level have maximum focus
	float elevationFactor = 1.0f - (fabsf(elevation) / 90.0f);
	return fmaxf(0.1f, elevationFactor);
}

void CoordinateSystemMapper::ApplySpatializationHints(AudioSphericalCoordinate* coord)
{
	// Set spatialization hints based on standard
	switch (fSpatialization) {
		case STANDARD_BINAURAL:
			coord->requiresHRTF = true;
			coord->isOmnidirectional = false;
			break;

		case STANDARD_AMBISONICS_1ST:
		case STANDARD_AMBISONICS_2ND:
			coord->requiresHRTF = false;
			coord->spread = 0.0f; // Point sources for Ambisonics
			break;

		case STANDARD_VR_SPATIAL:
			coord->requiresHRTF = true;
			coord->isMoving = true; // Assume dynamic positioning in VR
			break;

		default:
			// Generic 3D defaults
			break;
	}
}

// =====================================
// Utility Functions
// =====================================

AudioSphericalCoordinate CoordinateSystemMapper::ClampToValidRange(const AudioSphericalCoordinate& coord)
{
	AudioSphericalCoordinate result = coord;

	result.radius = fmaxf(0.0f, fminf(1.0f, result.radius));
	result.azimuth = NormalizeAngle(result.azimuth);
	result.elevation = ClampAngle(result.elevation, -90.0f, 90.0f);
	result.distance = fmaxf(fMinAudibleDistance, fminf(fMaxAudibleDistance, result.distance));
	result.spread = fmaxf(0.0f, fminf(1.0f, result.spread));
	result.focus = fmaxf(0.0f, fminf(1.0f, result.focus));

	return result;
}

float CoordinateSystemMapper::ClampAngle(float angle, float minAngle, float maxAngle)
{
	return fmaxf(minAngle, fminf(maxAngle, angle));
}

float CoordinateSystemMapper::NormalizeAngle(float angle)
{
	// Normalize to -180° to +180° range
	while (angle > 180.0f) angle -= 360.0f;
	while (angle < -180.0f) angle += 360.0f;
	return angle;
}

void CoordinateSystemMapper::UpdateStatistics(const AudioSphericalCoordinate& coord)
{
	fStats.conversionsPerformed++;
	fStats.averageRadius = (fStats.averageRadius * (fStats.conversionsPerformed - 1) + coord.radius) / fStats.conversionsPerformed;
	fStats.averageElevation = (fStats.averageElevation * (fStats.conversionsPerformed - 1) + coord.elevation) / fStats.conversionsPerformed;

	if (fStats.conversionsPerformed == 1) {
		fStats.minDistance = fStats.maxDistance = coord.distance;
	} else {
		fStats.minDistance = fminf(fStats.minDistance, coord.distance);
		fStats.maxDistance = fmaxf(fStats.maxDistance, coord.distance);
	}

	if (coord.IsInFront()) {
		fStats.frontPositions++;
	} else {
		fStats.rearPositions++;
	}
}

void CoordinateSystemMapper::ResetStatistics()
{
	fStats = ConversionStats();
}

BString CoordinateSystemMapper::CoordinateToString(const AudioSphericalCoordinate& coord)
{
	BString result;
	result.SetToFormat("r=%.3f, az=%.1f°, el=%.1f°, dist=%.2fm, spread=%.2f",
	                   coord.radius, coord.azimuth, coord.elevation, coord.distance, coord.spread);
	return result;
}

// =====================================
// PositionPresets Implementation
// =====================================

std::vector<Coordinate3D> PositionPresets::GenerateCircularPositions(int32 count, float radius)
{
	std::vector<Coordinate3D> positions;
	positions.reserve(count);

	for (int32 i = 0; i < count; i++) {
		float angle = (2.0f * M_PI * i) / count;
		float x = radius * cosf(angle);
		float z = radius * sinf(angle);
		positions.push_back(Coordinate3D(x, 0.0f, z));
	}

	return positions;
}

std::vector<Coordinate3D> PositionPresets::GenerateSpherePositions(int32 count, float radius)
{
	std::vector<Coordinate3D> positions;
	positions.reserve(count);

	// Use golden angle spiral for even distribution
	const float goldenAngle = M_PI * (3.0f - sqrtf(5.0f));

	for (int32 i = 0; i < count; i++) {
		float y = 1.0f - (2.0f * i / (count - 1));
		float radiusAtY = sqrtf(1.0f - y * y);
		float theta = goldenAngle * i;

		float x = cosf(theta) * radiusAtY;
		float z = sinf(theta) * radiusAtY;

		positions.push_back(Coordinate3D(x * radius, y * radius, z * radius));
	}

	return positions;
}

std::vector<Coordinate3D> PositionPresets::GenerateRandomPositions(int32 count)
{
	std::vector<Coordinate3D> positions;
	positions.reserve(count);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(-Format3DMix::kMaxCoordinate, Format3DMix::kMaxCoordinate);

	for (int32 i = 0; i < count; i++) {
		positions.push_back(Coordinate3D(dis(gen), dis(gen), dis(gen)));
	}

	return positions;
}

// =====================================
// Stub implementations for missing methods
// =====================================

AudioSphericalCoordinate CoordinateSystemMapper::NormalizedCubeConversion(const Coordinate3D& /* coord */)
{
	// TODO: Implement normalized cube conversion
	return AudioSphericalCoordinate(0.0f, 0.0f, 0.0f);
}

AudioSphericalCoordinate CoordinateSystemMapper::OptimizeForSpatializer(const AudioSphericalCoordinate& coord)
{
	// TODO: Implement spatialization optimization
	return coord;  // Return unmodified for now
}

} // namespace VeniceDAW