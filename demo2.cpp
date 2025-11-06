/*
 * demo2 - Simple terminal audio player for BeOS Track Object files
 * Tests stereo int16 playback at 44100 Hz (BeOS R6 format)
 */

#include <SoundPlayer.h>
#include <MediaDefs.h>
#include <Application.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Audio data storage
struct AudioData {
	int16_t* samples;      // Stereo int16 samples (L,R,L,R,...)
	size_t totalFrames;    // Total stereo frames
	size_t currentFrame;   // Current playback position
	bool isPlaying;
};

// Audio callback function
void PlayBufferFunc(void* cookie, void* buffer, size_t size, const media_raw_audio_format& format) {
	AudioData* data = (AudioData*)cookie;

	if (!data->isPlaying) {
		// Fill with silence
		memset(buffer, 0, size);
		return;
	}

	int16_t* outBuffer = (int16_t*)buffer;
	size_t framesToPlay = size / (sizeof(int16_t) * 2);  // Stereo frames

	for (size_t i = 0; i < framesToPlay; i++) {
		if (data->currentFrame >= data->totalFrames) {
			// End of audio - loop back to start
			data->currentFrame = 0;
		}

		// Copy stereo frame (L,R)
		size_t srcIdx = data->currentFrame * 2;
		outBuffer[i * 2 + 0] = data->samples[srcIdx + 0];  // Left
		outBuffer[i * 2 + 1] = data->samples[srcIdx + 1];  // Right

		data->currentFrame++;
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: demo2 <audio_file>\n");
		printf("Plays BeOS Track Object files (96-byte header + stereo int16 @ 44100 Hz)\n");
		return 1;
	}

	const char* filename = argv[1];

	// Open file
	FILE* f = fopen(filename, "rb");
	if (!f) {
		printf("Error: Cannot open file '%s'\n", filename);
		return 1;
	}

	// Get file size
	fseek(f, 0, SEEK_END);
	long fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	printf("File: %s\n", filename);
	printf("File size: %ld bytes\n", fileSize);

	// Read header (96 bytes for BeOS Track Object format)
	const size_t headerSize = 96;
	uint8_t header[headerSize];

	if (fread(header, 1, headerSize, f) != headerSize) {
		printf("Error: Cannot read header\n");
		fclose(f);
		return 1;
	}

	// Parse sample rate from header (offset 0x28 = 40, little-endian float)
	float sampleRate;
	memcpy(&sampleRate, &header[40], sizeof(float));
	printf("Sample rate from header: %.0f Hz\n", sampleRate);

	// Calculate audio data size (after header)
	size_t audioDataSize = fileSize - headerSize;
	size_t totalFrames = audioDataSize / (sizeof(int16_t) * 2);  // Stereo frames

	printf("Audio data: %zu bytes\n", audioDataSize);
	printf("Total frames: %zu (stereo)\n", totalFrames);
	printf("Duration: %.2f seconds\n", totalFrames / sampleRate);

	// Allocate and read audio samples
	int16_t* samples = (int16_t*)malloc(audioDataSize);
	if (!samples) {
		printf("Error: Cannot allocate memory\n");
		fclose(f);
		return 1;
	}

	if (fread(samples, 1, audioDataSize, f) != audioDataSize) {
		printf("Error: Cannot read audio data\n");
		free(samples);
		fclose(f);
		return 1;
	}

	fclose(f);

	// Print first few samples for debugging
	printf("\nFirst 10 stereo frames:\n");
	for (int i = 0; i < 10 && i < (int)totalFrames; i++) {
		printf("  Frame %d: L=%d, R=%d\n", i, samples[i*2], samples[i*2+1]);
	}

	// Create BApplication (required for BSoundPlayer)
	BApplication app("application/x-vnd.demo2-player");

	// Setup audio format (stereo int16 @ 44100 Hz)
	media_raw_audio_format format;
	format = media_raw_audio_format::wildcard;
	format.frame_rate = 44100.0f;
	format.channel_count = 2;
	format.format = media_raw_audio_format::B_AUDIO_SHORT;
	format.byte_order = B_MEDIA_LITTLE_ENDIAN;
	format.buffer_size = 4096;

	// Create audio data structure
	AudioData audioData;
	audioData.samples = samples;
	audioData.totalFrames = totalFrames;
	audioData.currentFrame = 0;
	audioData.isPlaying = false;

	// Create sound player
	BSoundPlayer* player = new BSoundPlayer(&format, "demo2", PlayBufferFunc, NULL, &audioData);

	if (player->InitCheck() != B_OK) {
		printf("Error: Cannot initialize sound player\n");
		delete player;
		free(samples);
		return 1;
	}

	printf("\n=== BeOS Track Object Player ===\n");
	printf("Commands:\n");
	printf("  p - play/pause\n");
	printf("  r - restart from beginning\n");
	printf("  q - quit\n");
	printf("\n");

	player->Start();
	player->SetHasData(true);

	// Command loop
	char cmd;
	while (true) {
		printf("> ");
		fflush(stdout);

		cmd = getchar();
		if (cmd == '\n') continue;  // Skip newline

		// Consume rest of line
		while (getchar() != '\n');

		switch (cmd) {
			case 'p':
				audioData.isPlaying = !audioData.isPlaying;
				printf("%s\n", audioData.isPlaying ? "Playing..." : "Paused");
				printf("Position: %.2f / %.2f seconds\n",
					   (float)audioData.currentFrame / format.frame_rate,
					   (float)audioData.totalFrames / format.frame_rate);
				break;

			case 'r':
				audioData.currentFrame = 0;
				printf("Restarted from beginning\n");
				break;

			case 'q':
				printf("Quitting...\n");
				goto cleanup;

			default:
				printf("Unknown command: %c\n", cmd);
				break;
		}
	}

cleanup:
	player->Stop();
	delete player;
	free(samples);

	return 0;
}
