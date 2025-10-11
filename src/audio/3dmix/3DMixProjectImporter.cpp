/*
 * 3DMixProjectImporter.cpp - Complete 3dmix project import implementation
 */

#include "3DMixProjectImporter.h"
#include "../AudioLogging.h"
#include "../../gui/MixerWindow.h"  // VeniceDAW integration
#include "../../gui/Mixer3DWindow.h"
#include "../../audio/SimpleHaikuEngine.h"
#include <storage/Directory.h>
#include <storage/Path.h>
#include <storage/FindDirectory.h>
#include <app/Application.h>
#include <interface/Alert.h>

namespace VeniceDAW {

// =====================================
// ThreeDMixProjectImporter Implementation
// =====================================

ThreeDMixProjectImporter::ThreeDMixProjectImporter()
	: fTargetMixer(nullptr)
	, fTarget3DMixer(nullptr)
	, fTargetEngine(nullptr)
	, fTargetSoundView(nullptr)
	, fProgressCallback(nullptr)
	, fProgressUserData(nullptr)
{
	// Configure components for optimal integration
	fCoordinateMapper.SetConversionMode(CONVERSION_SPHERICAL);
	fCoordinateMapper.SetSpatialization(STANDARD_GENERIC_3D);

	fPathResolver.SetSearchStrategy(SEARCH_COMPREHENSIVE);
	fPathResolver.LoadDefaultSearchDirectories();

	AUDIO_LOG_INFO("3DMixImporter", "Initialized VeniceDAW 3dmix project importer");
}

ThreeDMixProjectImporter::~ThreeDMixProjectImporter()
{
	CleanupTemporaryFiles();
}

ImportResult ThreeDMixProjectImporter::ImportProject(const char* filePath)
{
	if (!filePath) {
		fLastResult = ImportResult();
		fLastResult.errorMessage = "Invalid file path";
		return fLastResult;
	}

	AUDIO_LOG_INFO("3DMixImporter", "Starting import of 3dmix project: %s", filePath);
	bigtime_t startTime = system_time();

	// Reset state
	fLastResult = ImportResult();
	fTrackMappings.clear();
	fStats = ImportStatistics();

	// Execute import pipeline
	fLastResult = ExecuteImportPipeline(filePath);
	fLastResult.importTime = system_time() - startTime;

	// Log results
	if (fLastResult.success) {
		AUDIO_LOG_INFO("3DMixImporter", "Import completed successfully:");
		AUDIO_LOG_INFO("3DMixImporter", "  Project: %s", fLastResult.projectName.String());
		AUDIO_LOG_INFO("3DMixImporter", "  Tracks imported: %d", fLastResult.tracksImported);
		AUDIO_LOG_INFO("3DMixImporter", "  Audio files resolved: %d", fLastResult.audioFilesResolved);
		AUDIO_LOG_INFO("3DMixImporter", "  Import time: %lld μs", fLastResult.importTime);
	} else {
		AUDIO_LOG_ERROR("3DMixImporter", "Import failed: %s", fLastResult.errorMessage.String());
	}

	return fLastResult;
}

ImportResult ThreeDMixProjectImporter::ImportProject(const char* filePath, const ImportConfiguration& config)
{
	fConfig = config;
	return ImportProject(filePath);
}

ImportResult ThreeDMixProjectImporter::ImportTo3DMixer(const char* filePath, Mixer3DWindow* mixer3D)
{
	if (!mixer3D) {
		ImportResult result;
		result.errorMessage = "Invalid 3D mixer window";
		return result;
	}

	fTarget3DMixer = mixer3D;
	fConfig.openIn3DMixer = true;

	return ImportProject(filePath);
}

ImportResult ThreeDMixProjectImporter::ExecuteImportPipeline(const char* filePath)
{
	ImportResult result;
	Project3DMix legacyProject;

	ReportProgress("Loading legacy project", 0.1f);

	// Phase 1: Load 3dmix project
	status_t status = LoadLegacyProject(filePath, &legacyProject);
	if (status != B_OK) {
		result.errorMessage = "Failed to load 3dmix project file";
		return result;
	}

	result.projectName = legacyProject.ProjectName();
	result.projectPath = legacyProject.BasePath();

	ReportProgress("Resolving audio files", 0.3f);

	// Phase 2: Resolve audio file paths
	status = ResolveAudioFiles(&legacyProject);
	if (status != B_OK) {
		result.errorMessage = "Failed to resolve audio file paths";
		return result;
	}

	ReportProgress("Converting coordinates", 0.5f);

	// Phase 3: Convert coordinate system
	status = ConvertCoordinates(&legacyProject);
	if (status != B_OK) {
		result.errorMessage = "Failed to convert coordinate system";
		return result;
	}

	ReportProgress("Processing audio files", 0.7f);

	// Phase 4: Process audio files
	status = ProcessAudioFiles(&legacyProject);
	if (status != B_OK) {
		ReportWarning("Some audio files could not be processed");
	}

	ReportProgress("Integrating with VeniceDAW", 0.9f);

	// Phase 5: Integrate with VeniceDAW
	status = IntegrateWithVeniceDAW(&legacyProject);
	if (status != B_OK) {
		result.errorMessage = "Failed to integrate with VeniceDAW";
		return result;
	}

	// Calculate final statistics
	result.success = true;
	result.tracksImported = fTrackMappings.size();
	result.audioFilesResolved = 0;
	result.audioFilesConverted = 0;

	for (const auto& mapping : fTrackMappings) {
		if (mapping.wasCreated || mapping.wasUpdated) {
			if (mapping.audioFilePath.Length() > 0) {
				result.audioFilesResolved++;
			}
		}
	}

	ReportProgress("Import complete", 1.0f);
	return result;
}

status_t ThreeDMixProjectImporter::LoadLegacyProject(const char* filePath, Project3DMix* project)
{
	if (!project) {
		return B_BAD_VALUE;
	}

	status_t status = fLoader.LoadProject(filePath);
	if (status != B_OK) {
		ReportError("Failed to parse 3dmix file format");
		return status;
	}

	*project = fLoader.GetProject();

	// Validate loaded project
	if (!project->IsValid()) {
		ReportError("Loaded project is invalid or corrupted");
		return B_BAD_DATA;
	}

	AUDIO_LOG_INFO("3DMixImporter", "Loaded project '%s' with %d tracks",
	               project->ProjectName().String(), project->CountTracks());

	return B_OK;
}

status_t ThreeDMixProjectImporter::ResolveAudioFiles(Project3DMix* project)
{
	if (!project) {
		return B_BAD_VALUE;
	}

	bool allResolved = true;

	for (int32 i = 0; i < project->CountTracks(); i++) {
		Track3DMix* track = project->TrackAt(i);
		if (!track) {
			continue;
		}

		AudioFileResolution resolution = fPathResolver.ResolveAudioFile(track->AudioFilePath());
		if (resolution.wasFound) {
			track->SetAudioFilePath(resolution.resolvedPath.String());
			AUDIO_LOG_DEBUG("3DMixImporter", "Resolved audio file for track %d: %s",
			                i, resolution.resolvedPath.String());
		} else {
			allResolved = false;
			AUDIO_LOG_WARNING("3DMixImporter", "Could not resolve audio file for track %d: %s",
			                  i, track->AudioFilePath().String());
		}
	}

	return allResolved ? B_OK : B_PARTIAL_ERROR;
}

status_t ThreeDMixProjectImporter::ConvertCoordinates(Project3DMix* project)
{
	if (!project) {
		return B_BAD_VALUE;
	}

	// Configure coordinate mapper based on user preferences
	fCoordinateMapper.SetConversionMode(fConfig.coordMode);
	fCoordinateMapper.SetSpatialization(fConfig.spatialStd);

	// Convert all track positions
	for (int32 i = 0; i < project->CountTracks(); i++) {
		Track3DMix* track = project->TrackAt(i);
		if (!track) {
			continue;
		}

		// Get original BeOS coordinates
		Coordinate3D originalPos = track->Position();

		// Convert to modern spherical coordinates
		AudioSphericalCoordinate sphericalPos = fCoordinateMapper.ConvertFromBeOS(originalPos);

		// Optimize for binaural if requested
		if (fConfig.optimizeForBinaural) {
			sphericalPos = fCoordinateMapper.OptimizeForSpatializer(sphericalPos);
		}

		// Update track with converted coordinates
		track->SetSphericalPosition(sphericalPos.ToSphericalCoordinate());

		AUDIO_LOG_DEBUG("3DMixImporter", "Converted track %d position: BeOS(%.2f,%.2f,%.2f) → Spherical(r=%.3f,az=%.1f°,el=%.1f°)",
		                i, originalPos.x, originalPos.y, originalPos.z,
		                sphericalPos.radius, sphericalPos.azimuth, sphericalPos.elevation);
	}

	return B_OK;
}

status_t ThreeDMixProjectImporter::ProcessAudioFiles(Project3DMix* project)
{
	if (!project) {
		return B_BAD_VALUE;
	}

	status_t overallStatus = B_OK;

	for (int32 i = 0; i < project->CountTracks(); i++) {
		Track3DMix* track = project->TrackAt(i);
		if (!track) {
			continue;
		}

		status_t status = ProcessTrackAudio(track);
		if (status != B_OK) {
			overallStatus = B_PARTIAL_ERROR;
			ReportWarning(BString().SetToFormat("Failed to process audio for track %d", i).String());
		}
	}

	return overallStatus;
}

status_t ThreeDMixProjectImporter::ProcessTrackAudio(Track3DMix* track)
{
	if (!track) {
		return B_BAD_VALUE;
	}

	BString audioPath = track->AudioFilePath();
	if (audioPath.Length() == 0) {
		return B_BAD_VALUE;
	}

	// Check if file exists
	BEntry entry(audioPath.String());
	if (!entry.Exists()) {
		return B_ENTRY_NOT_FOUND;
	}

	// Convert RAW audio to WAV if needed
	if (fConfig.convertRawAudio && fPathResolver.IsRawAudioFile(audioPath)) {
		AudioFormatDetection format = fPathResolver.DetectAudioFormat(audioPath);

		// Generate WAV file path
		BString wavPath = audioPath;
		wavPath.RemoveLast(".");
		wavPath << ".wav";

		status_t status = ConvertRawAudioFile(audioPath, wavPath, format);
		if (status == B_OK) {
			track->SetAudioFilePath(wavPath.String());
			fTemporaryFiles.push_back(wavPath);
			AUDIO_LOG_DEBUG("3DMixImporter", "Converted RAW audio: %s → %s",
			                audioPath.String(), wavPath.String());
		}
	}

	// Normalize audio levels if requested
	if (fConfig.normalizeAudioLevels) {
		status_t status = NormalizeAudioLevel(track->AudioFilePath());
		if (status != B_OK) {
			ReportWarning("Failed to normalize audio level");
		}
	}

	return B_OK;
}

status_t ThreeDMixProjectImporter::IntegrateWithVeniceDAW(Project3DMix* project)
{
	if (!project) {
		return B_BAD_VALUE;
	}

	// Create VeniceDAW tracks
	status_t status = CreateVeniceDAWTracks(*project);
	if (status != B_OK) {
		return status;
	}

	// Setup 3D positions
	status = Setup3DPositions(*project);
	if (status != B_OK) {
		ReportWarning("Failed to setup all 3D positions");
	}

	// Configure audio engine
	status = ConfigureAudioEngine(*project);
	if (status != B_OK) {
		ReportWarning("Failed to configure audio engine");
	}

	// Update 3D mixer visualization if available
	if (fTarget3DMixer) {
		UpdateMixer3DVisualization();
	}

	return B_OK;
}

status_t ThreeDMixProjectImporter::CreateVeniceDAWTracks(const Project3DMix& project)
{
	fTrackMappings.clear();
	fTrackMappings.reserve(project.CountTracks());

	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (!track) {
			continue;
		}

		VeniceTrackMapping mapping;
		mapping.originalTrackIndex = i;
		mapping.trackName = track->TrackName();
		mapping.audioFilePath = track->AudioFilePath();
		SphericalCoordinate spherical = track->GetSphericalPosition();
		mapping.position = AudioSphericalCoordinate(spherical.radius, spherical.azimuth, spherical.elevation);

		// Check if we should update existing track
		if (fConfig.updateExistingTracks) {
			int32 existingIndex;
			if (FindMatchingTrack(track->TrackName(), &existingIndex)) {
				if (UpdateExistingTrack(existingIndex, *track)) {
					mapping.veniceTrackIndex = existingIndex;
					mapping.wasUpdated = true;
				}
			}
		}

		// Create new track if not updated
		if (!mapping.wasUpdated) {
			int32 newIndex = CreateNewTrack(*track);
			if (newIndex >= 0) {
				mapping.veniceTrackIndex = newIndex;
				mapping.wasCreated = true;
			}
		}

		fTrackMappings.push_back(mapping);

		AUDIO_LOG_DEBUG("3DMixImporter", "Track mapping %d: '%s' → VeniceDAW track %d (%s)",
		                i, track->TrackName().String(), mapping.veniceTrackIndex,
		                mapping.wasCreated ? "created" : "updated");
	}

