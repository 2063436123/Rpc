//
// Created by Hello Peter on 2021/11/12.
//

#include "../src/include/RpcClient.h"

using namespace std;

int main() {
    // fixme 如果客户端执行连续两次，那么服务器将SIGSEGV崩溃，定位于HeaderHelper::read_head中的cb_(conn)回调时出错
    RpcClient client(InAddr("127.0.0.1:5556"));
    std::string ret = client.sync_call<std::string>("say_hello", std::string("world"));
    cout << ret << endl;
    ret = client.sync_call<std::string>("say_hello", std::string("world"));
    cout << ret << endl;
    //client.sync_call<void>("make_high");
    cout << "client end" << endl;;
}