//
// Created by Hello Peter on 2021/11/13.
//

#ifndef TESTLINUX_UTIL_H
#define TESTLINUX_UTIL_H
#include <TcpConnection.h>

class HeaderHelper {
public:
    explicit HeaderHelper(std::function<void(TcpConnection* conn)> cb) : cb_(std::move(cb)) {    }

    static void parseRpcHeader(const char *p, RpcHeader &header) {
        header.request_id = *(uint64_t *) p;
        header.type = *(RequestType *) (p + 8);
        header.body_len = *(uint32_t *) (p + 9);
    }

    static bool IsHeader(TcpConnection *conn) {
        return conn->readBuffer().readableBytes() >= RPC_HEADER_SIZE;
    }

    void read_head(TcpConnection *conn) {
        if (!conn->data()) {
            conn->setData((char *) new RpcMeta);
        }
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        if (header.type == RequestType::unparsed) {
            parseRpcHeader(conn->readBuffer().peek(), header);
            conn->readBuffer().retrieve(RPC_HEADER_SIZE);
        }
        assert(header.type != RequestType::unparsed);
        cb_(conn);
    }

private:
    std::function<void(TcpConnection* conn)> cb_;
};
#endif //TESTLINUX_UTIL_H
