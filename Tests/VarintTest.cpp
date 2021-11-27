//
// Created by Hello Peter on 2021/11/27.
//

#include <cassert>
#include <iostream>
#include <unistd.h>
#include "../src/include/SpecialTypeCodec.h"

using namespace std;
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
    const int bytesOfOneLine = 10;
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

int main() {
    for (uint32_t i = 0; i < fourBytesCanHave + 1; i++) {
        std::string str = trans_int_to_binary(i);
        const char* p = str.data();
        uint32_t ret = retrieve_int_from_binary(p);
        if (ret != i) {
            std::cout << "raw number: " << i << std::endl;
            std::cout << "binary string: ";
            showHex(str);
            std::cout << "number: " << ret << std::endl;
            std::cout << "offset: " << p - str.data() << std::endl;
            exit(1);
        }

    }
    for (uint32_t i = UINT32_MAX - 100000; i < UINT32_MAX; i++) {
        std::string str = trans_int_to_binary(i);
        const char* p = str.data();
        uint32_t ret = retrieve_int_from_binary(p);
        if (ret != i) {
            std::cout << "raw number: " << i << std::endl;
            std::cout << "binary string: ";
            showHex(str);
            std::cout << "number: " << ret << std::endl;
            std::cout << "offset: " << p - str.data() << std::endl;
            exit(1);
        }

    }
}
