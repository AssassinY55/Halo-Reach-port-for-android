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

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port - XEX Finder Active";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Searching for executable inside ISO at: %s", isoPath);

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
        bool foundExecutable = false;

        while (offset + 14 < rootDirSize) {
            uint32_t sector = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 8]);
            uint32_t size = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 12]);
            uint8_t nameLen = dirBlock[offset + 17];

            if (nameLen == 0 || nameLen > 64) break;

            std::string fileName(&dirBlock[offset + 18], nameLen);
            
            // Convert filename to lowercase for robust matching
            std::string lowerFileName = fileName;
            std::transform(lowerFileName.begin(), lowerFileName.end(), lowerFileName.begin(), ::tolower);

            if (lowerFileName.find("default.xex") != std::string::npos || lowerFileName.find("default.xbe") != std::string::npos) {
                LOGI("--> TARGET FOUND! File: %s | Sector: %u | Size: %u bytes", fileName.c_str(), sector, size);
                foundExecutable = true;
            }

            offset += 18 + ((nameLen + 1) & ~1);
        }

        if (!foundExecutable) {
            LOGI("Scanned root directory table, looking deeper into subdirectories next.");
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
