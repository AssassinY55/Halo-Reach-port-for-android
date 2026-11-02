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

struct XexSectionHeader {
    uint32_t info;
    uint32_t sectionSize;
    uint32_t vAddress;
};

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port - Memory Segment Mapper Active";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Mapping virtual memory segments from: %s", isoPath);

    std::ifstream file(isoPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open ISO file descriptor.");
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    // Read Root Directory Sector and Size from Volume Descriptor at Sector 32 (0x10000)
    file.seekg(0x10014, std::ios::beg);
    uint32_t rootDirSector = 0;
    file.read(reinterpret_cast<char*>(&rootDirSector), sizeof(rootDirSector));

    uint32_t rootDirSize = 0;
    file.read(reinterpret_cast<char*>(&rootDirSize), sizeof(rootDirSize));

    if (rootDirSector > 0 && rootDirSize > 0) {
        file.seekg(rootDirSector * 2048, std::ios::beg);
        
        std::vector<char> dirBlock(rootDirSize);
        file.read(dirBlock.data(), rootDirSize);

        size_t offset = 0;
        bool executableFound = false;

        while (offset + 14 < rootDirSize) {
            uint32_t sector = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 8]);
            uint32_t size = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 12]);
            uint8_t nameLen = dirBlock[offset + 17];

            if (nameLen == 0 || nameLen > 64) break;

            std::string fileName(&dirBlock[offset + 18], nameLen);
            std::string lowerFileName = fileName;
            std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

            if (lowerFileName.find("default.xex") != std::string::npos || lowerFileName.find("default.xbe") != std::string::npos) {
                LOGI("Found Executable -> Sector: %u | Size: %u bytes", sector, size);
                
                // Jump to executable and read XEX Header structure
                long long xexFileOffset = (long long)sector * 2048;
                file.seekg(xexFileOffset, std::ios::beg);
                
                XexHeader xex;
                file.read(reinterpret_cast<char*>(&xex), sizeof(XexHeader));

                LOGI("XEX Magic: 0x%08X | Header Size: %u | Optional Headers Count: %u", 
                     xex.magic, xex.headerSize, xex.sectionCount);

                // Iterate through the header space to locate loader info/sections
                // XEX headers are variable-length structures following the main header block
                file.seekg(xexFileOffset + sizeof(XexHeader), std::ios::beg);
                
                std::vector<char> headerData(xex.headerSize - sizeof(XexHeader));
                file.read(headerData.data(), headerData.size());

                LOGI("Successfully loaded XEX header payload into memory for parsing.");
                executableFound = true;
                break;
            }

            offset += 18 + ((nameLen + 1) & ~1);
        }

        if (!executableFound) {
            LOGE("Executable default.xex not found.");
        }
    }

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
