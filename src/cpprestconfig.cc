// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <map>
#include <memory>

#include <boost/any.hpp>
#include "cpprest/http_listener.h"
#include "cpprest/json.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace cpprestconfig {

using std::to_string;

using namespace web;  // NOLINT
using namespace web::http;  // NOLINT
using namespace web::http::experimental::listener;  // NOLINT

bool started = false;  // config(...) is mostly called in static initilization
                       // context, don't do anything funny

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

std::shared_ptr<spdlog::logger> logger() {
    static auto logger = std::make_shared<spdlog::logger>(
        "config",
        std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
    return logger;
}

struct ConfigProperty {
    std::string key, short_desc, long_desc;
    boost::any value, default_value;
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
        return boost::any_cast<bool>(value) ? "true" : "false";
    } else if (value.type() == typeid(int)) {
        return std::to_string(boost::any_cast<int>(value));
    } else if (value.type() == typeid(std::string)) {
        return boost::any_cast<std::string>(value);
    }

    throw std::runtime_error(fmt::format("Unknown boost::any type: {0}",
            value.type().name()));
}

json::value to_json_value(const boost::any &value) {
    if (value.type() == typeid(void)) {
        return json::value::null();
    } else if (value.type() == typeid(bool)) {
        return json::value::boolean(boost::any_cast<bool>(value));
    } else if (value.type() == typeid(int)) {
        return json::value::number(boost::any_cast<int>(value));
    } else if (value.type() == typeid(std::string)) {
        return json::value::string(boost::any_cast<std::string>(value));
    }

    throw std::runtime_error(fmt::format("Unknown boost::any type: {0}",
            value.type().name()));
}

json::value to_json_value(const std::type_info &type) {
    if (type == typeid(void)) {
        return json::value::string("null");
    } else if (type == typeid(bool)) {
        return json::value::string("boolean");
    } else if (type == typeid(int)) {
        return json::value::string("integer");
    } else if (type == typeid(std::string)) {
        return json::value::string("string");
    }

    throw std::runtime_error(fmt::format("Unknown type: {0}", type.name()));
}

template<>
bool &config<bool>(
    bool default_value,
    const char *key,
    const char *short_desc,
    const char *long_desc
) {
    logger()->info("{}={}", key, default_value);

    ConfigProperty &cp = config_properties()[key];
    cp.key = key;
    cp.short_desc = short_desc;
    cp.long_desc = long_desc;
    cp.value = default_value;
    cp.default_value = default_value;

    return boost::any_cast<bool&>(cp.value);
}

void handle_get(http_request request) {
    auto body = json::value::object();

    for (auto const & p : config_properties()) {
        auto o = json::value::object();
        o["short_desc"] = json::value::string(p.second.short_desc);
        o["long_desc"] = json::value::string(p.second.long_desc);
        o["default_value"] = to_json_value(p.second.default_value);
        o["value"] = to_json_value(p.second.value);
        o["type"] = to_json_value(p.second.value.type());

        body[p.first] = o;
    }

    request.reply(status_codes::OK, body);
}

std::unique_ptr<http_listener> g_listener;

void start_server(
    int port,
    const char *basepath
) {
    for (const auto &p : config_properties()) {
        logger()->info("{}={}", p.first, to_string(p.second.value));
    }

    auto uri = uri_builder()
        .set_scheme("http")
        .set_host("localhost")
        .set_port(port)
        .set_path(basepath)
        .to_uri();

    // should close previous listener
    g_listener = make_unique<http_listener>(uri);
    g_listener->support(methods::GET, handle_get);

    try {
        (*g_listener).open().wait();
        logger()->info("listening on {}", g_listener->uri().to_string());
    } catch (std::exception const &e) {
        logger()->warn("Exception {}", e.what());
    }
}

void stop_server() {
    g_listener.release();
}

}  // namespace cpprestconfig
