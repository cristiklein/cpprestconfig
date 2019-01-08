// Copyright 2019 Cristian Klein
#ifndef CPPRESTCONFIG_CPPRESTCONFIG_HH
#define CPPRESTCONFIG_CPPRESTCONFIG_HH

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

} 

#endif  // CPPRESTCONFIG_CPPRESTCONFIG_HH
