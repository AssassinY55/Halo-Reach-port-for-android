#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <fstream>

#define LOG_TAG "HaloReachNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static ANativeWindow* globalWindow = nullptr;

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port - Render Loop Ready";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Analyzing ISO/XISO path: %s", isoPath);

    std::ifstream file(isoPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open ISO file descriptor.");
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    file.seekg(0, std::ios::end);
    long long fileSize = file.tellg();
    file.close();
    env->ReleaseStringUTFChars(isoPath_, isoPath);

    if (fileSize > 1024LL * 1024LL * 1024LL) {
        LOGI("XISO validation passed! Size: %lld bytes", fileSize);
        return JNI_TRUE;
    } else {
        LOGE("File is too small.");
        return JNI_FALSE;
    }
}

JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeSetSurface(
        JNIEnv* env,
        jobject /* this */,
        jobject surface) {
    if (surface != nullptr) {
        globalWindow = ANativeWindow_fromSurface(env, surface);
        if (globalWindow) {
            LOGI("ANativeWindow acquired successfully!");
        } else {
            LOGE("Failed to acquire ANativeWindow.");
        }
    } else {
        if (globalWindow) {
            ANativeWindow_release(globalWindow);
            globalWindow = nullptr;
            LOGI("ANativeWindow released.");
        }
    }
}

JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeResizeSurface(
        JNIEnv* env,
        jobject /* this */,
        jint width,
        jint height) {
    LOGI("Surface dimensions changed: %dx%d", width, height);
    if (globalWindow) {
        ANativeWindow_setBuffersGeometry(globalWindow, width, height, WINDOW_FORMAT_RGBA_8888);
    }
}

// Function to test drawing pixels directly to the mobile screen canvas via C++
JNIEXPORT void JNICALL
Java_com_xbox360_androidport_MainActivity_nativeRenderFrame(
        JNIEnv* env,
        jobject /* this */) {
    if (!globalWindow) {
        LOGE("Cannot render frame: ANativeWindow is null.");
        return;
    }

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(globalWindow, &buffer, nullptr) < 0) {
        LOGE("Failed to lock ANativeWindow buffer.");
        return;
    }

    // Paint pixels directly to the screen (e.g., a dark theme rendering test block)
    uint32_t* line = (uint32_t*)buffer.bits;
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            // Fill with a stylish dark blue / slate color pattern for testing
            line[x] = (0xFF << 24) | (0x10 << 16) | (0x20 << 8) | 0x40; 
        }
        line = (uint32_t*)((char*)line + buffer.stride * 4);
    }

    ANativeWindow_unlockAndPost(globalWindow);
    LOGI("Test frame successfully rendered and posted to screen.");
}

}
