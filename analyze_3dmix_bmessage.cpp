/*
 * analyze_3dmix_bmessage.cpp - Analyze BMessage structure in 3dmix files
 *
 * This tool reads a 3dmix file and dumps the complete BMessage structure
 * for each track to understand what data is actually stored.
 */

#include <Application.h>
#include <File.h>
#include <Message.h>
#include <String.h>
#include <ByteOrder.h>
#include <stdio.h>
#include <stdlib.h>

class BMessageAnalyzer {
public:
    void AnalyzeFile(const char* path) {
        BFile file(path, B_READ_ONLY);
        if (file.InitCheck() != B_OK) {
            printf("ERROR: Cannot open file: %s\n", path);
            return;
        }

        printf("=================================================\n");
        printf("3DMix BMessage Structure Analysis\n");
        printf("=================================================\n");
        printf("File: %s\n\n", path);

        // Read magic number
        uint32 magic;
        file.Read(&magic, 4);
        magic = B_BENDIAN_TO_HOST_INT32(magic);
        printf("Magic: 0x%08X ('%c%c%c%c')\n", magic,
               (magic >> 24) & 0xFF, (magic >> 16) & 0xFF,
               (magic >> 8) & 0xFF, magic & 0xFF);

        // Read track count
        uint32 count;
        file.Read(&count, 4);
        count = B_BENDIAN_TO_HOST_INT32(count);
        printf("Track count: %d\n\n", count);

        // Analyze each track
        for (int i = 0; i < (int)count; i++) {
            printf("--- TRACK #%d ---\n", i + 1);
            AnalyzeTrack(&file, i + 1);
            printf("\n");
        }
    }

private:
    void AnalyzeTrack(BFile* file, int trackNum) {
        // Read audio file path (2048 bytes)
        char path[2048];
        file->Read(path, 2048);
        path[2047] = '\0';
        printf("Audio file: %s\n", path);

        // Read X, Y coordinates
        uint32 xRaw, yRaw;
        file->Read(&xRaw, 4);
        xRaw = B_BENDIAN_TO_HOST_INT32(xRaw);
        float x = *((float*)&xRaw);

        file->Read(&yRaw, 4);
        yRaw = B_BENDIAN_TO_HOST_INT32(yRaw);
        float y = *((float*)&yRaw);

        printf("Position: X=%.2f, Y=%.2f\n", x, y);

        // Read BMessage size
        uint32 messageSize;
        ssize_t bytesRead = file->Read(&messageSize, 4);
        if (bytesRead != 4) {
            printf("No BMessage data (end of track)\n");
            return;
        }

        messageSize = B_BENDIAN_TO_HOST_INT32(messageSize);
        printf("BMessage size: %d bytes\n", messageSize);

        if (messageSize == 0 || messageSize > 100000) {
            printf("Invalid or no BMessage data\n");
            return;
        }

        // Read BMessage data
        uint8* messageData = new uint8[messageSize];
        bytesRead = file->Read(messageData, messageSize);
        if (bytesRead != (ssize_t)messageSize) {
            printf("ERROR: Could not read complete BMessage (got %ld of %d bytes)\n",
                   bytesRead, messageSize);
            delete[] messageData;
            return;
        }

        // Try to unflatten BMessage
        BMessage message;
        status_t status = message.Unflatten((const char*)messageData);

        if (status == B_OK) {
            printf("✓ BMessage successfully unflattened!\n");
            printf("Message what: 0x%08X\n", message.what);
            DumpMessageContents(&message);
        } else {
            printf("✗ Failed to unflatten BMessage (status: 0x%08X)\n", status);
            printf("Hex dump of first 64 bytes:\n");
            HexDump(messageData, messageSize < 64 ? messageSize : 64);
        }

        delete[] messageData;
    }

    void DumpMessageContents(BMessage* message) {
        printf("\nBMessage Contents:\n");
        printf("------------------\n");

        char* name;
        type_code type;
        int32 count;

        for (int32 i = 0; message->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; i++) {
            printf("Field #%d: \"%s\"\n", i, name);
            printf("  Type: 0x%08X ('%c%c%c%c')\n", type,
                   (type >> 24) & 0xFF, (type >> 16) & 0xFF,
                   (type >> 8) & 0xFF, type & 0xFF);
            printf("  Count: %d\n", count);

            // Try to extract values based on type
            DumpFieldValue(message, name, type);
            printf("\n");
        }
    }

    void DumpFieldValue(BMessage* message, const char* name, type_code type) {
        switch (type) {
            case B_INT32_TYPE: {
                int32 value;
                if (message->FindInt32(name, &value) == B_OK) {
                    printf("  Value: %d (0x%08X)\n", value, value);
                }
                break;
            }
            case B_FLOAT_TYPE: {
                float value;
                if (message->FindFloat(name, &value) == B_OK) {
                    printf("  Value: %.6f\n", value);
                }
                break;
            }
            case B_BOOL_TYPE: {
                bool value;
                if (message->FindBool(name, &value) == B_OK) {
                    printf("  Value: %s\n", value ? "true" : "false");
                }
                break;
            }
            case B_STRING_TYPE: {
                const char* value;
                if (message->FindString(name, &value) == B_OK) {
                    printf("  Value: \"%s\"\n", value);
                }
                break;
            }
            case B_INT64_TYPE: {
                int64 value;
                if (message->FindInt64(name, &value) == B_OK) {
                    printf("  Value: %lld (0x%016llX)\n", value, value);
                }
                break;
            }
            case B_DOUBLE_TYPE: {
                double value;
                if (message->FindDouble(name, &value) == B_OK) {
                    printf("  Value: %.10f\n", value);
                }
                break;
            }
            default:
                printf("  (Type not handled in analyzer)\n");
                break;
        }
    }

    void HexDump(const uint8* data, size_t length) {
        for (size_t i = 0; i < length; i += 16) {
            printf("  %04lX: ", i);

            // Hex
            for (size_t j = 0; j < 16 && (i + j) < length; j++) {
                printf("%02X ", data[i + j]);
            }

            // Padding
            for (size_t j = length - i; j < 16; j++) {
                printf("   ");
            }

            // ASCII
            printf(" | ");
            for (size_t j = 0; j < 16 && (i + j) < length; j++) {
                uint8 c = data[i + j];
                printf("%c", (c >= 32 && c < 127) ? c : '.');
            }

            printf("\n");
        }
    }
};

class AnalyzerApp : public BApplication {
public:
    AnalyzerApp() : BApplication("application/x-vnd.VeniceDAW-3DMixAnalyzer") {}

    void ReadyToRun() override {
        // This is a command-line tool, quit immediately after analysis
        PostMessage(B_QUIT_REQUESTED);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <3dmix-file>\n", argv[0]);
        printf("\nAnalyzes the BMessage structure in BeOS 3dmix files.\n");
        printf("Shows all fields, types, and values stored in each track.\n");
        return 1;
    }

    // Need BApplication for BMessage to work properly
    AnalyzerApp app;

    BMessageAnalyzer analyzer;
    analyzer.AnalyzeFile(argv[1]);

    return 0;
}