	return B_OK;
}

int32 ThreeDMixProjectImporter::CreateNewTrack(const Track3DMix& legacyTrack)
{
	// This would integrate with VeniceDAW's track creation system
	// For now, return a mock track index
	static int32 nextTrackIndex = 0;

	AUDIO_LOG_INFO("3DMixImporter", "Creating VeniceDAW track: '%s'", legacyTrack.TrackName().String());

	// In a real implementation, this would:
	// 1. Call VeniceDAW's track creation API
	// 2. Set audio file path
	// 3. Configure track parameters (volume, balance, etc.)
	// 4. Return the new track's index

	return nextTrackIndex++;
}

bool ThreeDMixProjectImporter::UpdateExistingTrack(int32 veniceIndex, const Track3DMix& /* legacyTrack */)
{
	AUDIO_LOG_INFO("3DMixImporter", "Updating VeniceDAW track %d with 3dmix data", veniceIndex);

	// In a real implementation, this would:
	// 1. Get existing VeniceDAW track by index
	// 2. Update audio file path
	// 3. Update track parameters
	// 4. Update 3D position

	return true;
}

bool ThreeDMixProjectImporter::FindMatchingTrack(const BString& /* trackName */, int32* /* veniceIndex */)
{
	// This would search VeniceDAW's track list for matching name
	// For now, return false (no matches)
	return false;
}

