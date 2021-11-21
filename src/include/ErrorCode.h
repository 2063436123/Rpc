//
// Created by Hello Peter on 2021/11/20.
//

#ifndef TESTLINUX_ERRORCODE_H
#define TESTLINUX_ERRORCODE_H
#include <string>
#include <utility>

class ErrorCode {
public:
    enum error_type {
        NO_ERROR, ERROR
    };

    ErrorCode() : ErrorCode(NO_ERROR) {}

    explicit ErrorCode(error_type type) : ErrorCode(type, "") {

    }

    ErrorCode(error_type type, const char *cause, size_t len) : ErrorCode(type, std::string(cause, len)) {

    }

    ErrorCode(error_type type, std::string cause) : type_(type), error_cause_(std::move(cause)) {

    }

    // 是否有错误发生
    explicit operator bool() const {
        return type_ != NO_ERROR;
    }

    std::string cause() const {
        return error_cause_;
    }
private:
    error_type type_;
    std::string error_cause_;
};

#endif //TESTLINUX_ERRORCODE_H
