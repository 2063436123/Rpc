//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_RPCCLIENT_H
#define TESTLINUX_RPCCLIENT_H

#include "ErrorCode.h"
#include "common_udt.h"
#include "Util.h"
#include <TcpClient.h>
#include <Timer.h>
#include <unordered_set>
#include <utility>

// RpcClient仅支持被单个线程调用, 虽然其内部client可以是多线程的.
// 1. 服务发现
// 2. 负载均衡，选择合适的服务器进行RPC
// 3. （主动）与服务器建立连接，发送RPC请求，接受RPC响应。（心跳包）
// 4. 封包解包
class RpcClient {
public:
    explicit RpcClient(InAddr addr) : next_request_id_(1), timer_(&loop_),
                                      client_(&loop_, Codec(HeaderHelper::IsHeader)) {
        client_.setConnEstaCallback([this](TcpConnection *conn) {
            conn->setData((char *) new RpcMeta);
            start_heartbeat();
        });
        client_.setConnMsgCallback([this](TcpConnection *conn) {
            HeaderHelper::read_head(conn, [this](TcpConnection *conn) { handle_response(conn); });
        });
        client_.setConnCloseCallback([this](TcpConnection *conn) {
            delete (RpcMeta *) conn->data();
            Logger::info("服务器失去连接!\n");
            stop();
        });
        client_.addWorkerThreads(1); // 必要的，因为主线程可能因为SYNC_CALL而阻塞，此时由工作线程来处理连接上的读写事件
        main_conn = client_.connect(addr);
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
        sync_call<Ret>(DEFAULT_REQUEST_TIMEOUT, service_name, args...);
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<!std::is_void_v<Ret>, Ret>
    sync_call(size_t timeout, const std::string &service_name, Args... args) {
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);

        Ret ret;
        ErrorCode ec;
        sent_requests_[request_id] = [&ret, &ec](ErrorCode eci, std::string body) {
            ret = MessageCodec::unpack<Ret>(body);
            ec = std::move(eci);
        };
        std::unique_lock ul(cond_mutex_);
        if (cv_.wait_for(ul, std::chrono::milliseconds(timeout)) == std::cv_status::timeout) {
            throw std::runtime_error("request timeout!");
        };
        if (ec) {
            throw std::runtime_error(ec.cause());
        }
        return ret;
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<std::is_void_v<Ret>>
    sync_call(size_t timeout, const std::string &service_name, Args... args) {
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);

        ErrorCode ec;
        sent_requests_[request_id] = [&ec](ErrorCode eci, const std::string &) {
            ec = std::move(eci);
        };
        std::unique_lock ul(cond_mutex_);
        if (cv_.wait_for(ul, std::chrono::milliseconds(timeout)) == std::cv_status::timeout) {
            throw std::runtime_error("request timeout!");
        }
        if (ec) {
            throw std::runtime_error(ec.cause());
        }
        return;
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<!std::is_void_v<Ret>>
    async_call(size_t timeout, std::function<void(ErrorCode error_code, Ret)> cb, const std::string &service_name,
               Args... args) {
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);
        sent_requests_[request_id] = [cb](ErrorCode ec, std::string body) {
            if (ec)
                cb(ec, {});
            else
                cb(ec, MessageCodec::unpack<Ret>(body));
        };
        return;
    }

    template<typename Ret, typename ...Args>
    typename std::enable_if_t<std::is_void_v<Ret>>
    async_call(size_t timeout, std::function<void(ErrorCode error_code)> cb, const std::string &service_name,
               Args... args) {
        std::string body = MessageCodec::pack(service_name, args...);
        uint64_t request_id = next_request_id_++;
        Sender::send(main_conn, request_id, RequestType::normal_req, body);
        sent_requests_[request_id] = [cb](ErrorCode ec, const std::string &) {
            cb(std::move(ec));
        };
    }

    void subscribe(const std::string &channel, std::function<void(std::string)> cb) {
        Sender::send(main_conn, 0, RequestType::subscribe, MessageCodec::pack(channel));
        subscribed_channels_[channel] = std::move(cb);
    }

    void publish(const std::string &channel, const std::string &message) {
        Sender::send(main_conn, 0, RequestType::publish, MessageCodec::pack(channel, message));
    }

    void stop() {
        client_.stop();
    }

    void join() {
        client_.join();
    }

private:
    void start_heartbeat() {
        auto &heartbeat_handler = (reinterpret_cast<RpcMeta *>(main_conn->data()))->heartbeat_handler;
        heartbeat_handler = timer_.addOneTask(DEFAULT_HEARTBEAT_INTERVAL * DEFAULT_RETRY_TIMES, [this]() {
            Logger::info("服务器失去连接!\n");
            stop();
        });
        auto &helper_handler = (reinterpret_cast<RpcMeta *>(main_conn->data()))->helper_handler;
        helper_handler = timer_.addTimedTask(0, DEFAULT_HEARTBEAT_INTERVAL, [this]() {
            Sender::send(main_conn, 0, RequestType::heartbeat, "");
        });
    }

    void handle_response(TcpConnection *conn) {
        // 1. 验证header并提取body
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        if (conn->readBuffer().readableBytes() < header.body_len)
            return;

        auto req_iter = sent_requests_.find(header.request_id);
        if (header.request_id != 0) {
            if (req_iter == sent_requests_.end())
                throw std::runtime_error(
                        "haven't sent this request!, request_id = " + std::to_string(header.request_id));
            else
                sent_requests_.erase(req_iter);
        }
        std::string body(conn->readBuffer().peek(), header.body_len);
        conn->readBuffer().retrieve(header.body_len);

        // 2. 分类处理不同消息
        if (header.type == RequestType::success_response) {
            req_iter->second(ErrorCode(ErrorCode::NO_ERROR), std::move(body));
            // 同一时刻只会有一个线程阻塞在cv_上, notify_one == notify_all
            cv_.notify_one();
        } else if (header.type == RequestType::failure_response) {
            req_iter->second(ErrorCode(ErrorCode::ERROR, body), "");
        } else if (header.type == RequestType::heartbeat) {
            auto &heartbeat_handler = (reinterpret_cast<RpcMeta *>(conn->data()))->heartbeat_handler;
            heartbeat_handler.resetOneTask(DEFAULT_HEARTBEAT_INTERVAL * DEFAULT_RETRY_TIMES);
        } else if (header.type == RequestType::broadcast) {
            auto infos = MessageCodec::unpack<std::tuple<std::string, std::string>>(body);
            auto sub_iter = subscribed_channels_.find(std::get<0>(infos));
            if (sub_iter == subscribed_channels_.end())
                throw std::runtime_error("haven't subscribed this channel!");
            sub_iter->second(std::get<1>(infos));
        } else {
            // todo
        }

        // 3. 清空header，进行下一次消息监听
        header.type = RequestType::unparsed; // ready for next request
        if (HeaderHelper::IsHeader(conn))
            HeaderHelper::read_head(conn, [this](TcpConnection *conn) { handle_response(conn); });
    }

    using CallbackType = std::function<void(ErrorCode, std::string)>;

    std::unordered_map<uint64_t, CallbackType> sent_requests_;
    std::unordered_map<std::string, std::function<void(std::string)>> subscribed_channels_;

    std::mutex cond_mutex_;
    std::condition_variable cv_;

    TcpConnection *main_conn;
    uint64_t next_request_id_;
    EventLoop loop_;
    Timer timer_;
    TcpClient client_;
};

#endif //TESTLINUX_RPCCLIENT_H
