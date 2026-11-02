#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <fstream>
#include <vector>

#define LOG_TAG "HaloReachNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow* globalWindow = nullptr;

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port - File Scanner Active";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Scanning XDVDFS directory tree at: %s", isoPath);

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

    LOGI("Root Directory Sector: %u, Size: %u bytes", rootDirSector, rootDirSize);

    if (rootDirSector > 0 && rootDirSize > 0) {
        file.seekg(rootDirSector * 2048, std::ios::beg);
        
        // Read the root directory table block
        std::vector<char> dirBlock(rootDirSize);
        file.read(dirBlock.data(), rootDirSize);

        // Parse root table records (XDVDFS entry format: left(4), right(4), sector(4), size(4), attr(1), namelen(1), name)
        size_t offset = 0;
        int filesLogged = 0;
        while (offset + 14 < rootDirSize && filesLogged < 10) {
            uint32_t sector = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 8]);
            uint32_t size = *reinterpret_cast<uint32_t*>(&dirBlock[offset + 12]);
            uint8_t nameLen = dirBlock[offset + 17];

            if (nameLen == 0 || nameLen > 64) break;

            std::string fileName(&dirBlock[offset + 18], nameLen);
            LOGI("Found ISO Asset -> Name: %s | Sector: %u | Size: %u bytes", fileName.c_str(), sector, size);

            filesLogged++;
            offset += 18 + ((nameLen + 1) & ~1); // Align to 2-byte boundaries
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
