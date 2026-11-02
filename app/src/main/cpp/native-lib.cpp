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
    std::string hello = "Halo Reach Android Port - Canvas Active";
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
            LOGI("ANativeWindow acquired successfully from Java Surface!");
        } else {
            LOGE("Failed to acquire ANativeWindow from Surface.");
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

}
