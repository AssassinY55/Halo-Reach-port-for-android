#include <jni.h>
#include <string>

extern "C" JNIEXPORT jstring JNICALL
Java_com_xbox360_androidport_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from Halo Reach Android Port C++!";
    return env->NewStringUTF(hello.c_str());
}
