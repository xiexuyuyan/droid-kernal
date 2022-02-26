#include "droid_internal_os_ZygoteSocketClient.h"
#include "zygotesocket/ZygoteSocket.h"
#include "log/log.h"

#undef TAG
#define TAG "droid_internal_os_ZygoteSocketClient.cpp"

JNIEXPORT jboolean JNICALL Java_droid_internal_os_ZygoteSocketClient_nativeIsOpen(
        JNIEnv* env, jclass clz) {
    auto* zygoteSocketClient = droid::ZygoteSocketClient::getInstance();
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeIsOpen: ");
    if (zygoteSocketClient != nullptr) {
        return zygoteSocketClient->isOpen();
    }
    return false;
}
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteSocketClient_nativeOpen(
        JNIEnv* env, jclass clz) {
    auto* zygoteSocketClient = droid::ZygoteSocketClient::getInstance();
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeOpen: ");
    if (zygoteSocketClient != nullptr) {
        return zygoteSocketClient->openSocket();
    }
    return -1;
}

JNIEXPORT void JNICALL Java_droid_internal_os_ZygoteSocketClient_nativeClose(
        JNIEnv* env, jclass clz) {
    auto* zygoteSocketClient = droid::ZygoteSocketClient::getInstance();
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeClose: ");
    if (zygoteSocketClient != nullptr) {
        zygoteSocketClient->closeSocket();
    }
}

JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteSocketClient_nativeSendMessage(
        JNIEnv* env, jclass clz, jstring message) {
    int ret;

    auto* zygoteSocketClient = droid::ZygoteSocketClient::getInstance();
    const char* msg = env->GetStringUTFChars(message, nullptr);
    LOG_D(TAG, ("Java_droid_internal_os_ZygoteSocketClient_nativeSendMessage: "
                + std::string(msg)).c_str());
    if (zygoteSocketClient == nullptr) {
        LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeSendMessage: null pointer");
        return -1;
    }
    if (!zygoteSocketClient->isOpen()) {
        LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeSendMessage: not open fd");
        return -1;
    }

    ret = zygoteSocketClient->sendMessage(msg);
    if (ret == -1) {
        LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketClient_nativeSendMessage: write return -1");
    }

    return ret;
}


