#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ByteOrder.h>

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

    // Read magic
    uint32_t magic;
    fread(&magic, 4, 1, f);
    magic = B_BENDIAN_TO_HOST_INT32(magic);
    printf("Magic: 0x%08X ('%c%c%c%c')\n", magic,
           (magic >> 24) & 0xFF, (magic >> 16) & 0xFF,
           (magic >> 8) & 0xFF, magic & 0xFF);

    // Read track count
    uint32_t count;
    fread(&count, 4, 1, f);
    count = B_BENDIAN_TO_HOST_INT32(count);
    printf("Track count: %d\n\n", count);

    // Read ALL tracks to see coordinate range
    float minX = 999999, maxX = -999999, minY = 999999, maxY = -999999;

    for (int i = 0; i < (int)count; i++) {
        // Read path (2048 bytes)
        char path[2048];
        fread(path, 2048, 1, f);
        path[2047] = '\0';
        printf("Track %d path: %s\n", i, path);

        // Read X
        uint32_t xRaw;
        fread(&xRaw, 4, 1, f);
        xRaw = B_BENDIAN_TO_HOST_INT32(xRaw);
        float x = *((float*)&xRaw);
        printf("  X = %.6f (raw: 0x%08X)\n", x, xRaw);

        // Read Y
        uint32_t yRaw;
        fread(&yRaw, 4, 1, f);
        yRaw = B_BENDIAN_TO_HOST_INT32(yRaw);
        float y = *((float*)&yRaw);
        printf("  Y = %.6f (raw: 0x%08X)\n\n", y, yRaw);

        if (x < minX) minX = x;
        if (x > maxX) maxX = x;
        if (y < minY) minY = y;
        if (y > maxY) maxY = y;
    }

    printf("\n=== Coordinate Range ===\n");
    printf("X: %.2f to %.2f (range: %.2f)\n", minX, maxX, maxX - minX);
    printf("Y: %.2f to %.2f (range: %.2f)\n", minY, maxY, maxY - minY);

    fclose(f);
    return 0;
}
