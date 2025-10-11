/*
 * CoordinateSystemMapper.h - Advanced coordinate system conversion for 3dmix
 * Converts BeOS Cartesian coordinates to modern spherical audio positioning
 */

#ifndef COORDINATE_SYSTEM_MAPPER_H
#define COORDINATE_SYSTEM_MAPPER_H

#include "3DMixFormat.h"
#include <support/SupportDefs.h>
#include <vector>

namespace VeniceDAW {

/*
 * Coordinate conversion modes for different audio applications
 */
enum coordinate_conversion_mode {
	CONVERSION_DIRECT_SCALE = 0,		// Simple scaling (fast, preserves layout)
	CONVERSION_SPHERICAL,				// Full spherical conversion (recommended)
	CONVERSION_CYLINDRICAL,				// Cylindrical coordinates (for rotation-heavy apps)
	CONVERSION_NORMALIZED_CUBE,			// Normalized cubic space
	CONVERSION_AMBISONICS				// Ambisonics-ready coordinates
};

/*
 * Audio spatialization standards for modern applications
 */
enum spatialization_standard {
	STANDARD_GENERIC_3D = 0,			// Generic 3D audio
	STANDARD_BINAURAL,					// Binaural/HRTF positioning
	STANDARD_SURROUND_5_1,				// 5.1 surround mapping
	STANDARD_SURROUND_7_1,				// 7.1 surround mapping
	STANDARD_AMBISONICS_1ST,			// 1st order Ambisonics
	STANDARD_AMBISONICS_2ND,			// 2nd order Ambisonics
	STANDARD_VR_SPATIAL,				// VR/AR spatial audio
	STANDARD_GAME_ENGINE				// Game engine compatibility
};

/*
 * Spherical coordinate with extended audio metadata
 */
struct AudioSphericalCoordinate {
	// Core spherical coordinates
	float radius;			// Distance (0.0-1.0 normalized)
	float azimuth;			// Horizontal angle (-180° to +180°)
	float elevation;		// Vertical angle (-90° to +90°)

	// Extended audio parameters
	float distance;			// Absolute distance in meters
	float spread;			// Source width/spread (0.0-1.0)
	float focus;			// Source focus/directivity (0.0-1.0)

	// Spatialization hints
	bool isOmnidirectional;	// True for ambient sources
	bool requiresHRTF;		// True for precise positioning
	bool isMoving;			// True for Doppler effect

	AudioSphericalCoordinate()
		: radius(0.0f), azimuth(0.0f), elevation(0.0f)
		, distance(1.0f), spread(0.0f), focus(1.0f)
		, isOmnidirectional(false), requiresHRTF(true), isMoving(false)
	{}

	AudioSphericalCoordinate(float r, float az, float el)
		: radius(r), azimuth(az), elevation(el)
		, distance(r), spread(0.0f), focus(1.0f)
		, isOmnidirectional(false), requiresHRTF(true), isMoving(false)
	{}

	// Validation
	bool IsValid() const;

	// Conversion to simple spherical
	SphericalCoordinate ToSphericalCoordinate() const;

	// Audio-specific calculations
	float CalculateAttenuation(float maxDistance = 10.0f) const;
	float CalculateDelayMs(float speedOfSound = 343.0f) const;
	bool IsInFront() const { return (azimuth >= -90.0f && azimuth <= 90.0f); }
};

/*
 * Advanced coordinate system mapper with audio optimization
 */
class CoordinateSystemMapper {
public:
	CoordinateSystemMapper();
	~CoordinateSystemMapper();

	// Configuration
	void SetConversionMode(coordinate_conversion_mode mode) { fConversionMode = mode; }
	void SetSpatialization(spatialization_standard standard) { fSpatialization = standard; }
	void SetListenerOrientation(float yaw, float pitch, float roll);
	void SetWorkspaceSize(float width, float height, float depth);

	// Main conversion interface
	AudioSphericalCoordinate ConvertFromBeOS(const Coordinate3D& beosCoord);
	Coordinate3D ConvertToBeOS(const AudioSphericalCoordinate& sphericalCoord);

