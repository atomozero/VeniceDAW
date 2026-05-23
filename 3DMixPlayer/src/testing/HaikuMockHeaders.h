/*
 * HaikuMockHeaders.h - Mock BeAPI headers for DEVELOPMENT ONLY
 * 
 * ATTENZIONE: Questi sono headers FINTI per sviluppo su sistemi non-Haiku.
 * Il vero sistema di testing funziona SOLO su Haiku nativo con BeAPI reali.
 * 
 * Questo file permette di compilare il codice per verifica sintassi,
 * ma tutti i test reali devono essere eseguiti su sistema Haiku nativo.
 */

#ifndef HAIKU_MOCK_HEADERS_H
#define HAIKU_MOCK_HEADERS_H

#warning "ATTENZIONE: Stai usando headers MOCK. Il testing reale funziona SOLO su Haiku nativo!"

#include <iostream>
#include <string>
#include <cstdint>
#include <thread>
#include <functional>
#include <vector>
#include <chrono>
#include <unistd.h>

// Mock BeAPI types - SOLO per compilazione sviluppo
typedef int32_t thread_id;
typedef int32_t status_t;
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint8_t uint8;
typedef int64_t bigtime_t;
typedef int32_t sem_id;
typedef long off_t;

#define B_OK 0
#define B_ERROR -1
#define B_REAL_TIME_PRIORITY 10
#define B_NORMAL_PRIORITY 7
#define B_LOW_PRIORITY 5
#define B_READ_ONLY 1
#define B_ENTRY_NOT_FOUND -2147459069
#define B_IO_ERROR -2147459074
#define B_GENERAL_ERROR_BASE -2147483648

// Mock media types and constants
#define B_MEDIA_RAW_AUDIO 0x1
#define B_MEDIA_LITTLE_ENDIAN 1

enum media_raw_audio_format {
    B_AUDIO_UCHAR = 0x11,
    B_AUDIO_SHORT = 0x2,
    B_AUDIO_INT = 0x4,
    B_AUDIO_FLOAT = 0x24
};

struct media_format {
    uint32 type;
    union {
        struct {
            uint32 format;
            float frame_rate;
            uint32 channel_count;
            uint32 byte_order;
            size_t buffer_size;
        } raw_audio;
    } u;

    media_format() : type(B_MEDIA_RAW_AUDIO) {
        u.raw_audio.format = B_AUDIO_SHORT;
        u.raw_audio.frame_rate = 44100;
        u.raw_audio.channel_count = 2;
        u.raw_audio.byte_order = B_MEDIA_LITTLE_ENDIAN;
        u.raw_audio.buffer_size = 4096;
    }
};

// Mock system_time function
inline bigtime_t system_time() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Mock BeAPI classes - NON FUNZIONANTI
class BDataIO {
public:
    virtual ~BDataIO() {}
    virtual ssize_t Read(void* buffer, size_t size) = 0;
    virtual ssize_t Write(const void* buffer, size_t size) = 0;
};

class BFile : public BDataIO {
public:
    BFile() : fInitStatus(B_ERROR) {}
    BFile(const char* path, uint32_t openMode) : fInitStatus(B_OK) {}
    virtual ~BFile() {}

    status_t InitCheck() const { return fInitStatus; }
    status_t GetSize(off_t* size) { *size = 1024; return B_OK; }

    virtual ssize_t Read(void* buffer, size_t size) override { return size; }
    virtual ssize_t Write(const void* buffer, size_t size) override { return size; }

private:
    status_t fInitStatus;
};

class BMessage {
public:
    BMessage(uint32_t what) : fWhat(what) {}
    status_t AddInt32(const char* name, int32_t value) { return B_OK; }
    status_t AddInt64(const char* name, int64_t value) { return B_OK; }
    status_t Unflatten(BDataIO* stream) { return B_OK; }
private:
    uint32_t fWhat;
};

class BRect {
public:
    BRect() : left(0), top(0), right(0), bottom(0) {}
    BRect(float l, float t, float r, float b) : left(l), top(t), right(r), bottom(b) {}
    float Width() const { return right - left; }
    float Height() const { return bottom - top; }
    float left, top, right, bottom;
};

class BString {
public:
    BString() {}
    BString(const char* str) : fString(str ? str : "") {}
    const char* String() const { return fString.c_str(); }
    void SetTo(const char* str) { fString = str ? str : ""; }
    int32 Length() const { return fString.length(); }
    bool operator==(const char* str) const { return fString == str; }

    int32 FindFirst(const char* str) const {
        size_t pos = fString.find(str);
        return pos == std::string::npos ? -1 : (int32)pos;
    }

    int32 FindLast(const char* str) const {
        size_t pos = fString.rfind(str);
        return pos == std::string::npos ? -1 : (int32)pos;
    }

    void CopyInto(BString& dest, int32 from, int32 length) const {
        dest.fString = fString.substr(from, length);
    }

private:
    std::string fString;
};

