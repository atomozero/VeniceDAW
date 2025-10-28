/*
 * test_3dmix_parser.cpp - Standalone test for 3dmix parser
 *
 * This program loads a .3dmix project file and prints all parsed data
 * to verify the parser is working correctly before GUI integration.
 */

#include "src/audio/3dmix/3DMixParser.h"
#include "src/audio/3dmix/3DMixFormat.h"
#include <stdio.h>
#include <Application.h>

using namespace VeniceDAW;

void PrintProjectInfo(const Project3DMix& project)
{
	printf("\n");
	printf("=====================================\n");
	printf("3DMix Project Information\n");
	printf("=====================================\n");
	printf("Project Name: %s\n", project.ProjectName().String());
	printf("Base Path: %s\n", project.BasePath().String());
	printf("Track Count: %d\n", project.CountTracks());
	printf("Master Volume: %.2f\n", project.MasterVolume());
	printf("Master Enabled: %s\n", project.IsMasterEnabled() ? "Yes" : "No");
	printf("Sample Rate: %.0f Hz\n", project.ProjectSampleRate());
	printf("Project Length: %.2f seconds\n", project.ProjectLength());

	Coordinate3D listenerPos = project.ListenerPosition();
	printf("Listener Position: (%.2f, %.2f, %.2f)\n",
	       listenerPos.x, listenerPos.y, listenerPos.z);

	float yaw = project.ListenerOrientationYaw();
	float pitch = project.ListenerOrientationPitch();
	printf("Listener Orientation: Yaw=%.2f°, Pitch=%.2f°\n", yaw, pitch);

	printf("\n");
}

void PrintTrackInfo(const Track3DMix* track, int32 index)
{
	printf("--- Track #%d ---\n", index + 1);
	printf("  Name: %s\n", track->TrackName().String());
	printf("  Audio File: %s\n", track->AudioFilePath().String());

	Coordinate3D pos = track->Position();
	printf("  3D Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);

	printf("  Volume: %.3f\n", track->Volume());
	printf("  Balance: %.3f\n", track->Balance());
	printf("  Enabled: %s\n", track->IsEnabled() ? "Yes" : "No");

	// Timeline positions
	int32 startPos = track->StartPosition();
	int32 endPos = track->EndPosition();

	const AudioFormat3DMix& format = track->GetAudioFormat();
	float sampleRate = format.sampleRate > 0 ? format.sampleRate : 44100.0f;

	float startTime = startPos / sampleRate;
	float endTime = endPos / sampleRate;

	printf("  Timeline Start: %d samples (%.3f seconds)\n", startPos, startTime);
	printf("  Timeline End: %d samples (%.3f seconds)\n", endPos, endTime);
	printf("  Duration: %.3f seconds\n", endTime - startTime);

	// Loop points
	int32 loopStart = track->LoopStart();
	int32 loopEnd = track->LoopEnd();
	if (loopStart > 0 || loopEnd > 0) {
		printf("  Loop Start: %d samples (%.3f seconds)\n", loopStart, loopStart / sampleRate);
		printf("  Loop End: %d samples (%.3f seconds)\n", loopEnd, loopEnd / sampleRate);
		printf("  Loop Enabled: %s\n", track->IsLoopEnabled() ? "Yes" : "No");
	}

	// Audio format
	printf("  Audio Format:\n");
	printf("    Sample Rate: %d Hz\n", format.sampleRate);
	printf("    Bit Depth: %d bits\n", format.bitDepth);
	printf("    Channels: %d\n", format.channels);
	printf("    File Size: %d bytes\n", format.fileSize);

	printf("\n");
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("Usage: %s <path-to-3dmix-file>\n", argv[0]);
		printf("\nExample:\n");
		printf("  %s /boot/home/Desktop/3D_Mixes/the-lynx/the-lynx-3dmix/the-lynx.3dmix\n", argv[0]);
		return 1;
	}

	BApplication app("application/x-vnd.VeniceDAW-3DMixParserTest");

	const char* projectPath = argv[1];

	printf("\n");
	printf("=====================================\n");
	printf("3DMix Parser Test\n");
	printf("=====================================\n");
	printf("Loading: %s\n", projectPath);
	printf("\n");

	// Create loader and load project
	Legacy3DMixLoader loader;
	status_t status = loader.LoadProject(projectPath);

	if (status != B_OK) {
		printf("❌ FAILED to load project!\n");
		printf("Error: %s\n", loader.GetLastError().String());
		return 1;
	}

	printf("✅ Project loaded successfully!\n");
	printf("   Loaded tracks: %d\n", loader.GetLoadedTrackCount());
	printf("   Failed tracks: %d\n", loader.GetFailedTrackCount());
	printf("   Loading time: %ld μs\n", (long)loader.GetLoadingTime());
	printf("\n");

	// Get project
	const Project3DMix& project = loader.GetProject();

	// Print project info
	PrintProjectInfo(project);

	// Print all tracks
	printf("=====================================\n");
	printf("Track Details\n");
	printf("=====================================\n");
	printf("\n");

	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track) {
			PrintTrackInfo(track, i);
		}
	}

	// Print validation results
	const std::vector<ValidationResult>& validationResults = loader.GetValidationResults();
	if (!validationResults.empty()) {
		printf("=====================================\n");
		printf("Validation Results\n");
		printf("=====================================\n");
		printf("\n");

		for (size_t i = 0; i < validationResults.size(); i++) {
			const ValidationResult& result = validationResults[i];

			const char* levelStr = "UNKNOWN";
			switch (result.level) {
				case VALIDATION_WARNING: levelStr = "WARNING"; break;
				case VALIDATION_ERROR: levelStr = "ERROR"; break;
				case VALIDATION_CRITICAL: levelStr = "CRITICAL"; break;
			}

			printf("[%s] %s", levelStr, result.message.String());
			if (result.context.Length() > 0) {
				printf(" (Context: %s)", result.context.String());
			}
			printf("\n");
		}
		printf("\n");
	}

	// Summary
	printf("=====================================\n");
	printf("Summary\n");
	printf("=====================================\n");
	printf("Total tracks: %d\n", project.CountTracks());
	printf("Tracks with timeline data: ");

	int32 tracksWithTimeline = 0;
	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track && (track->StartPosition() > 0 || track->EndPosition() > 0)) {
			tracksWithTimeline++;
		}
	}

	printf("%d\n", tracksWithTimeline);
	printf("Tracks with 3D positions: ");

	int32 tracksWith3D = 0;
	for (int32 i = 0; i < project.CountTracks(); i++) {
		Track3DMix* track = project.TrackAt(i);
		if (track) {
			Coordinate3D pos = track->Position();
			if (pos.x != 0.0f || pos.y != 0.0f || pos.z != 0.0f) {
				tracksWith3D++;
			}
		}
	}

	printf("%d\n", tracksWith3D);
	printf("\n");

	printf("✅ Test completed successfully!\n\n");

	return 0;
}
