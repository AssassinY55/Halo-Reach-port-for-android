#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Halo Reach Android Port Initialized";
    return env->NewStringUTF(hello.c_str());
}
