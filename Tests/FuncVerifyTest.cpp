//
// Created by Hello Peter on 2021/11/16.
//
#include "../src/include/Util.h"
#include <iostream>
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

void Func1(UDT) {

}

void Func2(UDT, UDT) {

}

void Func3(UDT, int) {

}

void Func4(int, UDT) {

}

void Func5(std::string) {

}

int main() {
    cout << boolalpha;
    cout << is_all_args_serializable<decltype(Func1)>() << endl;
    cout << is_all_args_serializable<decltype(Func2)>() << endl;
    cout << is_all_args_serializable<decltype(Func3)>() << endl;
    cout << is_all_args_serializable<decltype(Func4)>() << endl;
    cout << is_all_args_serializable<decltype(Func5)>() << endl;
}
