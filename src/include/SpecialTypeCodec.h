//
// Created by Hello Peter on 2021/11/16.
//

#ifndef TESTLINUX_SPECIALTYPECODEC_H
#define TESTLINUX_SPECIALTYPECODEC_H

#include <tuple>
#include <cmath>

static const uint32_t unit_amount = std::pow(2, 7);
static const uint32_t oneByteCanHave = unit_amount - 1;
static const uint32_t twoBytesCanHave = unit_amount * unit_amount - 1;
static const uint32_t threeBytesCanHave = unit_amount * unit_amount * unit_amount - 1;
static const uint32_t fourBytesCanHave = unit_amount * unit_amount * unit_amount * unit_amount - 1;
static const uint64_t fiveBytesCanHave = (uint64_t)unit_amount * unit_amount * unit_amount * unit_amount * unit_amount - 1;

static bool msb(unsigned char ch) {
    return ch & 0x80;
}

static unsigned char left(unsigned char ch) {
    return ch & 0x7f;
}

static uint32_t retrieve_int_from_binary(const char *&p) {
    /*uint32_t ret = *(reinterpret_cast<const uint32_t *>(p));
    p += 4;*/
    uint32_t ret = 0;
    uint32_t rate = 1;
    do {
        ret += rate * left(*p);
        rate *= unit_amount;
    } while (msb(*p++));

    return ret;
}

static std::string trans_int_to_binary(uint32_t len) {
    std::string ret;
    unsigned char v;
    if (len <= oneByteCanHave) {
        v = len & 0x7f;
        ret += v;
    } else if (len <= twoBytesCanHave) {
        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v;
    } else if (len <= threeBytesCanHave) {
        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v;

    } else if (len <= fourBytesCanHave) {
        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v;

    } else {
        if (len >= fiveBytesCanHave) {
            std::cout << "len: " << len << " vs " << fiveBytesCanHave << std::endl;
            assert(len < fiveBytesCanHave);
        }
        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x7f;
        ret += v | 0x80;
        len >>= 7;

        v = len & 0x0f;
        ret += v;
    }
    /* const char *p = (char *) &len;
    ret.push_back(*p++);
    ret.push_back(*p++);
    ret.push_back(*p++);
    ret.push_back(*p++); */
    return ret;
}

template<typename T>
struct Adaptor {
public:
    // 把T类型对象的内容序列化到buffer中
    static void pack(std::string &buffer, const T &t) {
        std::string code = T::encode(t);
        uint32_t len = code.size();
        buffer += trans_int_to_binary(len) + code;
    }

    // 从p指向的字节数组中反序列化出T类型对象，并将p移动到下一个对象开始处
    static void unpack(const char *&p, T &t) {
        uint32_t len = retrieve_int_from_binary(p);
        t = T::decode(p, len);
        p += len;
    }
};

// string specialization
template<>
struct Adaptor<std::string> {
public:
    static void pack(std::string &buffer, const std::string &str) {
        const std::string &code = str;
        uint32_t len = code.size();
        buffer += trans_int_to_binary(len) + code;
    }

    static void unpack(const char *&p, std::string &str) {
        uint32_t len = retrieve_int_from_binary(p);
        str = std::string(p, len);
        p += len;
    }
};

// tuple specialization
template<typename T, unsigned N>
struct tuple_unpack_helper {
public:
    static void unpack(const char *p, T &tpl) {
        constexpr size_t cur_size = std::tuple_size_v<std::decay_t<T>> - N;
        using arg_type = std::tuple_element_t<cur_size, std::decay_t<T>>;
        Adaptor<arg_type>::unpack(p, get<cur_size>(tpl));
        tuple_unpack_helper<T, N - 1>::unpack(p, tpl);
    }
};

template<typename T>
struct tuple_unpack_helper<T, 0> {
public:
    static void unpack(const char *p, T &tpl) {
        // reach boundary, do nothing
    }
};

template<typename... Args>
struct Adaptor<std::tuple<Args...>> {
public:
    static void pack(std::string &buffer, const std::tuple<Args...> &t) {
        std::string code = t.encode();
        uint32_t len = code.size();
        buffer += trans_int_to_binary(len) + code;
    }

    static void unpack(const char *&p, std::tuple<Args...> &t) {
        tuple_unpack_helper<decltype(t), sizeof...(Args)>::unpack(p, t);
    }

};


#endif //TESTLINUX_SPECIALTYPECODEC_H
