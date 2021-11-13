//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_RPCCLIENT_H
#define TESTLINUX_RPCCLIENT_H

#include "./const_vars.h"
#include "./Util.h"
#include <TcpClient.h>

// 1. 服务发现
// 2. 负载均衡，选择合适的服务器进行RPC
// 3. （主动）与服务器建立连接，发送RPC请求，接受RPC响应。（心跳包）
// 4. 封包解包
class RpcClient {
public:
    explicit RpcClient(InAddr addr) : client_(&loop_, Codec(HeaderHelper::IsHeader)), helper_(handle_response) {
        client_.setConnMsgCallback([this](TcpConnection *conn) { helper_.read_head(conn); });
        main_conn = client_.connect(addr);
    }

private:
    static void handle_response(TcpConnection *conn) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        std::cout << "response: " << std::string(conn->readBuffer().peek(), header.body_len) << std::endl;
    }

    // todo template declaration
    void call() {

    }

    void start(int helperThreads = 0) {
        client_.addWorkerThreads(helperThreads);
        client_.join();
    }

    TcpConnection *main_conn;
    HeaderHelper helper_;
    EventLoop loop_;
    TcpClient client_;
};

#endif //TESTLINUX_RPCCLIENT_H
