//
// Created by Hello Peter on 2021/11/10.
//
#define THREADED

#include <zookeeper/zookeeper.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

using namespace std;
#define CHECK() do {if (rc) fprintf(stderr, "Error %d for %d\n", rc, __LINE__);} while(0)

void watcher(zhandle_t *zzh, int type, int state, const char *path,
             void *watcherCtx) {
    cout << "in watcher: ";
    if (type == ZOO_SESSION_EVENT) {
        // state refers to states of zookeeper connection.
        // To keep it simple, we would demonstrate these 3: ZOO_EXPIRED_SESSION_STATE, ZOO_CONNECTED_STATE, ZOO_NOTCONNECTED_STATE
        if (state == ZOO_CONNECTED_STATE) {
            cout << "connected!" << endl;
        } else if (state == ZOO_NOTCONNECTED_STATE) {
            cout << "unconnected!" << endl;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            cout << "session expired!" << endl;
            zookeeper_close(zzh);
        }
    } else if (type == ZOO_CHILD_EVENT) {
        cout << path << endl;
        if (strcmp(path, "/services") == 0) {
            cout << "service changed!" << endl;
        } else {
            cout << "server in " << path << " changed!" << endl;
        }
    }
}

zhandle_t *zh;

void init() {
    zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
    zh = zookeeper_init("localhost:2181", watcher, 10000, 0, 0, 0);
}

vector<string> find_services() {
    std::string path = "/services";

    String_vector res;
    int rc = zoo_get_children(zh, path.c_str(), 1, &res);
    CHECK();
    vector<string> ret;
    for (int i = 0; i < res.count; i++) {
        ret.emplace_back(res.data[i]);
    }
    return ret;
}

vector<string> search_service_provider(const std::string &service_name) {
    std::string path = "/services/" + service_name;

    String_vector res;
    int rc = zoo_get_children(zh, path.c_str(), 1, &res);
    CHECK();
    vector<string> ret;
    for (int i = 0; i < res.count; i++) {
        ret.emplace_back(res.data[i]);
    }
    return ret;
}

int main() {
    init();
    auto services = find_services();
    for (const auto &str: services)
        cout << str << " ";
    cout << endl;

    auto servers = search_service_provider("hello");
    for (const auto &str: servers)
        cout << str << " ";
    cout << endl;
    sleep(120);
}