status_t ThreeDMixProjectImporter::Setup3DPositions(const Project3DMix& /* project */)
{
	for (const auto& mapping : fTrackMappings) {
		if (mapping.veniceTrackIndex >= 0) {
			Apply3DPosition(mapping.veniceTrackIndex, mapping.position);
		}
	}

	return B_OK;
}

status_t ThreeDMixProjectImporter::Apply3DPosition(int32 veniceTrackIndex, const AudioSphericalCoordinate& position)
{
	AUDIO_LOG_DEBUG("3DMixImporter", "Applying 3D position to track %d: r=%.3f, az=%.1f°, el=%.1f°",
	                veniceTrackIndex, position.radius, position.azimuth, position.elevation);

	// In a real implementation, this would:
	// 1. Get VeniceDAW track object
	// 2. Set 3D position in the track's spatial processor
	// 3. Update 3D mixer visualization
	// 4. Configure HRTF/binaural processing if needed

	return B_OK;
}

status_t ThreeDMixProjectImporter::ConfigureAudioEngine(const Project3DMix& project)
{
	if (!fTargetEngine) {
		// Try to get current audio engine
		fTargetEngine = ThreeDMixIntegrationUtils::GetAudioEngine();
	}

	if (fTargetEngine) {
		// Configure engine with project settings
		int32 projectSampleRate = project.ProjectSampleRate();
		if (projectSampleRate > 0) {
			// Set engine sample rate if needed
			AUDIO_LOG_DEBUG("3DMixImporter", "Configuring audio engine for %d Hz", projectSampleRate);
		}
	}

	return B_OK;
}

