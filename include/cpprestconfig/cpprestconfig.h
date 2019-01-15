// Copyright 2019 Cristian Klein
#ifndef INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_
#define INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_

#include <functional>

namespace cpprestconfig {

template<typename T>
struct limits;

template<>
struct limits<bool> {
    // reserved
};

template<>
struct limits<int> {
    int min;
    int max;
    int step;
};

template<typename T>
using callback = std::function<void(const char *key, T value)>;

template<typename T>
T &config(
    T default_value,
    const char *key,
    const char *short_desc,
    const char *long_desc,
    callback<T> callback = {},
    limits<T> limits = {});

void start_server(
    int port = 8080,
    const char *baseurl = "/api/config");

void stop_server();

}  // namespace cpprestconfig

#endif  // INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_
