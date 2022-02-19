#include <sys/time.h>

#include <cstring>
#include <iostream>
#include <ctime>
#include <jni.h>


JavaVM* mJvm = nullptr;
JNIEnv* mEnv = nullptr;

void printCurrentTime();

void jvmInit(JavaVM* jvm, JNIEnv* env);
void jvmDestroy(JavaVM* jvm, JNIEnv* env);

void loadRoot(JNIEnv* env);

int main() {
    jvmInit(mJvm, mEnv);
    loadRoot(mEnv);
    jvmDestroy(mJvm, mEnv);
    return 0;
}

void loadRoot(JNIEnv* env) {
    jclass rooClz = env->FindClass("Root");
    if (rooClz == nullptr) {
        printCurrentTime();
        std::cout<<"CreateJvm.cpp find Root.class failed!"<<std::endl;
    } else {
        printCurrentTime();
        std::cout<<"CreateJvm.cpp find Root.class success!"<<std::endl;
        jmethodID mainMethod = env->GetStaticMethodID(
                rooClz, "main", "([Ljava/lang/String;)V");
        if (mainMethod == nullptr) {
            printCurrentTime();
            std::cout<<"CreateJvm.cpp find Root.main(String[]) failed!"<<std::endl;
        } else {
            printCurrentTime();
            std::cout<<"CreateJvm.cpp find Root.main success!"<<std::endl;
            env->CallStaticVoidMethod(rooClz, mainMethod, nullptr);
        }
    }
}

void jvmInit(JavaVM* jvm, JNIEnv* env) {
    JavaVMInitArgs vmInitArgs;
    JavaVMOption vmOption[3];
    jint ret;

    vmInitArgs.version = JNI_VERSION_1_8;
    vmInitArgs.nOptions = 0;
    vmInitArgs.ignoreUnrecognized = false;

    {
        char vmOptionClassPath[1024] = "-Djava.class.path=";
        char systemServerClassPath[128] =
                "/mnt/c/Users/23517/IdeaProjects/Dust/build/classes/java/main/";
        strcat(vmOptionClassPath, systemServerClassPath);

        vmOption[0].optionString = vmOptionClassPath;
        vmInitArgs.nOptions++;

        printCurrentTime();
        std::cout<<"CreateJvm.cpp vmOption[classpath]: "<<vmOption[0].optionString<<std::endl;


        char vmOptionLibraryPath[1024] = "-Djava.library.path=";
        char droidRuntimeJniLibraryPath[128] =
                "/mnt/c/Users/23517/CLionProjects/untitled/out/lib/frameworks/droid/jni";
        strcat(vmOptionLibraryPath, droidRuntimeJniLibraryPath);

        vmOption[1].optionString = vmOptionLibraryPath;
        vmInitArgs.nOptions++;

        printCurrentTime();
        std::cout<<"CreateJvm.cpp vmOption[libpath]: "<<vmOption[1].optionString<<std::endl;

    }

    if (vmInitArgs.nOptions > 0) {
        vmInitArgs.options = vmOption;
    }

    ret = JNI_CreateJavaVM(&jvm, (void **)&env, &vmInitArgs);

    if (ret < 0) {
        printCurrentTime();
        std::cout<<"CreateJvm.cpp create jvm error, code = "<<ret<<std::endl;
        return;
    }

    mJvm = jvm;
    mEnv = env;
}

void jvmDestroy(JavaVM* jvm, JNIEnv* env) {
    jvm->DestroyJavaVM();
    jvm = nullptr;
    env = nullptr;
    printCurrentTime();
    std::cout<<"Exit"<<std::endl;
}

void printCurrentTime() {
    struct timeval curTime{};
    gettimeofday(&curTime, nullptr);
    time_t curT = curTime.tv_sec;
    tm* dateTime = localtime(&curT);
    long uTime = curTime.tv_usec % (1000*1000);
    double ufTime = (double)uTime / 1000;
    std::cout<<"["
             <<dateTime->tm_year + 1900
             <<"-"<<dateTime->tm_mon + 1
             <<"-"<<dateTime->tm_mday
             <<" "<<dateTime->tm_hour
             <<":"<<dateTime->tm_min
             <<":"<<dateTime->tm_sec
             <<" "<<ufTime<<"]";
}