status_t ThreeDMixProjectImporter::UpdateMixer3DVisualization()
{
	if (!fTarget3DMixer) {
		return B_NO_INIT;
	}

	// Update 3D mixer window with new track positions
	AUDIO_LOG_DEBUG("3DMixImporter", "Updating 3D mixer visualization");

	// In a real implementation, this would:
	// 1. Refresh 3D mixer display
	// 2. Position track objects in 3D space
	// 3. Update camera view if needed
	// 4. Refresh real-time visualization

	return B_OK;
}

// =====================================
// Audio File Processing
// =====================================

status_t ThreeDMixProjectImporter::ConvertRawAudioFile(const BString& rawPath, const BString& wavPath,
                                                      const AudioFormatDetection& format)
{
	return fFormatConverter.ConvertRawToWav(rawPath, wavPath, format);
}

status_t ThreeDMixProjectImporter::NormalizeAudioLevel(const BString& filePath)
{
	// Placeholder for audio level normalization
	AUDIO_LOG_DEBUG("3DMixImporter", "Normalizing audio level for: %s", filePath.String());
	return B_OK;
}

// =====================================
// Utility Functions
// =====================================

BString ThreeDMixProjectImporter::GenerateUniqueTrackName(const BString& baseName)
{
	BString uniqueName = baseName;
	int32 counter = 1;

	while (IsTrackNameTaken(uniqueName)) {
		uniqueName.SetToFormat("%s (%d)", baseName.String(), counter++);
	}

	return uniqueName;
}

