#include "zygotesocket/ZygoteSocket.h"

int main(int argc, char* argv[]) {
    auto* zygoteSocketClient = droid::ZygoteSocketClient::getInstance();
    zygoteSocketClient->openSocket();
    zygoteSocketClient->sendMessage("hello world");
}