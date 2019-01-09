// Copyright 2019 Cristian Klein
#ifndef INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_
#define INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_

namespace cpprestconfig {

template<typename T>
T &config(
    T default_value,
    const char *api_key,
    const char *short_desc,
    const char *long_desc);

void start_server(
    int port = 8080,
    const char *baseurl = "/api/config");

void stop_server();

}  // namespace cpprestconfig

#endif  // INCLUDE_CPPRESTCONFIG_CPPRESTCONFIG_H_
