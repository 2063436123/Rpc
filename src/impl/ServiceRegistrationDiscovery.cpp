//
// Created by Hello Peter on 2021/11/11.
//
#include "../include/ServiceRegistrationDiscovery.h"

ServiceRegistrationDiscovery AllServices::rd_("localhost:2181", AllServices::watcher);
std::unordered_map<std::string, std::vector<std::string>> AllServices::services_;
