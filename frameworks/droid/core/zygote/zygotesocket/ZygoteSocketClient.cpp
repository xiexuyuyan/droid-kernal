#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "log/log.h"
#include "zygotesocket/ZygoteSocket.h"

#define SERVICE_NAME "@zygote_socket"

#undef TAG
#define TAG "ZygoteSocketClient.cpp"

namespace droid {
    ZygoteSocketClient* ZygoteSocketClient::instance = nullptr;
    ZygoteSocketClient* ZygoteSocketClient::getInstance() {
        if (instance == nullptr) {
            instance = new ZygoteSocketClient;
        }
        return instance;
    }

    int ZygoteSocketClient::openSocket() {
        int ret;
        static struct sockaddr_un srvAddr;

        ret = 0;
        // 1. create socket client
        connFd = socket(PF_UNIX, SOCK_STREAM, 0);
        if (connFd < 0) {
            LOG_E(TAG, "Create socket client error.");
            ret = -1;
            goto out_unlink;
        }
        srvAddr.sun_family = AF_UNIX;
        strcpy(srvAddr.sun_path, SERVICE_NAME);
        srvAddr.sun_path[0] = 0;

        // 2. connect server
        if (-1 == connect(connFd, (struct sockaddr *) &srvAddr, sizeof(srvAddr))) {
            LOG_E(TAG, "Connect to server failed.");
            ret = -1;
            goto out_close_conn_fd;
        }

        goto out_ret;

out_close_conn_fd:
        close(connFd);
out_unlink:
        unlink(SERVICE_NAME);
out_ret:
        return ret;
    }

    int ZygoteSocketClient::sendMessage(const char *msg) const {
        int writeLen;

        // 3. send message
        LOG_D(TAG, ("send msg: len = "
                    + std::to_string(strlen(msg))
                    + ", msg = " + msg).c_str());
        writeLen = (int) write(connFd, msg, strlen(msg));

        return writeLen;
    }

    void ZygoteSocketClient::closeSocket() const {
        if (connFd > 0) {
            close(connFd);
        }
        unlink(SERVICE_NAME);
    }

    bool ZygoteSocketClient::isOpen() const {
        LOG_D(TAG, ("isOpen: fd = " + std::to_string(connFd)).c_str());
        return (connFd > 0);
    }
}