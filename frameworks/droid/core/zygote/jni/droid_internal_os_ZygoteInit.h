/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class droid_internal_os_ZygoteInit */

#ifndef _Included_droid_internal_os_ZygoteInit
#define _Included_droid_internal_os_ZygoteInit
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeForkSystemServer
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeForkSystemServer
  (JNIEnv *, jclass, jint);

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeGetPid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeGetPid
  (JNIEnv *, jclass);

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeGetParentPid
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_droid_internal_os_ZygoteInit_nativeGetParentPid
  (JNIEnv *, jclass);

/*
 * Class:     droid_internal_os_ZygoteInit
 * Method:    nativeExit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_droid_internal_os_ZygoteInit_nativeExit
  (JNIEnv *, jclass, jint);

#ifdef __cplusplus
}
#endif
#endif