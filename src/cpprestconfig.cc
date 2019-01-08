// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <map>

#include <boost/any.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace cpprestconfig {

using std::to_string;

bool started = false;  // config(...) is mostly called in static initilization
                       // context, don't do anything funny

std::shared_ptr<spdlog::logger> logger() {
    static auto logger = std::make_shared<spdlog::logger>(
        "config",
        std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    return logger;
}

struct ConfigProperty {
    std::string api_key, short_desc, long_desc;
    boost::any value;
};
typedef std::map<std::string, ConfigProperty> ConfigProperties;

static ConfigProperties& config_properties() {
    static ConfigProperties cp;
    return cp;
}

std::string to_string(const boost::any &value) {
    if (value.type() == typeid(void)) {
        return "void";
    } else if (value.type() == typeid(bool)) {
        return std::to_string(boost::any_cast<bool>(value));
    } else if (value.type() == typeid(int)) {
        return std::to_string(boost::any_cast<int>(value));
    } else if (value.type() == typeid(std::string)) {
        return boost::any_cast<std::string>(value);
    }

    throw std::runtime_error(fmt::format("Unknown boost::any type: {0}",
            value.type().name()));
}

template<>
bool &config<bool>(
    bool default_value,
    const char *api_key,
    const char *short_desc,
    const char *long_desc
) {
    logger()->info("{}={}", api_key, default_value);

    ConfigProperty &cp = config_properties()[api_key];
    cp.api_key = api_key;
    cp.short_desc = short_desc;
    cp.long_desc = long_desc;
    cp.value = default_value;

    return boost::any_cast<bool&>(cp.value);
}

void start_server(
    int port,
    const char *baseurl
) {
    for (const auto &p : config_properties()) {
        logger()->info("{}={}", p.first, to_string(p.second.value));
    }

    logger()->info("started, listening on 127.0.0.1:{}/{} (not yet implemented)",
        port, baseurl);
}

}  // namespace cpprestconfig
