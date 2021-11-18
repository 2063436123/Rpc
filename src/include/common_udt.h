//
// Created by Hello Peter on 2021/11/18.
//

#ifndef TESTLINUX_COMMON_UDT_H
#define TESTLINUX_COMMON_UDT_H

#include "Serializable.h"
#include <cstring>

class UDT : public Serializable<UDT> {
public:
    UDT() = default;

    UDT(int a, std::string b) : a_(a), b_(std::move(b)) {}

    static std::string encode(const UDT &udt) {
        return std::to_string(udt.a_) + "," + udt.b_;
    }

    static UDT decode(const char *p, size_t len) {
        const char *pIndex = strchr(p, ',');
        return UDT{stoi(std::string(p, pIndex - p)), std::string(pIndex + 1, (len - (pIndex - p) - 1))};
    }

public:
    int a_;
    std::string b_;
};

#endif //TESTLINUX_COMMON_UDT_H
