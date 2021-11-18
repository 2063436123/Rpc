//
// Created by Hello Peter on 2021/11/16.
//
#include "../src/include/Serializable.h"
#include "../src/include/MessageCodec.h"
#include <iostream>
#include <cstring>

using namespace std;

class UDT : public Serializable<UDT> {
public:
    UDT() = default;
    UDT(int a, std::string b) : a_(a), b_(std::move(b)) {}

    static std::string encode(const UDT&udt) {
        return to_string(udt.a_) + "," + udt.b_;
    }

    static UDT decode(const char* p, size_t len) {
        const char* pIndex = strchr(p, ',');
        return UDT{stoi(std::string(p, pIndex - p)), std::string(pIndex + 1, (len - (pIndex - p) - 1))};
    }

public:
    int a_;
    std::string b_;
};

int main() {
    UDT udt1(10, "say"), udt2(20, "hello");
    string ret1 = MessageCodec::pack(std::string("hello world"), udt1, udt2);
    auto tup1 = MessageCodec::unpack<std::tuple<std::string, UDT, UDT>>(ret1);
    cout << get<0>(tup1) << endl;
    cout << get<1>(tup1).a_ << " " << get<1>(tup1).b_ << endl;
    cout << get<2>(tup1).a_ << " " << get<2>(tup1).b_ << endl;
}