	// Batch conversion for efficiency
	std::vector<AudioSphericalCoordinate> ConvertProjectTracks(const Project3DMix& project);
	void ConvertTrackPositions(Project3DMix* project);

	// Specialized conversions
	AudioSphericalCoordinate ConvertToAmbisonics(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate ConvertToBinaural(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate ConvertToSurround(const Coordinate3D& beosCoord, int32 channels);

	// Position validation and optimization
	AudioSphericalCoordinate OptimizeForSpatializer(const AudioSphericalCoordinate& coord);
	AudioSphericalCoordinate ClampToValidRange(const AudioSphericalCoordinate& coord);
	bool IsPositionOccluded(const AudioSphericalCoordinate& coord, const AudioSphericalCoordinate& listener);

	// Interactive positioning helpers
	AudioSphericalCoordinate CalculateRelativePosition(const Coordinate3D& sourcePos,
	                                                   const Coordinate3D& listenerPos,
	                                                   float listenerYaw = 0.0f);

	// Distance and attenuation calculations
	float CalculateDistance3D(const Coordinate3D& pos1, const Coordinate3D& pos2);
	float CalculateAttenuation(float distance, float rolloffFactor = 1.0f);
	float CalculateDelayCompensation(float distance, float speedOfSound = 343.0f);

	// Spread and focus calculations
	float CalculateSpreadFromDistance(float distance);
	float CalculateFocusFromElevation(float elevation);

	// Workspace management
	bool IsPositionInWorkspace(const Coordinate3D& position);
	Coordinate3D ClampToWorkspace(const Coordinate3D& position);
	std::vector<Coordinate3D> GenerateGridPositions(int32 count);

	// Motion and animation support
	AudioSphericalCoordinate InterpolatePosition(const AudioSphericalCoordinate& from,
	                                             const AudioSphericalCoordinate& to,
	                                             float progress);
	AudioSphericalCoordinate CalculateVelocity(const AudioSphericalCoordinate& prevPos,
	                                          const AudioSphericalCoordinate& currentPos,
	                                          float deltaTime);

	// Debug and visualization
	void PrintConversionInfo(const Coordinate3D& beosCoord,
	                        const AudioSphericalCoordinate& sphericalCoord);
	BString CoordinateToString(const AudioSphericalCoordinate& coord);

	// Statistics and analysis
	struct ConversionStats {
		int32 conversionsPerformed;
		float averageRadius;
		float averageElevation;
		float minDistance;
		float maxDistance;
		int32 frontPositions;
		int32 rearPositions;
	};
	ConversionStats GetConversionStatistics() const { return fStats; }
	void ResetStatistics();

private:
	// Core conversion algorithms
	AudioSphericalCoordinate DirectScaleConversion(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate SphericalConversion(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate CylindricalConversion(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate NormalizedCubeConversion(const Coordinate3D& beosCoord);
	AudioSphericalCoordinate AmbisonicsConversion(const Coordinate3D& beosCoord);

	// Coordinate space transformations
	Coordinate3D NormalizeBeOSCoordinate(const Coordinate3D& beosCoord);
	Coordinate3D ApplyListenerTransform(const Coordinate3D& worldCoord);
	Coordinate3D TransformToAudioSpace(const Coordinate3D& normalizedCoord);

	// Audio-specific optimizations
	void ApplyDistanceModel(AudioSphericalCoordinate* coord);
	void ApplySpreadCalculation(AudioSphericalCoordinate* coord);
	void ApplySpatializationHints(AudioSphericalCoordinate* coord);

	// Validation helpers
	float ClampAngle(float angle, float minAngle, float maxAngle);
	float NormalizeAngle(float angle); // Convert to -180° to +180° range
	float DegToRad(float degrees) { return degrees * (M_PI / 180.0f); }
	float RadToDeg(float radians) { return radians * (180.0f / M_PI); }

	// Statistics tracking
	void UpdateStatistics(const AudioSphericalCoordinate& coord);

	// Configuration
	coordinate_conversion_mode fConversionMode;
	spatialization_standard fSpatialization;

	// Listener state
	float fListenerYaw;
	float fListenerPitch;
	float fListenerRoll;
	Coordinate3D fListenerPosition;

	// Workspace configuration
	float fWorkspaceWidth;
	float fWorkspaceHeight;
	float fWorkspaceDepth;

	// Audio parameters
	float fMaxAudibleDistance;
	float fMinAudibleDistance;
	float fReferenceDistance;
	float fRolloffFactor;

	// Performance optimization
	bool fUseFastMath;
	bool fCacheResults;
	bool fUseApproximations;

	// Statistics
	mutable ConversionStats fStats;
};

/*
 * Specialized coordinate mappers for different audio standards
 */
class AmbisonicsMapper {
public:
	static AudioSphericalCoordinate ConvertToAmbisonics(const Coordinate3D& beosCoord, int32 order = 1);
	static std::vector<float> CalculateAmbisonicsCoefficients(const AudioSphericalCoordinate& coord, int32 order);
	static Coordinate3D CalculateOptimalListenerPosition(const std::vector<Coordinate3D>& sources);
};

class BinauralMapper {
public:
	static AudioSphericalCoordinate ConvertToBinaural(const Coordinate3D& beosCoord);
	static void CalculateHRTFParameters(const AudioSphericalCoordinate& coord, float* leftGain, float* rightGain, float* delayMs);
	static AudioSphericalCoordinate OptimizeForHRTF(const AudioSphericalCoordinate& coord);
};

class SurroundMapper {
public:
	static AudioSphericalCoordinate ConvertToSurround(const Coordinate3D& beosCoord, int32 channels);
	static std::vector<float> CalculateSpeakerGains(const AudioSphericalCoordinate& coord, int32 channels);
	static AudioSphericalCoordinate FindClosestSpeakerPosition(const AudioSphericalCoordinate& coord, int32 channels);
};

/*
 * Position presets for common audio setups
 */
class PositionPresets {
public:
	// Standard stereo positions
	static Coordinate3D StereoLeft() { return Coordinate3D(-6.0f, 0.0f, 0.0f); }
	static Coordinate3D StereoRight() { return Coordinate3D(6.0f, 0.0f, 0.0f); }
	static Coordinate3D StereoCenter() { return Coordinate3D(0.0f, 0.0f, 0.0f); }

	// Surround sound positions (5.1)
	static Coordinate3D SurroundFrontLeft() { return Coordinate3D(-4.0f, 0.0f, 6.0f); }
	static Coordinate3D SurroundFrontRight() { return Coordinate3D(4.0f, 0.0f, 6.0f); }
	static Coordinate3D SurroundCenter() { return Coordinate3D(0.0f, 0.0f, 8.0f); }
	static Coordinate3D SurroundRearLeft() { return Coordinate3D(-4.0f, 0.0f, -6.0f); }
	static Coordinate3D SurroundRearRight() { return Coordinate3D(4.0f, 0.0f, -6.0f); }
	static Coordinate3D SurroundLFE() { return Coordinate3D(0.0f, -3.0f, 0.0f); }

	// Creative positions
	static Coordinate3D Overhead() { return Coordinate3D(0.0f, 12.0f, 0.0f); }
	static Coordinate3D Underground() { return Coordinate3D(0.0f, -12.0f, 0.0f); }
	static Coordinate3D FarDistance() { return Coordinate3D(0.0f, 0.0f, -12.0f); }
	static Coordinate3D CloseDistance() { return Coordinate3D(0.0f, 0.0f, 12.0f); }

	// Generate position arrays
	static std::vector<Coordinate3D> GenerateCircularPositions(int32 count, float radius = 8.0f);
	static std::vector<Coordinate3D> GenerateSpherePositions(int32 count, float radius = 8.0f);
	static std::vector<Coordinate3D> GenerateRandomPositions(int32 count);
};

} // namespace VeniceDAW

#endif // COORDINATE_SYSTEM_MAPPER_H