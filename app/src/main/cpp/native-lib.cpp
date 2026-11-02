#include <jni.h>
#include <string>
#include <android/log.h>
#include <fstream>

#define LOG_TAG "HaloReachNative"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port Initialized";
    return env->NewStringUTF(hello.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_xbox360_androidport_MainActivity_nativeInitIso(
        JNIEnv* env,
        jobject /* this */,
        jstring isoPath_) {
    
    // Convert Java string to C-style string
    const char* isoPath = env->GetStringUTFChars(isoPath_, 0);
    LOGI("Received ISO path from Java: %s", isoPath);

    // Verify if C++ can open the file
    std::ifstream file(isoPath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open ISO file at path!");
        env->ReleaseStringUTFChars(isoPath_, isoPath);
        return JNI_FALSE;
    }

    // Read the first few bytes to check file validity (e.g., header check)
    char header[16];
    file.read(header, sizeof(header));
    LOGI("Successfully opened ISO! File size check passed.");

    file.close();
    env->ReleaseStringUTFChars(isoPath_, isoPath);
    return JNI_TRUE;
}

}
