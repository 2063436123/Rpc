//
// Created by Hello Peter on 2021/11/12.
//

#ifndef TESTLINUX_CONST_VARS_H
#define TESTLINUX_CONST_VARS_H
#include <cstdint>
#include <string>
#include <Timer.h>

enum class RequestType : uint8_t {  unparsed = 0, normal_req, success_response, failure_response, subscribe, publish, broadcast, heartbeat, test};

struct RpcHeader {
    uint64_t request_id;
    RequestType type;
    uint32_t body_len;
};

// 为什么不使用sizeof？因为sizeof结果包含了padding，而我们网络传输逐字段进行，不涉及padding
const int RPC_HEADER_SIZE = sizeof(RpcHeader::request_id) + sizeof(RpcHeader::type) + sizeof(RpcHeader::body_len);

const int DEFAULT_HEARTBEAT_INTERVAL = 3 * 1000; // ms
const int DEFAULT_RETRY_TIMES = 5; // ms

const int DEFAULT_REQUEST_TIMEOUT = 5 * 1000; // ms

// 被保存在TcpConnection类中
struct RpcMeta {
    RpcHeader header{};
    TimerHandler heartbeat_handler;
    TimerHandler helper_handler;
    ~RpcMeta() {
        heartbeat_handler.cancelTimer();
        helper_handler.cancelTimer();
    }
};


#endif //TESTLINUX_CONST_VARS_H
