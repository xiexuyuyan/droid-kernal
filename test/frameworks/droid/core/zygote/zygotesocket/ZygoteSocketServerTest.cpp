#include "zygotesocket/ZygoteSocket.h"

int main(int argc, char* argv[]) {
    auto* zygoteSocketServer = droid::ZygoteSocketServer::getInstance();
    zygoteSocketServer->startSocket();
}