//
// Created by Hello Peter on 2021/11/10.
//
#define THREADED

#include <unistd.h>
#include "../src/include/ServiceRegistrationDiscovery.h"

using namespace std;

int main() {
    AllServices::init();
    const auto& map = AllServices::services();
    for (auto& pair : map) {
        cout << "service: " << pair.first << " ";
        for (auto ipport : pair.second) {
            cout << ipport << ", ";
        }
        cout << endl;
    }
    sleep(120);
}