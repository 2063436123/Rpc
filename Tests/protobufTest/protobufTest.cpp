//
// Created by Hello Peter on 2021/11/14.
//

#include <iostream>
#include "message.pb.h"

using namespace google::protobuf;
using namespace std;

std::string buf;

void test_request_encode() {
    Hello::HelloRequest request;
    request.set_str("hello");
    request.add_vec(1);
    request.add_vec(2);
    request.add_vec(3);
    buf = request.SerializeAsString();
}

void test_request_decode() {
    Hello::HelloRequest request;
    request.ParseFromString(buf);
    cout << request.str() << endl;
    for (int i = 0; i < request.vec_size(); i++)
        cout << request.vec(i) << " ";
    cout << endl;
}

class Defer {
public:
    template<typename T>
    explicit Defer(T cb) : cb_(cb) {}

    ~Defer() { cb_(); }

private:
    std::function<void()> cb_;
};

void showBinary(unsigned char ch) {
    unsigned char i = 0x80;
    for (int j = 0; j < 8; j++) {
        if (ch & (i >> j))
            cout << '1';
        else
            cout << '0';
    }
}

void showStr(const string &str) {
    for (char ch: str)
        if (ch >= 0x21)
            cout << ch;
        else
            cout << ' ';
}

void showHex(const string &content) {
    const int bytesOfOneLine = 4;
    string asciis;
    int i;
    for (i = 0; i < content.size(); i++) {
        showBinary(content[i]);
        cout << ' ';
        asciis.push_back(content[i]);
        if ((i + 1) % bytesOfOneLine == 0) {
            showStr(asciis);
            asciis.clear();
            cout << endl;
        }
    }
    if ((i + 1) % bytesOfOneLine != 0) {
        showStr(asciis);
        cout << endl;
    }
}

void showFormat(const Message &message) {
    std::string content;
    message.SerializeToString(&content);
    showHex(content);
}

void test_varint() {
    Hello::Ints ints;
    ints.set_a(0x10);
    ints.set_b(0x7ffffffe);
    ints.set_c(-2);
    ints.set_d(0x10121314);
    showFormat(ints);
}

void test_fixed_ints() {
    Hello::fixed_ints ints;
    ints.set_n(0x10121314);
    ints.set_m(0x0102030405060708);
    showFormat(ints);

}

void test_length_delimited() {
    Hello::Ldls ldls;
    ldls.set_str("hello");
    ldls.add_vec(0x11);
    ldls.add_vec(0xfe);
    showFormat(ldls);
}

void test_complexType() {
    Hello::complexType ct;
    Hello::aInt *aInt = new Hello::aInt;
    aInt->set_a(0x01);
    ct.set_allocated_b(aInt);
    showFormat(ct);
}

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
//    test_request_encode();
//    test_request_decode();
//    test_varint();
//    test_fixed_ints();
//    test_length_delimited();
    test_complexType();
}
