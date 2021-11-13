//
// Created by Hello Peter on 2021/11/11.
//

#ifndef TESTLINUX_SERVICEREGISTERDISCOVERY_H
#define TESTLINUX_SERVICEREGISTERDISCOVERY_H

#include <iostream>
#include <string>
#include <vector>

#define THREADED

#include <zookeeper/zookeeper.h>
#include <cstring>
#include <functional>

#define CHECK() do {if (rc) fprintf(stderr, "ZooKeeper Error %d for %s:%d\n", rc, __FILE__, __LINE__);} while(0)

class ServiceRegistrationDiscovery {
public:
    explicit ServiceRegistrationDiscovery(const std::string &host = "localhost:2181", watcher_fn watcher = nullptr) {
        zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);
        zh_ = zookeeper_init(host.data(), watcher, 10000, 0, 0, 0);
    }

    // 注册新的服务
    void add_new_service(const std::string &service_name) {
        static char buf[512];
        std::string path = "/services/" + service_name;
        int rc = zoo_create(zh_, path.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, buf, sizeof(buf) - 1);
        CHECK();
    }

    // 在已存在的服务节点下添加自己的ip_port
    void register_service(const std::string &service_name, const std::string &ip_port) {
        static char buf[512];
        std::string path = "/services/" + service_name + "/" + ip_port;
        int rc = zoo_create(zh_, path.c_str(), nullptr, -1, &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, buf, sizeof(buf) - 1);
        CHECK();
    }

    // 查找所有的服务名
    std::vector<std::string> find_services() {
        std::string path = "/services";

        String_vector res;
        int rc = zoo_get_children(zh_, path.c_str(), 1, &res);
        CHECK();
        std::vector<std::string> ret;
        for (int i = 0; i < res.count; i++) {
            ret.emplace_back(res.data[i]);
        }
        return ret;
    }

    // 查找哪些服务器支持这项服务
    std::vector<std::string> search_service_provider(const std::string &service_name) {
        std::string path = "/services/" + service_name;

        String_vector res;
        int rc = zoo_get_children(zh_, path.c_str(), 1, &res);
        CHECK();
        std::vector<std::string> ret;
        for (int i = 0; i < res.count; i++) {
            ret.emplace_back(res.data[i]);
        }
        return ret;
    }

private:
    zhandle_t *zh_;
};

#undef CHECK

// for service discovery
class AllServices {
public:
    static void init() {
        refresh();
    }

    // 返回一个会被实时更新的服务列表，not thread-safe.
    static const auto& services() {
        return services_;
    }
private:
    static void handleChange(const std::string& path) {
        // 1. 添加新服务 path = '/services'
        // 2. 添加新服务器 path = '/services/service_name'
        if (path == "/services") {
            auto services = rd_.find_services();
            for (const auto& service_name : services) {
                if (!services_.contains(service_name)) {
                    auto providers = rd_.search_service_provider(service_name);
                    services_[service_name] = providers;
                }
            }
        } else {
            auto service_name = path.substr(path.find_first_not_of("/services/"));
            std::cout << "parent service_name = " << service_name << std::endl;
            auto providers = rd_.search_service_provider(service_name);
            services_[service_name] = providers;
        }
    }

    static void refresh() {
        auto services = rd_.find_services();
        for (const auto& service_name : services) {
            auto providers = rd_.search_service_provider(service_name);
            services_[service_name] = providers;
        }
    }

    static void watcher(zhandle_t *zzh, int type, int state, const char *path,
                            void *watcherCtx) {
        using std::cout;
        using std::endl;
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
            if (strncmp(path, "/services", 9) == 0) {
                handleChange(path);
                cout << "service changed!" << endl;
            }
        } else {
            if (type == ZOO_DELETED_EVENT && strncmp(path, "/services", 9) == 0 && strlen(path) > 9)
                return; // 服务被删除, 少数情况
            cout << "unknown type: " << type << endl;
        }
    }

    static std::unordered_map<std::string, std::vector<std::string>> services_; // service_name -> ip_port list
    static ServiceRegistrationDiscovery rd_;
};

#endif //TESTLINUX_SERVICEREGISTERDISCOVERY_H


