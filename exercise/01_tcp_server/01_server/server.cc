#include "server.h"

Server::~Server() {
    this->close();
}

