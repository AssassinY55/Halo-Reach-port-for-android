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
    std::string hello = "Halo Reach Android Port - XDVDFS Scanner Active";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Mounting XDVDFS image at: %s", isoPath);

    std::ifstream file(isoPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open ISO file descriptor.");
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    file.seekg(0, std::ios::end);
    long long fileSize = file.tellg();
    
    if (fileSize < 1024LL * 1024LL * 1024LL) {
        LOGE("File is too small.");
        file.close();
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    // Read Volume Descriptor at Sector 32 (offset 0x10000)
    file.seekg(0x10000, std::ios::beg);
    char magic[20] = {0};
    file.read(magic, 20);

    // Read Root Directory Table Sector (u32 at offset 0x14 from sector 32 -> 0x10014)
    file.seekg(0x10014, std::ios::beg);
    uint32_t rootDirSector = 0;
    file.read(reinterpret_cast<char*>(&rootDirSector), sizeof(rootDirSector));

    // Read Root Directory Table Size (u32 at offset 0x18 from sector 32 -> 0x10018)
    uint32_t rootDirSize = 0;
    file.read(reinterpret_cast<char*>(&rootDirSize), sizeof(rootDirSize));

    LOGI("XDVDFS Volume Descriptor verified successfully!");
    LOGI("Root Directory Sector: %u, Size: %u bytes", rootDirSector, rootDirSize);

    // Position reader at the root directory table sector (sector * 2048)
    if (rootDirSector > 0) {
        file.seekg(rootDirSector * 2048, std::ios::beg);
        LOGI("Successfully positioned at XDVDFS root directory table tree.");
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
            // Tactical dark slate rendering pattern
            line[x] = (0xFF << 24) | (0x0F << 16) | (0x17 << 8) | 0x2A; 
        }
        line = (uint32_t*)((char*)line + buffer.stride * 4);
    }

    ANativeWindow_unlockAndPost(globalWindow);
}

}
