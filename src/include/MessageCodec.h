//
// Created by Hello Peter on 2021/11/16.
//

#ifndef TESTLINUX_MESSAGECODEC_H
#define TESTLINUX_MESSAGECODEC_H

#include <string>
#include "SpecialTypeCodec.h"

template<typename Arg>
static void pack_helper(std::string &buffer, const Arg& arg) {
    Adaptor<Arg>::pack(buffer, arg);
}

template<typename Arg, typename... Args>
static void pack_helper(std::string &buffer, const Arg& arg, Args &&... args) {
    Adaptor<Arg>::pack(buffer, arg);
    pack_helper(buffer, std::forward<Args>(args)...);
}

class MessageCodec {
public:
    // 对于多个参数的组包，要添加len字段来指定分隔位置，解包同理.
    template<typename... Args>
    static std::string pack(Args &&... args) {
        std::string buffer;
        pack_helper(buffer, args...);
        return buffer;
    }

    // 只能返回一个参数，多参需求应在调用时指明std::tuple为T
    template<typename T>
    static T unpack(const std::string &str) {
        T t;
        const char* p = str.data();
        Adaptor<T>::unpack(p, t);
        return t;
    }
};

#endif //TESTLINUX_MESSAGECODEC_H
