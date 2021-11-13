//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_RPCSERVER_H
#define TESTLINUX_RPCSERVER_H

#include "./const_vars.h"
#include "./Util.h"
#include <TcpServer.h>

// 1. 服务注册
// 2. 监听RPC请求
// 3. （被动）与客户端建立连接，接受RPC请求，发送RPC响应。（心跳包）
// 4. 解包封包 + 回调对应本地函数
class RpcServer {
public:
    explicit RpcServer(InAddr addr) : server_(Socket::makeNewSocket(), addr, &loop_, Codec(HeaderHelper::IsHeader)),
                                      helper_([this](TcpConnection *conn) { forward_request(conn); }) {
        server_.setConnMsgCallback([this](TcpConnection *conn) { helper_.read_head(conn); });
    }

    void forward_request(TcpConnection *conn) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        if (header.type == RequestType::normal_req) {
            if (conn->readBuffer().readableBytes() >= header.body_len)
                read_body(conn);
        } else {
            // todo
        }
    }

    void read_body(TcpConnection *conn) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        std::cout << "body: " << std::string(conn->readBuffer().peek(), header.body_len) << std::endl;
    }

    template<typename Func>
    void register_service(const std::string &service_name, Func func) {
        // todo
    }

    void start(int helperThreads = 0) {
        server_.start(helperThreads);
    }

private:
    HeaderHelper helper_;
    EventLoop loop_;
    TcpServer server_;
};

#endif //TESTLINUX_RPCSERVER_H
