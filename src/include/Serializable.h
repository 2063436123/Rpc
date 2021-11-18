//
// Created by Hello Peter on 2021/11/14.
//

#ifndef TESTLINUX_SERIALIZABLE_H
#define TESTLINUX_SERIALIZABLE_H
#include <string>
#include <cassert>

template <typename T>
class Serializable {
public:
    // 所有子类都要实现下面4种方法.
    Serializable() = default;
    Serializable(const Serializable&) = default;
    static std::string encode(const Serializable&) {
        assert(0);
    }
    static T decode(const char*, size_t) {
        assert(0);
    }
};

#endif //TESTLINUX_SERIALIZABLE_H
