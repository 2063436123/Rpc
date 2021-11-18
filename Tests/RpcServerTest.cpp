//
// Created by Hello Peter on 2021/11/12.
//

#include "../src/include/RpcServer.h"
#include <iostream>
#include <utility>

using namespace std;

void make_high() {
    cout << "I am high" << endl;
}

std::string say_hello(std::string message) {
    return "hello, " + message;
}

int main() {
    RpcServer server(InAddr("0.0.0.0:5556"));
    server.register_service("say_hello", say_hello);
    server.register_service("make_high", make_high);
    server.start();
}