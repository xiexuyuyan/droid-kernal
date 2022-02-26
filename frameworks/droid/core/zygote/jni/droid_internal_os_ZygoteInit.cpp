#include "droid_internal_os_ZygoteInit.h"

#include <sys/prctl.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include "jni.h"

JavaVM* mVm;

/*---------------------------------------------------------------------------*/
static void signalHandler(int signal) {
    int temp = errno;
    if (waitpid(-1, nullptr, 0) < 0) {
        std::cout<<"signal handler error: "<<errno<<std::endl;
    }
    std::cout<<"child exit"<<std::endl;
    errno = temp;
}
/*---------------------------------------------------------------------------*/

JNIEXPORT jint JNICALL JNI_OnLoad(
        JavaVM *vm, void *reserved) {
    std::cout<<"Jni onload in "<<getpid()<<std::endl;
    mVm = vm;
    return JNI_VERSION_1_8;
}

/*---------------------------------------------------------------------------*/

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeForkSystemServer
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeForkSystemServer(
        JNIEnv* env, jclass clz, jint flag) {

    std::cout<<"ZygoteInit.cpp Java_ZygoteInit_nativeForkSystemServer()";
    std::cout<<" flag = "<<flag<<std::endl;

    if (signal(SIGCHLD, signalHandler) == SIG_ERR) {
        // if (signal(SIGCHLD, cycleHandler) == SIG_ERR) {
        std::cout<<"signal err: "<<errno<<std::endl;
    }

    int pid;
    pid = fork();

    if (pid == 0) {
        if (flag == 0) {
            prctl(PR_SET_NAME, "system_server");
        } else {
            char name[10];
            std::string nameString = std::string("app_" + std::to_string(flag));
            strcpy(name, nameString.c_str());
            prctl(PR_SET_NAME, name);
        }
    }

    return pid;
}

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeGetPid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeGetPid(
        JNIEnv* env, jclass clz) {
    int ret;

    ret = getpid();

    return ret;
}

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeGetParentPid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeGetParentPid(
        JNIEnv *, jclass) {
    int ret;

    ret = getppid();

    return ret;
}

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeExit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_droid_internal_os_ZygoteInit_nativeExit(
        JNIEnv* env, jclass clz, jint status) {
    exit(status);
}