class BList {
public:
    BList() {}
    ~BList() {}
    bool AddItem(void* item) { fItems.push_back(item); return true; }
    void* ItemAt(int32 index) const {
        if (index >= 0 && index < (int32)fItems.size())
            return fItems[index];
        return nullptr;
    }
    bool RemoveItem(int32 index) {
        if (index >= 0 && index < (int32)fItems.size()) {
            fItems.erase(fItems.begin() + index);
            return true;
        }
        return false;
    }
    int32 CountItems() const { return fItems.size(); }
    void MakeEmpty() { fItems.clear(); }
private:
    std::vector<void*> fItems;
};

class BPoint {
public:
    BPoint() : x(0), y(0) {}
    BPoint(float px, float py) : x(px), y(py) {}
    float x, y;
};

class BView {
public:
    BView(BRect frame, const char* name, uint32_t resizeMode, uint32_t flags) {}
    virtual ~BView() {}
    BRect Bounds() const { return BRect(); }
    BRect Frame() const { return BRect(); }
    virtual void AttachedToWindow() {}
    virtual void FrameResized(float w, float h) {}
    virtual void Draw(BRect updateRect) {}
    void Invalidate() {}
    BView* ChildAt(int32_t index) { return nullptr; }
};

#define B_FOLLOW_ALL_SIDES 0
#define B_WILL_DRAW 1
#define B_TITLED_WINDOW 0

class BHandler {
public:
    BHandler() {}
    virtual ~BHandler() {}
};

class BLooper : public BHandler {
public:
    BLooper() {}
    virtual ~BLooper() {}
    virtual void ReadyToRun() {}
    status_t PostMessage(BMessage* message) { delete message; return B_OK; }
    thread_id Thread() const { return 1; }
};

class BWindow : public BLooper {
public:
    BWindow(BRect frame, const char* title, uint32_t type, uint32_t flags) {}
    virtual ~BWindow() {}
    bool Lock() { return true; }
    void Unlock() {}
    void Show() {}
    void Quit() {}
    BRect Frame() const { return BRect(); }
    BRect Bounds() const { return BRect(); }
    void ResizeTo(float width, float height) {}
    void AddChild(BView* child) {}
};

class BApplication : public BLooper {
public:
    BApplication(const char* signature) {}
    virtual ~BApplication() {}
    virtual void ReadyToRun() {}
};

// Mock system functions
inline thread_id find_thread(const char* name) { return std::hash<std::thread::id>{}(std::this_thread::get_id()); }
inline status_t set_thread_priority(thread_id thread, int32_t priority) { return B_OK; }
inline status_t suggest_thread_priority(int32_t priority) { return B_OK; }
inline thread_id spawn_thread(int32_t (*func)(void*), const char* name, int32_t priority, void* data) { return 1; }
inline status_t resume_thread(thread_id thread) { return B_OK; }
inline status_t wait_for_thread(thread_id thread, status_t* result) { return B_OK; }
inline void snooze(uint64_t microseconds) { std::this_thread::sleep_for(std::chrono::microseconds(microseconds)); }

// Mock team/system info structs
struct team_info {
    int32_t team;
    int32_t image_count;
};

struct thread_info {
    thread_id thread;
    int32_t user_time;
    int32_t kernel_time;
    int32_t priority;
};

struct system_info {
    int32_t used_pages;
    int32_t max_pages;
};

#define B_CURRENT_TEAM 0

inline status_t get_team_info(int32_t team, team_info* info) { 
    if (info) {
        info->team = team;
        info->image_count = 10;
    }
    return B_OK; 
}

inline status_t get_thread_info(thread_id thread, thread_info* info) {
    if (info) {
        info->thread = thread;
        info->user_time = 1000;
        info->kernel_time = 500;
        info->priority = B_NORMAL_PRIORITY;
    }
    return B_OK;
}

inline status_t get_next_thread_info(int32_t team, int32_t* cookie, thread_info* info) { 
    if (*cookie >= 5) return B_ERROR; // Simulate 5 threads
    if (info) {
        info->thread = *cookie;
        info->user_time = 1000;
        info->kernel_time = 500;
        info->priority = B_NORMAL_PRIORITY;
    }
    (*cookie)++;
    return B_OK; 
}

inline status_t get_system_info(system_info* info) { 
    if (info) {
        info->used_pages = 1000;
        info->max_pages = 2000;
    }
    return B_OK; 
}

// Stampa warning ogni volta che viene incluso
struct HaikuMockWarning {
    HaikuMockWarning() {
        std::cerr << "\n⚠️  ATTENZIONE: Stai usando MOCK BeAPI headers!" << std::endl;
        std::cerr << "   Questo codice è solo per sviluppo/testing sintassi." << std::endl;
        std::cerr << "   Il vero testing VeniceDAW funziona SOLO su Haiku OS nativo!" << std::endl;
        std::cerr << "   Su Haiku reale, usa: make test-framework-quick" << std::endl << std::endl;
    }
};

static HaikuMockWarning __mock_warning_instance;

#endif // HAIKU_MOCK_HEADERS_H