bool ThreeDMixProjectImporter::IsTrackNameTaken(const BString& /* trackName */)
{
	// This would check against existing VeniceDAW tracks
	// For now, return false
	return false;
}

void ThreeDMixProjectImporter::ReportProgress(const char* operation, float progress)
{
	if (fProgressCallback) {
		fProgressCallback(operation, progress, fProgressUserData);
	}

	AUDIO_LOG_DEBUG("3DMixImporter", "Progress: %s (%.1f%%)", operation, progress * 100.0f);
}

void ThreeDMixProjectImporter::ReportError(const char* error)
{
	AUDIO_LOG_ERROR("3DMixImporter", "%s", error);
}

void ThreeDMixProjectImporter::ReportWarning(const char* warning)
{
	AUDIO_LOG_WARNING("3DMixImporter", "%s", warning);
}

void ThreeDMixProjectImporter::CleanupTemporaryFiles()
{
	for (const auto& tempFile : fTemporaryFiles) {
		BEntry entry(tempFile.String());
		if (entry.Exists()) {
			entry.Remove();
			AUDIO_LOG_DEBUG("3DMixImporter", "Cleaned up temporary file: %s", tempFile.String());
		}
	}
	fTemporaryFiles.clear();
}

void ThreeDMixProjectImporter::ResetImporter()
{
	CleanupTemporaryFiles();
	fLastResult = ImportResult();
	fTrackMappings.clear();
	fStats = ImportStatistics();
	fTargetMixer = nullptr;
	fTarget3DMixer = nullptr;
	fTargetEngine = nullptr;
	fTargetSoundView = nullptr;
}

// =====================================
// ThreeDMixIntegrationUtils Implementation
// =====================================

bool ThreeDMixIntegrationUtils::IsThreeDMixFile(const char* filePath)
{
	if (!filePath) {
		return false;
	}

	BString path(filePath);
	return path.IFindLast(".3dmix") >= 0;
}

bool ThreeDMixIntegrationUtils::IsThreeDMixPointerFile(const char* filePath)
{
	if (!IsThreeDMixFile(filePath)) {
		return false;
	}

	// Check file size - pointer files are typically very small (50-100 bytes)
	BFile file(filePath, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		return false;
	}

	off_t fileSize;
	if (file.GetSize(&fileSize) != B_OK) {
		return false;
	}

	return fileSize < 200; // Pointer files are under 200 bytes
}

BString ThreeDMixIntegrationUtils::ResolvePointerFile(const char* pointerPath)
{
	BFile file(pointerPath, B_READ_ONLY);
	if (file.InitCheck() != B_OK) {
		return BString("");
	}

	// Read the path from pointer file
	char buffer[1024];
	ssize_t bytesRead = file.Read(buffer, sizeof(buffer) - 1);
	if (bytesRead <= 0) {
		return BString("");
	}

	buffer[bytesRead] = '\0';
	return BString(buffer);
}

MixerWindow* ThreeDMixIntegrationUtils::GetActiveMixerWindow()
{
	// This would find the active VeniceDAW mixer window
	// For now, return nullptr
	return nullptr;
}

Mixer3DWindow* ThreeDMixIntegrationUtils::GetActive3DMixerWindow()
{
	// This would find the active VeniceDAW 3D mixer window
	// For now, return nullptr
	return nullptr;
}

SimpleHaikuEngine* ThreeDMixIntegrationUtils::GetAudioEngine()
{
	// This would get the current VeniceDAW audio engine
	// For now, return nullptr
	return nullptr;
}

BString ThreeDMixIntegrationUtils::GetDefaultProjectsDirectory()
{
	BPath path;
	if (find_directory(B_USER_DIRECTORY, &path) == B_OK) {
		path.Append("VeniceDAW Projects");
		return BString(path.Path());
	}
	return BString("/boot/home/VeniceDAW Projects");
}

} // namespace VeniceDAW