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

int main() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    test_request_encode();
    test_request_decode();
}
