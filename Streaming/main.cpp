#include <iostream>

#include "server_ws.hpp"

int main()
{
    std::cout << "Streaming test";

    webpp::ws_server ws();

    return 0;
}