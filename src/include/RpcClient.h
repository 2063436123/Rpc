//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_RPCCLIENT_H
#define TESTLINUX_RPCCLIENT_H

#include "common_udt.h"
#include "Util.h"
#include <TcpClient.h>

// RpcClient仅支持被单个线程调用, 虽然其内部client可以是多线程的.
// 1. 服务发现
// 2. 负载均衡，选择合适的服务器进行RPC
// 3. （主动）与服务器建立连接，发送RPC请求，接受RPC响应。（心跳包）
// 4. 封包解包
class RpcClient {
public:
    explicit RpcClient(InAddr addr) : next_request_id_(1), client_(&loop_, Codec(HeaderHelper::IsHeader)) {
        client_.setConnMsgCallback([this](TcpConnection *conn) {
            HeaderHelper::read_head(conn, std::bind(&RpcClient::handle_response, this, std::placeholders::_1));
        });
        client_.addWorkerThreads(1); // 必要的，因为主线程可能因为SYNC_CALL而阻塞，此时由工作线程来处理连接上的读写事件
        main_conn = client_.connect(addr);
    }

    void send_test(const std::string &body) {
        Sender::send(main_conn, 0, RequestType::test, body);
    }

    // call分为两种：有返回值，无返回值.
    template<typename Ret, typename ...Args>
    typename std::enable_if_t<!std::is_void_v<Ret>, Ret>
    sync_call(const std::string &service_name, Args... args) {
        return sync_call<Ret>(DEFAULT_REQUEST_TIMEOUT, service_name, args...);
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<std::is_void_v<Ret>>
    sync_call(const std::string &service_name, Args... args) {
        static_assert(std::is_same_v<Ret, void>, "logic error");
        sync_call<Ret>(DEFAULT_REQUEST_TIMEOUT, service_name, args...);
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<!std::is_void_v<Ret>, Ret>
    sync_call(size_t timeout, const std::string &service_name, Args... args) {
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);
        Ret ret;
        sent_requests_[request_id] = [&ret](std::string body) {
            ret = MessageCodec::unpack<Ret>(body);
        };
        std::unique_lock ul(cond_mutex_);
        if (cv_.wait_for(ul, std::chrono::milliseconds(timeout)) == std::cv_status::timeout) {
            throw std::runtime_error("request timeout!");
        };
        return ret;
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<std::is_void_v<Ret>>
    sync_call(size_t timeout, const std::string &service_name, Args... args) {
        static_assert(std::is_same_v<Ret, void>, "logic error");
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);
        sent_requests_[request_id] = [](std::string body) {

        };
        std::unique_lock ul(cond_mutex_);
        if (cv_.wait_for(ul, std::chrono::milliseconds(timeout)) == std::cv_status::timeout) {
            throw std::runtime_error("request timeout!");
        }
        return;
    }

    // todo
    template<typename Ret, typename ...Args>
    void async_call(size_t timeout, std::function<void()> cb, const std::string &service_name, Args... args) {

    }

    void join() {
        client_.join();
    }

private:
    void handle_response(TcpConnection *conn) {
        std::cout << "in handle_response" << std::endl;
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        assert(header.type == RequestType::success_response); // for test
        if (header.type == RequestType::success_response) {
            if (conn->readBuffer().readableBytes() < header.body_len)
                return;
            auto iter = sent_requests_.find(header.request_id);
            if (iter == sent_requests_.end()) {
                throw std::runtime_error(
                        "haven't sent this request!, request_id = " + std::to_string(header.request_id));
            }
            std::string body(conn->readBuffer().peek(), header.body_len);
            conn->readBuffer().retrieve(header.body_len);
            std::cout << "body: " << body << std::endl;
            iter->second(std::move(body));

            // 同一时刻只会有一个线程阻塞在cv_上, notify_one == notify_all
            cv_.notify_one();
        } else if (header.type == RequestType::failure_response) {
            // todo
        } else {
            // todo
        }
        header.type = RequestType::unparsed; // ready for next request
    }

    using CallbackType = std::function<void(std::string)>;

    std::unordered_map<uint64_t, CallbackType> sent_requests_;

    std::mutex cond_mutex_;
    std::condition_variable cv_;

    TcpConnection *main_conn;
    uint64_t next_request_id_;
    EventLoop loop_;
    TcpClient client_;
};

#endif //TESTLINUX_RPCCLIENT_H
