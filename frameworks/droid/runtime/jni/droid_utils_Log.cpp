#include "droid_utils_Log.h"

#include "log/log.h"

JNIEXPORT jint JNICALL Java_droid_utils_Log_println_1native(
        JNIEnv* env, jclass clz, jint bufId
        , jint priority, jstring tag, jstring msg) {
    const char* cTag, *cMsg;
    cTag = env->GetStringUTFChars(tag, nullptr);
    cMsg = env->GetStringUTFChars(msg, nullptr);
    /*
    std::cout<<"droid_utils_log.cpp bufId: "<<bufId
            <<", priority: "<<priority
            <<", tag: "<<cTag
            <<", msg: "<<cMsg
            <<std::endl;
    */
    __droid_log_buf_write(bufId, (LogPriority)priority, cTag, cMsg);

    env->ReleaseStringUTFChars(tag, cTag);
    env->ReleaseStringUTFChars(msg, cMsg);
    return 0;
}