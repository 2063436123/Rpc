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
    explicit RpcServer(InAddr addr) : timer_(&loop_),
                                      server_(Socket::makeNewSocket(), addr, &loop_, Codec(HeaderHelper::IsHeader)),
                                      ip_port_str_(addr.ipPortStr()) {
        server_.setConnEstaCallback([this](TcpConnection *conn) {
            std::cout << "newConn: " << conn->socket().fd() << std::endl;
            conn->setData((char *) new RpcMeta);
            start_heartbeat(conn);
        });
        server_.setConnMsgCallback([this](TcpConnection *conn) {
            HeaderHelper::read_head(conn, [this](TcpConnection *conn) { forward_request(conn); });
        });
        server_.setConnCloseCallback([](TcpConnection *conn) {
            delete (RpcMeta*)conn->data();
        });
    }

    template<typename Func>
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
    void start_heartbeat(TcpConnection *conn) {
        auto &heartbeat_handler = (reinterpret_cast<RpcMeta *>(conn->data()))->heartbeat_handler;
        heartbeat_handler = timer_.addOneTask(DEFAULT_HEARTBEAT_INTERVAL * DEFAULT_RETRY_TIMES, [this]() {
            Logger::info("一个客户端失去连接!\n");
        });
        auto &helper_handler = (reinterpret_cast<RpcMeta *>(conn->data()))->helper_handler;
        helper_handler = timer_.addTimedTask(0, DEFAULT_HEARTBEAT_INTERVAL, [conn]() {
            Sender::send(conn, 0, RequestType::heartbeat, "");
        });
    }

    template<typename Func>
    static std::string invoker(const Func &func, std::string body) {
        using args_type = typename function_traits<Func>::args_tuple;
        auto tpl = MessageCodec::unpack<args_type>(body);
        std::string response_body;
        invoke(func, std::move(tpl), response_body);
        return response_body;
    }

    void handle_normal_req(TcpConnection *conn, const std::string& body) {
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;

        auto service_name = MessageCodec::unpack<std::string>(body);
        auto iter = service_callbacks_.find(service_name);
        if (iter == service_callbacks_.end()) {
            Sender::send(conn, header.request_id, RequestType::failure_response, "no such service.");
            return;
        }
        std::string response_body;
        try {
            response_body = iter->second(body);
        } catch (const std::exception &e) {
            Sender::send(conn, header.request_id, RequestType::failure_response, e.what());
            return;
        }
        Sender::send(conn, header.request_id, RequestType::success_response, response_body);
    }

    void forward_request(TcpConnection *conn) {
        // 1. 验证header并提取body
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        if (conn->readBuffer().readableBytes() < header.body_len)
            return;
        std::string body(conn->readBuffer().peek(), header.body_len);
        conn->readBuffer().retrieve(header.body_len);

        // 2. 分类处理不同消息
        if (header.type == RequestType::normal_req) {
            handle_normal_req(conn, body);
        } else if (header.type == RequestType::heartbeat) {
            auto &heartbeat_handler = (reinterpret_cast<RpcMeta *>(conn->data()))->heartbeat_handler;
            heartbeat_handler.resetOneTask(DEFAULT_HEARTBEAT_INTERVAL * DEFAULT_RETRY_TIMES);
        } else if (header.type == RequestType::subscribe){
            auto channelName = MessageCodec::unpack<std::string>(body);
            subscribers_[channelName].push_back(conn);
            //
        } else if (header.type == RequestType::publish) {
            auto infos = MessageCodec::unpack<std::tuple<std::string, std::string>>(body);
            const auto& subs = subscribers_[std::get<0>(infos)];
            for (auto sub_conn : subs) {
                // todo 验证conn的有效性
                Sender::send(sub_conn, 0, RequestType::broadcast, body);
            }
        } else {
            // todo
        }

        // 3. 清空header，进行下一次消息监听
        header.type = RequestType::unparsed; // ready for next request
        if (HeaderHelper::IsHeader(conn))
            HeaderHelper::read_head(conn, [this](TcpConnection *conn) { forward_request(conn); });
    }

    using CallbackType = std::function<std::string(std::string)>;
    std::unordered_map<std::string, CallbackType> service_callbacks_;
    std::unordered_map<std::string, std::vector<TcpConnection*>> subscribers_;

    ServiceRegistrationDiscovery registrant_;
    std::string ip_port_str_;
    EventLoop loop_;
    Timer timer_;
    TcpServer server_;
};

#endif //TESTLINUX_RPCSERVER_H
