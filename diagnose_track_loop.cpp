/*
 * Diagnostic tool to check loop and timeline parameters in BeOS Track Object files
 */

#include <stdio.h>
#include <File.h>
#include <support/DataIO.h>
#include <support/ByteOrder.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <track-object-file>\n", argv[0]);
        return 1;
    }

    const char* filePath = argv[1];
    BFile file(filePath, B_READ_ONLY);

    if (file.InitCheck() != B_OK) {
        printf("❌ Failed to open file: %s\n", filePath);
        return 1;
    }

    // Read Track Object binary format
    // Real format observed from hex dump:
    // - !TRK (4 bytes)
    // - trackname_size (4 bytes)
    // - trackname (trackname_size bytes)
    // - unknown (4 bytes) - possibly version/flags
    // - SIMP (4 bytes) - type
    // - RAW_ (4 bytes) - subtype
    // - audiofile_size (4 bytes)
    // - audiofilename (audiofile_size bytes)
    // - v_from, v_to, st_skip, loop_point (4 bytes float each)

    // Skip !TRK magic (4 bytes)
    file.Seek(4, SEEK_SET);

    // Read track name size
    uint32 trackNameSize;
    file.Read(&trackNameSize, sizeof(trackNameSize));
    trackNameSize = B_BENDIAN_TO_HOST_INT32(trackNameSize);

    // Skip track name
    if (trackNameSize > 0 && trackNameSize < 2048) {
        file.Seek(trackNameSize, SEEK_CUR);
    }

    // Skip unknown 4 bytes
    file.Seek(4, SEEK_CUR);

    // Read and verify SIMP type
    uint32 type;
    file.Read(&type, sizeof(type));
    type = B_BENDIAN_TO_HOST_INT32(type);
    if (type != 0x53494D50) { // 'SIMP'
        printf("❌ Unexpected type: 0x%08X (expected 'SIMP')\n", type);
        return 1;
    }

    // Read subtype
    uint32 subtype;
    file.Read(&subtype, sizeof(subtype));
    subtype = B_BENDIAN_TO_HOST_INT32(subtype);

    // Read audio filename size
    uint32 filenameSize;
    file.Read(&filenameSize, sizeof(filenameSize));
    filenameSize = B_BENDIAN_TO_HOST_INT32(filenameSize);

    // Skip audio filename
    if (filenameSize > 0 && filenameSize < 2048) {
        file.Seek(filenameSize, SEEK_CUR);
    }

    // Read v_from (timeline start)
    uint32 vFromRaw;
    file.Read(&vFromRaw, sizeof(vFromRaw));
    vFromRaw = B_BENDIAN_TO_HOST_INT32(vFromRaw);
    float vFrom = *((float*)&vFromRaw);

    // Read v_to (timeline end)
    uint32 vToRaw;
    file.Read(&vToRaw, sizeof(vToRaw));
    vToRaw = B_BENDIAN_TO_HOST_INT32(vToRaw);
    float vTo = *((float*)&vToRaw);

    // Read st_skip (audio trim/start offset)
    uint32 stSkipRaw;
    file.Read(&stSkipRaw, sizeof(stSkipRaw));
    stSkipRaw = B_BENDIAN_TO_HOST_INT32(stSkipRaw);
    float stSkip = *((float*)&stSkipRaw);

    // Read loop_point
    uint32 loopPointRaw;
    file.Read(&loopPointRaw, sizeof(loopPointRaw));
    loopPointRaw = B_BENDIAN_TO_HOST_INT32(loopPointRaw);
    float loopPoint = *((float*)&loopPointRaw);

    printf("\n");
    printf("=================================================\n");
    printf("BeOS Track Object Loop/Timeline Diagnostic\n");
    printf("=================================================\n");
    printf("File: %s\n\n", filePath);

    printf("Timeline Position (in project):\n");
    printf("  v_from:     %.3f seconds (track starts at this time in project)\n", vFrom);
    printf("  v_to:       %.3f seconds (track ends at this time in project)\n", vTo);
    printf("  Duration:   %.3f seconds\n\n", vTo - vFrom);

    printf("Audio File Playback:\n");
    printf("  st_skip:    %.3f seconds (TRIM: skip this much from start of audio file)\n", stSkip);
    printf("  loop_point: %.3f seconds (LOOP: when playback reaches this point, loop back to st_skip)\n", loopPoint);
    printf("  Loop range: %.3f - %.3f seconds (%.3f seconds)\n\n", stSkip, loopPoint, loopPoint - stSkip);

    printf("At 44100 Hz:\n");
    printf("  Timeline:   %d - %d samples\n", (int)(vFrom * 44100), (int)(vTo * 44100));
    printf("  Audio trim: %d samples\n", (int)(stSkip * 44100));
    printf("  Loop point: %d samples\n\n", (int)(loopPoint * 44100));

    printf("Playback Behavior:\n");
    if (stSkip > 0.001 || loopPoint > 0.001) {
        printf("  ⚠️  This track uses TRIM and/or LOOP features\n");
        if (stSkip > 0.001) {
            printf("  - Audio starts at %.3fs (not from beginning)\n", stSkip);
        }
        if (loopPoint > stSkip + 0.001) {
            printf("  - Audio loops from %.3fs back to %.3fs\n", loopPoint, stSkip);
        }
        printf("\n  ❌ VeniceDAW currently IGNORES these parameters!\n");
        printf("  ❌ Audio will play from 0.0s instead of %.3fs\n", stSkip);
        printf("  ❌ No looping will occur\n");
    } else {
        printf("  ✓ No trim or loop (plays entire file once)\n");
    }
    printf("\n");

    return 0;
}
