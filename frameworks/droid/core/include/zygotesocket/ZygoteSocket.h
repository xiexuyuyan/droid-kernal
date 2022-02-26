#ifndef _ZYGOTE_SOCKET_H
#define _ZYGOTE_SOCKET_H

namespace droid {
    class ZygoteSocketClient {
    private:
        static ZygoteSocketClient* instance;
        int connFd;

        ZygoteSocketClient() = default;
    public:
        static ZygoteSocketClient* getInstance();

        bool isOpen() const;
        int openSocket();
        int sendMessage(const char *msg) const;
        void closeSocket() const;
    };

    class ZygoteSocketServer {
    private:
        static ZygoteSocketServer* instance;
        int listenFd = 0;
        int connFd = 0;

        ZygoteSocketServer() = default;
    public:
        static ZygoteSocketServer* getInstance();

        bool isOpen();
        int startSocket();
        void closeSocket();

        void(* callback)(const char*) = nullptr;
        void onReceive(int length, const char* msg) const;
    };
}

#endif //_ZYGOTE_SOCKET_H