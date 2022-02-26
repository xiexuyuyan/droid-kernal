#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "log/log.h"
#include "zygotesocket/ZygoteSocket.h"

#define SERVICE_NAME "@zygote_socket"

#undef TAG
#define TAG "ZygoteSocketServer.cpp"

namespace droid {
    ZygoteSocketServer* ZygoteSocketServer::instance = nullptr;
    ZygoteSocketServer* ZygoteSocketServer::getInstance() {
        if (instance == nullptr) {
            instance = new ZygoteSocketServer;
        }
        return instance;
    }

    void ZygoteSocketServer::onReceive(const int length, const char* msg) const {
        if (callback != nullptr) {
            callback(msg);
        }
    }

    void ZygoteSocketServer::closeSocket() {
        if (listenFd > 0) {
            close(listenFd);
        }
        if (connFd > 0) {
            close(connFd);
        }
        unlink(SERVICE_NAME);
    }

    int ZygoteSocketServer::startSocket() {
        int ret;
        char buff[1024];
        int recLen;

        int lenCltAddr;
        static struct sockaddr_un srvAddr, cltAddr;

        ret = 0;

        // 1. create listen fd
        listenFd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (listenFd < 0) {
            LOG_E(TAG, "Create socket server listen fd error.");
            ret = -1;
            goto out_unlink;
        }
        srvAddr.sun_family = AF_UNIX;
        strcpy(srvAddr.sun_path, SERVICE_NAME);
        srvAddr.sun_path[0] = 0;

        // 2. bind to listen fd
        if (-1 == bind(listenFd, (struct sockaddr *) &srvAddr, sizeof(srvAddr))) {
            LOG_E(TAG, "Bind to listen fd failed.");
            ret = -1;
            goto out_close_listen_fd;
        }

        // 3. listen
        if (-1 == listen(listenFd, 1)) {
            LOG_E(TAG, "Listen client connect request failed.");
            ret = -1;
            goto out_close_listen_fd;
        }

        // 4. accept
        lenCltAddr = sizeof(cltAddr);
        while (true) {
            connFd = accept(listenFd, (struct sockaddr *) &cltAddr, (socklen_t *) &lenCltAddr);
            if (connFd < 0) {
                ret = -1;
                goto out_close_conn_fd;
            }

            // 5. read
            memset(buff, 0, 1024);
            recLen = (int) read(connFd, buff, sizeof(buff));
            LOG_D(TAG, ("recv msg: len = "
                        + std::to_string(recLen)
                        + ", msg = " + buff).c_str());
            onReceive(recLen, buff);
        }

        goto out_ret;

        // 6. close
out_close_conn_fd:
        close(connFd);
out_close_listen_fd:
        close(listenFd);
out_unlink:
        unlink(SERVICE_NAME);
out_ret:
        return ret;
    }

    bool ZygoteSocketServer::isOpen() {
        return (listenFd > 0);
    }
}