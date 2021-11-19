//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_RPCSERVER_H
#define TESTLINUX_RPCSERVER_H

#include "common_udt.h"
#include "Util.h"
#include "ServiceRegistrationDiscovery.h"
#include <TcpServer.h>

// 1. 服务注册
// 2. 监听RPC请求
// 3. （被动）与客户端建立连接，接受RPC请求，发送RPC响应。（心跳包）
// 4. 解包封包 + 回调对应本地函数
class RpcServer {
public:
    explicit RpcServer(InAddr addr) : server_(Socket::makeNewSocket(), addr, &loop_, Codec(HeaderHelper::IsHeader)),
                                      ip_port_str_(addr.ipPortStr()) {
        server_.setConnMsgCallback([this](TcpConnection *conn) {
            HeaderHelper::read_head(conn, [this](TcpConnection *conn) { forward_request(conn); });
        });
    }

    template<typename Func>
    // todo1 const Func& -> Func?
    void register_service(const std::string &service_name, const Func &func) {
        registrant_.register_service(service_name, ip_port_str_);
        // note: 必须用decay_t将Func转换为函数指针类型
        service_callbacks_[service_name] = std::bind(invoker < std::decay_t<Func>>, std::move(
                func), std::placeholders::_1);
    }

    void start(int helperThreads = 0) {
        server_.start(helperThreads);
    }

private:
    template<typename Func>
    static std::string invoker(const Func &func, std::string body) {
        using args_type = typename function_traits<Func>::args_tuple;
        auto tpl = MessageCodec::unpack<args_type>(body);
        std::string response_body;
        invoke(func, std::move(tpl), response_body);
        return response_body;
    }

    void handle_normal_req(TcpConnection *conn) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        std::string body(conn->readBuffer().peek(), header.body_len);
        conn->readBuffer().retrieve(header.body_len);

        auto service_name = MessageCodec::unpack<std::string>(body);
        auto iter = service_callbacks_.find(service_name);
        if (iter == service_callbacks_.end()) {
            Sender::send(conn, header.request_id, RequestType::failure_response, "no such service.");
            return;
        }
        std::string response_body;
        try {
            response_body = iter->second(std::move(body));
        } catch (const std::exception &e) {
            Sender::send(conn, header.request_id, RequestType::failure_response, e.what());
            return;
        }
        Sender::send(conn, header.request_id, RequestType::success_response, response_body);
    }

    void forward_request(TcpConnection *conn) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        assert(header.type == RequestType::normal_req); // for test
        if (header.type == RequestType::normal_req) {
            if (conn->readBuffer().readableBytes() < header.body_len)
                return;
            handle_normal_req(conn);
        } else if (header.type == RequestType::test) {
            assert(conn->readBuffer().readableBytes() >= header.body_len);
            Sender::send(conn, 0, RequestType::test,
                         std::string(conn->readBuffer().peek(), header.body_len) + ", hello~");
            conn->readBuffer().retrieve(header.body_len);
        } else {
            // todo
        }
        header.type = RequestType::unparsed; // ready for next request
    }

    using CallbackType = std::function<std::string(std::string)>;
    std::unordered_map<std::string, CallbackType> service_callbacks_;

    ServiceRegistrationDiscovery registrant_;
    std::string ip_port_str_;
    EventLoop loop_;
    TcpServer server_;
};

#endif //TESTLINUX_RPCSERVER_H
