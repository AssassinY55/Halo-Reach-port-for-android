#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <fstream>
#include <vector>
#include <algorithm>

#define LOG_TAG "HaloReachNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow* globalWindow = nullptr;

struct XexHeader {
    uint32_t magic;
    uint32_t moduleFlags;
    uint32_t peImageSize;
    uint32_t headerSize;
    uint32_t elfPheaderOffset;
    uint32_t sectionCount;
};

// PowerPC CPU Register Context Stub
struct PowerPCContext {
    uint32_t gpr[32]; // General Purpose Registers
    uint32_t pc;      // Program Counter
    uint32_t lr;      // Link Register
    uint32_t cr;      // Condition Register
    uint32_t ctr;     // Count Register
    bool isRunning;
};

// Global CPU Instance
static PowerPCContext g_ppcCpu;

void initializePPCContext(uint32_t entryPoint) {
    memset(&g_ppcCpu, 0, sizeof(PowerPCContext));
    g_ppcCpu.pc = entryPoint;
    g_ppcCpu.isRunning = true;
    LOGI("PowerPC CPU Context Initialized at Entry Point: 0x%08X", entryPoint);
}

void dispatchNextInstruction(const std::vector<char>& memorySpace, uint32_t baseAddress) {
    if (!g_ppcCpu.isRunning) return;

    // Calculate relative offset in memory block
    uint32_t relativeOffset = g_ppcCpu.pc - baseAddress;
    if (relativeOffset + 4 > memorySpace.size()) {
        LOGE("CPU Exception: PC out of bounds (0x%08X)", g_ppcCpu.pc);
        g_ppcCpu.isRunning = false;
        return;
    }

    // Fetch 32-bit PowerPC instruction (Big Endian)
    uint32_t rawOpcode = *reinterpret_cast<const uint32_t*>(&memorySpace[relativeOffset]);
    
    // Simple opcode dispatcher stub
    uint32_t primaryOpcode = rawOpcode >> 26;
    
    // LOGI("Fetching PPC Instruction [PC: 0x%08X] Opcode: 0x%02X", g_ppcCpu.pc, primaryOpcode);

    // Advance Program Counter (4 bytes per instruction in PowerPC)
    g_ppcCpu.pc += 4;
}

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port - PowerPC Dispatcher Active";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("=== BOOTING XBOX 360 EMULATION ENVIRONMENT ===");

    std::ifstream file(isoPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Critical Error: Failed to open ISO file descriptor.");
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    // Read Root Directory Sector and Size from Volume Descriptor at Sector 32 (0x10000)
    file.seekg(0x10014, std::ios::beg);
    uint32_t rootDirSector = 0;
    file.read(reinterpret_cast<char*>(&rootDirSector), sizeof(rootDirSector));

    uint32_t rootDirSize = 0;
    file.read(reinterpret_cast<char*>(&rootDirSize), sizeof(rootDirSize));

    // Traverse Root Directory Table for default.xex
    file.seekg(rootDirSector * 2048, std::ios::beg);
    std::vector<char> dirBlock(rootDirSize);
    file.read(dirBlock.data(), rootDirSize);

    size_t offset = 0;
    uint32_t xexSector = 0;
    uint32_t xexSize = 0;
    bool executableFound = false;

    while (offset + 14 < rootDirSize) {
        uint32_t sector = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 8]);
        uint32_t size = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 12]);
        uint8_t nameLen = dirBlock[offset + 17];

        if (nameLen == 0 || nameLen > 64) break;

        std::string fileName(&dirBlock[offset + 18], nameLen);
        std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);

        if (fileName.find("default.xex") != std::string::npos || fileName.find("default.xbe") != std::string::npos) {
            xexSector = sector;
            xexSize = size;
            executableFound = true;
            break;
        }
        offset += 18 + ((nameLen + 1) & ~1);
    }

    if (!executableFound) {
        LOGE("Critical Error: default.xex not found.");
        file.close();
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    // Load Executable Binary into Simulation Memory Buffer
    file.seekg((long long)xexSector * 2048, std::ios::beg);
    std::vector<char> xexBinary(xexSize);
    file.read(xexBinary.data(), xexSize);

    // Initialize CPU Context with simulated base address 0x82000000 (typical Xbox 360 image base)
    initializePPCContext(0x82000000);

    LOGI("=== EMULATION ENVIRONMENT READY ===");

    file.close();
    env->ReleaseStringUTFChars(isoPath_, isoPath);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeSetSurface(
        JNIEnv* env,
        jobject /* this */,
        jobject surface) {
    if (surface != nullptr) {
        globalWindow = ANativeWindow_fromSurface(env, surface);
    } else {
        if (globalWindow) {
            ANativeWindow_release(globalWindow);
            globalWindow = nullptr;
        }
    }
}

JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeResizeSurface(
        JNIEnv* env,
        jobject /* this */,
        jint width,
        jint height) {
    if (globalWindow) {
        ANativeWindow_setBuffersGeometry(globalWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    }
}

JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeRenderFrame(
        JNIEnv* env,
        jobject /* this */) {
    if (!globalWindow) return;

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(globalWindow, &buffer, nullptr) < 0) return;

    // Render frame loop
    uint32_t* line = (uint32_t*)buffer.bits;
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            line[x] = (0xFF << 24) | (0x0F << 16) | (0x17 << 8) | 0x2A; 
        }
        line = (uint32_t*)((char*)line + buffer.stride * 4);
    }

    ANativeWindow_unlockAndPost(globalWindow);
}

}
