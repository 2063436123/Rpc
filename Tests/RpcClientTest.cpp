//
// Created by Hello Peter on 2021/11/12.
//

#include "../src/include/RpcClient.h"

using namespace std;

void test1() {
    RpcClient client(InAddr("127.0.0.1:5556"));
    std::string ret = client.sync_call<std::string>("say_hello", std::string("world"));
    cout << ret << endl;

    client.sync_call<void>("make_high");
    UDT udt_ret = client.sync_call<UDT>("enforce_udt", UDT(10, "hello"));
    cout << udt_ret.a_ << " " << udt_ret.b_ << endl;
    cout << "client end" << endl;
    client.stop();
}

void test2() {
    RpcClient client(InAddr("127.0.0.1:5556"));

    client.async_call<std::string>(10, [](ErrorCode ec, std::string str) {
        cout << "ret: " << str << endl;
    }, "say_hello", std::string("world"));

    client.async_call<void>(10, [](ErrorCode ec) {
        cout << "ret void" << endl;
    }, "make_high");

    cout << "client end" << endl;
    client.join();
}

void test3() {
    RpcClient client(InAddr("127.10.0.1:5556"));
    client.subscribe("hello", [](std::string str) {
        std::cout << "broadcast: " << str << std::endl;
    });
    std::string msg;
    getline(cin, msg);
    client.publish("hello", msg);
    cout << "client end" << endl;
    client.join();
}

int main() {
    test3();
}
