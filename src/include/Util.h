//
// Created by Hello Peter on 2021/11/13.
//

#ifndef TESTLINUX_UTIL_H
#define TESTLINUX_UTIL_H

#include "const_vars.h"
#include "Serializable.h"
#include "MessageCodec.h"
#include <TcpConnection.h>

class HeaderHelper {
public:
    static void parseRpcHeader(const char *p, RpcHeader &header) {
        header.request_id = *(uint64_t *) p;
        header.type = *(RequestType *) (p + 8);
        header.body_len = *(uint32_t *) (p + 9);
        std::cout << "parse result: " << header.request_id << " " << (int)header.type << " " << header.body_len << std::endl;
       }

    static bool IsHeader(TcpConnection *conn) {
        return conn->readBuffer().readableBytes() >= RPC_HEADER_SIZE;
    }

    static void read_head(TcpConnection *conn, std::function<void(TcpConnection *)> cb_) {
        assert(conn->data());
        auto &header = (reinterpret_cast<RpcMeta *>(conn->data()))->header;
        if (header.type == RequestType::unparsed) {
            parseRpcHeader(conn->readBuffer().peek(), header);
            conn->readBuffer().retrieve(RPC_HEADER_SIZE);
        }
        assert(header.type != RequestType::unparsed);
        assert(cb_);
        cb_(conn);
    }
};

class Sender {
public:
    static void send(TcpConnection *conn, uint64_t request_id, RequestType type, const std::string &body) {
        std::cout << "send from client/server: " << (int)type << " " << body << std::endl;

        uint32_t body_len = static_cast<uint32_t>(body.size());
        conn->writeBuffer().append(reinterpret_cast<char *>(&request_id), sizeof(request_id));
        conn->writeBuffer().append(reinterpret_cast<char *>(&type), sizeof(type));
        conn->writeBuffer().append(reinterpret_cast<char *>(&body_len), sizeof(body_len));
        conn->writeBuffer().append(body.c_str(), body.size());
        conn->send();
    }
};

template<typename T>
struct function_traits;

template<typename Ret, typename Arg, typename... Args>
struct function_traits<Ret(Arg, Args...)> {
public:
    enum {
        arity = sizeof...(Args) + 1
    };

    typedef Ret function_type(Arg, Args...);

    typedef Ret return_type;
    using stl_function_type = std::function<function_type>;

    typedef Ret (*pointer)(Arg, Args...);

    typedef std::tuple<Arg, Args...> tuple_type;
    typedef std::tuple<std::remove_const_t<std::remove_reference_t<Arg>>,
            std::remove_const_t<std::remove_reference_t<Args>>...>
            bare_tuple_type;
    using args_tuple =
    std::tuple<std::string, Arg,
            std::remove_const_t<std::remove_reference_t<Args>>...>;
    using args_tuple_2nd =
    std::tuple<std::string,
            std::remove_const_t<std::remove_reference_t<Args>>...>;
};

template<typename Ret>
struct function_traits<Ret()> {
public:
    enum {
        arity = 0
    };

    typedef Ret function_type();

    typedef Ret return_type;
    using stl_function_type = std::function<function_type>;

    typedef Ret (*pointer)();

    typedef std::tuple<> tuple_type;
    typedef std::tuple<> bare_tuple_type;
    using args_tuple = std::tuple<std::string>;
    using args_tuple_2nd = std::tuple<std::string>;
};

template<typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : function_traits<Ret(Args...)> {
};

template<typename Ret, typename... Args>
struct function_traits<std::function<Ret(Args...)>>
        : function_traits<Ret(Args...)> {
};

template<typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...)>
        : function_traits<ReturnType(Args...)> {
};

template<typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
        : function_traits<ReturnType(Args...)> {
};

template<typename Callable>
struct function_traits : function_traits<decltype(&Callable::operator())> {
};

template<typename Func, size_t... I, typename Arg, typename... Args>
static typename std::result_of_t<Func(Args...)>
call_helper(const Func &func, const std::index_sequence<I...> &, std::tuple<Arg, Args...> tup) {
    return func(std::move(std::get<I + 1>(tup))...);
}

// fixed 频繁报错 no type named ‘type’ in ‘class std::result_of, 是因为Args...误为空，所以Func(Args...)没有匹配的函数故没有返回类型
template<typename Func, typename Arg, typename... Args>
static std::enable_if_t<!std::is_void_v<std::result_of_t<Func(Args...)>>>
invoke(const Func &func, std::tuple<Arg, Args...> args, std::string &body) {
    body = MessageCodec::pack(call_helper(func, std::make_index_sequence<sizeof...(Args)>{}, std::move(args)));
}

template<typename Func, typename Arg, typename... Args>
static std::enable_if_t<std::is_void_v<std::result_of_t<Func(Args...)>>>
invoke(const Func &func, std::tuple<Arg, Args...> args, std::string &body) {
    call_helper(func, std::make_index_sequence<sizeof...(Args)>{}, std::move(args));
}


template<typename T>
struct is_args_serializable_helper;

template<>
struct is_args_serializable_helper<std::tuple<>> {
public:
    constexpr static bool value() {
        return true;
    }
};

#include "SpecialTypeCodec.h"

template<typename Arg, typename ...Args>
struct is_args_serializable_helper<std::tuple<Arg, Args...>> {
public:
    constexpr static bool value() {
        return std::is_base_of_v<Serializable<Arg>, Arg>
               && is_args_serializable_helper<std::tuple<Args...>>::value();
    }
};

template<typename Func>
constexpr bool is_all_args_serializable() {
    using args_tuple = typename function_traits<Func>::tuple_type;
    return is_args_serializable_helper<args_tuple>::value();
}

#endif //TESTLINUX_UTIL_H
