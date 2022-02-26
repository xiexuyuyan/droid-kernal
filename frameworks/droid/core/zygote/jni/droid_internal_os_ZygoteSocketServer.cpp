#include "droid_internal_os_ZygoteSocketServer.h"
#include "droid_internal_os_ZygoteSocketServer_Receiver.h"
#include "zygotesocket/ZygoteSocket.h"
#include "log/log.h"

#undef TAG
#define TAG "droid_internal_os_ZygoteSocketServer.cpp"


JNIEXPORT jboolean JNICALL Java_droid_internal_os_ZygoteSocketServer_nativeIsOpen(
        JNIEnv* env, jclass clz) {
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketServer_nativeIsOpen: ");
    auto* zygoteSocketServer = droid::ZygoteSocketServer::getInstance();
    if (zygoteSocketServer != nullptr) {
        return zygoteSocketServer->isOpen();
    }
    return false;
}

JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteSocketServer_nativeOpen(
        JNIEnv* env, jclass clz) {
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketServer_nativeOpen: ");
    auto* zygoteSocketServer = droid::ZygoteSocketServer::getInstance();
    if (zygoteSocketServer != nullptr) {
        if (zygoteSocketServer->isOpen())
            return 0;
        else
            return zygoteSocketServer->startSocket();
    }
    return -1;
}

JNIEXPORT void JNICALL Java_droid_internal_os_ZygoteSocketServer_nativeClose(
        JNIEnv* env, jclass clz) {
    auto* zygoteSocketServer = droid::ZygoteSocketServer::getInstance();
    LOG_D(TAG, "Java_droid_internal_os_ZygoteSocketServer_nativeClose: ");
    if (zygoteSocketServer != nullptr && zygoteSocketServer->isOpen()) {
        zygoteSocketServer->closeSocket();
    }
}

/*---------------------------------------------------------------------------*/
JNIEnv* envGlobal;
jobject receiverGlobal;

static void callback(const char* message);

static void callback(const char* message) {
    LOG_D(TAG, (std::string("callback: ") + message).c_str());
    /* interface Receiver */jclass receiverClz = envGlobal->GetObjectClass(receiverGlobal);
    /* void callback(String msg) */jmethodID callbackFun = envGlobal->GetMethodID(
            receiverClz, "callback", "(Ljava/lang/String;)V");
    jstring msg = envGlobal->NewStringUTF(message);
    envGlobal->CallVoidMethod(receiverGlobal, callbackFun, msg);
}

JNIEXPORT void JNICALL Java_droid_internal_os_ZygoteSocketServer_nativeSetOnReceiveCallback(
        JNIEnv* env, jclass clz, jobject receiverObj) {
    envGlobal = env;
    receiverGlobal = env->NewGlobalRef(receiverObj);
    auto* zygoteSocketServer = droid::ZygoteSocketServer::getInstance();
    zygoteSocketServer->callback = callback;
}
/*---------------------------------------------------------------------------*/
