#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <3dmix file>\n", argv[0]);
        return 1;
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        printf("Cannot open file\n");
        return 1;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Read all
    uint8_t* data = (uint8_t*)malloc(size);
    fread(data, 1, size, f);
    fclose(f);

    printf("Searching for patterns with 3 consecutive floats (potential X,Y,Z coordinates):\n\n");

    int count = 0;
    for (long i = 0; i < size - 11 && count < 30; i++) {
        // Read 3 consecutive floats
        uint32_t raw1 = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24);
        uint32_t raw2 = data[i+4] | (data[i+5] << 8) | (data[i+6] << 16) | (data[i+7] << 24);
        uint32_t raw3 = data[i+8] | (data[i+9] << 8) | (data[i+10] << 16) | (data[i+11] << 24);

        float val1 = *((float*)&raw1);
        float val2 = *((float*)&raw2);
        float val3 = *((float*)&raw3);

        // Check if all 3 are valid coordinates
        if (!isnan(val1) && !isinf(val1) && val1 >= -13.0f && val1 <= 13.0f &&
            !isnan(val2) && !isinf(val2) && val2 >= -13.0f && val2 <= 13.0f &&
            !isnan(val3) && !isinf(val3) && val3 >= -13.0f && val3 <= 13.0f) {

            // Check if at least one is not exactly 0 (to avoid false positives)
            if (val1 != 0.0f || val2 != 0.0f || val3 != 0.0f) {
                printf("0x%04lx: X=%.6f Y=%.6f Z=%.6f\n", i, val1, val2, val3);
                count++;
                i += 11; // Skip this triplet
            }
        }
    }

    free(data);
    return 0;
}
