#include "droid_utils_Log.h"

char* jstringToChar(JNIEnv* env, jstring jstr);

JNIEXPORT jint JNICALL Java_droid_utils_Log_println_1native(
        JNIEnv* env, jclass clz, jint bufId
        , jint priority, jstring tag, jstring msg) {
    char* chTag = jstringToChar(env, tag);
    char* chMsg = jstringToChar(env, msg);

    std::cout<<"bufId: "<<bufId
            <<", priority: "<<priority
            <<", tag: "<<chTag
            <<", msg: "<<chMsg
            <<std::endl;

    free(chTag);
    free(chMsg);

    return 0;
}

char* jstringToChar(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
    env->DeleteLocalRef( strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0)
    {
        rtn = (char*)malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}