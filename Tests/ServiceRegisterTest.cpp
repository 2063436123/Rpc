//
// Created by Hello Peter on 2021/11/10.
//
#include "../src/include/ServiceRegistrationDiscovery.h"
#include <unistd.h>

using namespace std;

int main() {
    ServiceRegistrationDiscovery rd;
    rd.add_new_service("hello");
    rd.register_service("hello", "127.0.0.1:12345");
    sleep(120);
}