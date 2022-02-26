#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

static void signalHandler(int signal);
static void cycleHandler(int signal);

int main(int argc, char* argv[]) {
    pid_t pid;

    if (signal(SIGCHLD, signalHandler) == SIG_ERR) {
    // if (signal(SIGCHLD, cycleHandler) == SIG_ERR) {
        std::cout<<"signal err: "<<errno<<std::endl;
    }

    if ((pid = fork()) == 0) {
        sleep(1);
        std::cout<<"in child,        pid = "<<getpid()<<std::endl;
        exit(0);
    } else {
        std::cout<<"in parent, child pid = "<<pid<<std::endl;
        sleep(5);
    }

    return 0;
}

static void cycleHandler(int signal) {
    do {
        pid_t pid;
        if ((pid = waitpid(-1, nullptr, WNOHANG)) > 0) {
            std::cout<<"wait success, child pid is "<<pid<<std::endl;
        } else {
            std::cout<<"no child exit."<<std::endl;
            break;
        }
    } while (true);
}

static void signalHandler(int signal) {
    int temp = errno;
    if (waitpid(-1, nullptr, 0) < 0) {
        std::cout<<"signal handler error: "<<errno<<std::endl;
    }
    std::cout<<"child exit"<<std::endl;
    errno = temp;
}