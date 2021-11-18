//
// Created by Hello Peter on 2021/11/14.
//

#ifndef TESTLINUX_SERIALIZATION_H
#define TESTLINUX_SERIALIZATION_H

template <typename Object>
class Serialization {
public:
    virtual std::string encode() const = 0;
    virtual Object decode(const std::string&) const = 0;
};

#endif //TESTLINUX_SERIALIZATION_H
