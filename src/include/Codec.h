//
// Created by Hello Peter on 2021/11/16.
//

#ifndef TESTLINUX_CODEC_H
#define TESTLINUX_CODEC_H

#include <string>
#include "Util.h"

static uint32_t retrieve_int_from_binary(const char* p) {
    return *(reinterpret_cast<const uint32_t*>(p));
}

static std::string trans_int_to_binary(uint32_t len) {
    std::string ret;
    const char *p = (char *) &len;
    ret.push_back(*p++);
    ret.push_back(*p++);
    ret.push_back(*p++);
    ret.push_back(*p++);
    return ret;
}

template<typename T, unsigned N>
struct unpack_helper {
public:
    static void unpack(const char* p, T& tpl) {
        constexpr size_t cur_size = std::tuple_size_v<T> - N;
        using arg_type = std::tuple_element_t<cur_size, T>;
        uint32_t len = retrieve_int_from_binary(p);
        p += 4;
        get<cur_size>(tpl) = arg_type::decode(p, len);
        p += len;
        unpack_helper<T, N - 1>::unpack(p, tpl);
    }
};

template<typename T>
struct unpack_helper<T, 0> {
public:
    static void unpack(const char* p, T& tpl) {
        assert(p && *p == '\0');
        // reach boundary, do nothing
    }
};

template<typename Arg, typename... Args>
static void pack_helper(std::string &buffer, Arg &&arg, Args &&... args) {
    pack_helper(buffer, arg);
    pack_helper(buffer, args...);
}

template<typename Arg>
static void pack_helper(std::string &buffer, Arg &&arg) {
    std::string code = arg.encode();
    uint32_t len = code.size();
    buffer += trans_int_to_binary(len) + code;
}

class Codec {
public:
    // 对于多个参数的组包，要添加len字段来指定分隔位置，解包同理.
    template<typename... Args>
    static std::string pack(Args &&... args) {
        std::string buffer;
        pack_helper(buffer, args...);
        return buffer;
    }

    // \return tuple<Args>
    template<typename... Args>
    static std::tuple<Args...> unpack(const std::string &str) {
        std::tuple<Args...> ret;
        unpack_helper<decltype(ret), std::tuple_size_v<decltype(ret)>>::unpack(str.c_str(), ret);
        return ret;
    }
};

#endif //TESTLINUX_CODEC_H
