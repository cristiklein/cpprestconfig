// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <map>
#include <memory>

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include "cpprest/http_listener.h"
#include "cpprest/json.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace boost {
    template<>
    bool lexical_cast<bool, std::string>(const std::string& arg) {
        if (arg == "t" || arg == "1" || arg == "true")
            return true;
        if (arg == "f" || arg == "0" || arg == "false")
            return false;
        throw boost::bad_lexical_cast();
    }
}  // namespace boost

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

std::string to_string(const std::type_info &type) {
    if (type == typeid(void)) {
        return "null";
    } else if (type == typeid(bool)) {
        return "boolean";
    } else if (type == typeid(int)) {
        return "integer";
    } else if (type == typeid(std::string)) {
        return "string";
    }

    throw std::runtime_error(fmt::format("Unknown type: {0}", type.name()));
}

json::value to_json_value(const std::type_info &type) {
    return json::value::string(to_string(type));
}

template<typename T>
T& config(
    T default_value,
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

    return boost::any_cast<T&>(cp.value);
}

// explicit instantiation
template bool& config(bool, const char *, const char *, const char *);
template int& config(int, const char *, const char *, const char *);

template<typename T>
void _assign_from_string(boost::any *value, const std::string &s) {
    boost::any_cast<T&>(*value) = boost::lexical_cast<T>(s);
}

/** Assign a value to any *without* changing its type or
 * triggering a reallocation. */
void assign_from_string(boost::any *value, const std::string &s) {
    auto const &type = value->type();
    if (type == typeid(bool)) {
        _assign_from_string<bool>(value, s);
    } else if (type == typeid(int)) {
        _assign_from_string<int>(value, s);
    } else if (type == typeid(std::string)) {
        _assign_from_string<std::string>(value, s);
    } else {
        throw std::runtime_error(fmt::format("Unknown boost::any type: {0}",
            type.name()));
    }
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

void handle_put(http_request request) {
    auto const &path = uri::split_path(request.request_uri().path());

    if (path.empty()) {
        request.reply(
            status_codes::BadRequest,
            fmt::format("Got empty path in request"));
        return;
    }

    const std::string &key = path.back();
    const std::string new_value = request.extract_string().get();
    ConfigProperty *cp = NULL;

    try {
        cp = &config_properties().at(key);

        assign_from_string(&cp->value, new_value);
        logger()->info("{}={}", key, to_string(cp->value));

        request.reply(status_codes::OK);
    } catch (const std::out_of_range &ex) {
        request.reply(status_codes::NotFound,
            fmt::format("Key {} not found", key));
    } catch (const boost::bad_lexical_cast &ex) {
        request.reply(status_codes::BadRequest,
            fmt::format("Cannot convert '{}' to {}",
                new_value,
                cp ? to_string(cp->value.type()) : "unknown"));
    } catch (const std::exception &ex) {
        request.reply(status_codes::InternalError, ex.what());
    }
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
    g_listener->support(methods::PUT, handle_put);

    try {
        (*g_listener).open().wait();
        logger()->info("listening on {}", g_listener->uri().to_string());
    } catch (std::exception const &e) {
        logger()->warn("Exception {}", e.what());
    }
}

void stop_server() {
    logger()->info("stopped");
    g_listener.reset();
}

}  // namespace cpprestconfig
