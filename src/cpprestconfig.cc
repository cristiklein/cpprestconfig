// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <map>
#include <memory>

#include <boost/filesystem.hpp>
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

namespace fs = boost::filesystem;

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

template<typename T>
struct ConfigTypeProperty {
    T value, default_value;
    callback<T> _callback;
    struct limits<T> _limits;
};

std::string to_string(bool b) {
    return b ? "true" : "false";
}

template<typename T>
std::string to_string(const ConfigTypeProperty<T> &cpt) {
    return to_string(cpt.value);
}

template<typename T>
json::value to_json_value(const ConfigTypeProperty<T> &cpt) {
    return json::value(cpt.value);
}

template<typename T>
json::value to_json_value_from_default(const ConfigTypeProperty<T> &cpt) {
    return json::value(cpt.default_value);
}

json::value to_json_limits(const struct limits<bool> &l) {
    return json::value();  // null JSON, bool has no limits
}

json::value to_json_limits(const struct limits<int> &l) {
    auto o = json::value::object();
    o["min"] = json::value(l.min);
    o["max"] = json::value(l.max);
    o["step"] = json::value(l.step);
    return o;
}

bool apply_limits(bool value, const struct limits<bool> &l) {
    return value;  // i.e., nothing
}

int apply_limits(int value, const struct limits<int> &l) {
    if (l.step == 0)  // i.e., no constraints
        return value;
    if (value < l.min)
        return l.min;
    if (value > l.max)
        return l.max;
    if (l.step != 1)
        value = (value - l.min) / l.step * l.step + l.min;
    return value;
}

template<typename T>
void assign_from_string(
    ConfigTypeProperty<T> *cpt,
    const std::string &key,
    const std::string &s
) {
    cpt->value = apply_limits(boost::lexical_cast<T>(s), cpt->_limits);
    if (cpt->_callback) {
        cpt->_callback(key.c_str(), cpt->value);
    }
}

enum ConfigType {
    BOOL,
    INT,
};

std::string to_string(ConfigType c) {
    switch (c) {
        case BOOL:
            return "boolean";
        case INT:
            return "integer";
        default:
            throw std::runtime_error("Unknown config type");
    }
}

struct ConfigProperty {
    std::string key, short_desc, long_desc;
    ConfigType type;
    ConfigTypeProperty<bool> bool_property;
    ConfigTypeProperty<int> int_property;
    Options options;
};

std::string to_string(const ConfigProperty &cp) {
    switch (cp.type) {
        case BOOL:
            return to_string(cp.bool_property);
        case INT:
            return to_string(cp.int_property);
        default:
            throw std::runtime_error("Unknown config type");
    }
}

json::value to_json_limits(const ConfigProperty &cp) {
    switch (cp.type) {
        case BOOL:
            return to_json_limits(cp.bool_property._limits);
        case INT:
            return to_json_limits(cp.int_property._limits);
        default:
            throw std::runtime_error("Unknown config type");
    }
}

json::value to_json_value(const ConfigProperty &cp) {
    switch (cp.type) {
        case BOOL:
            return to_json_value(cp.bool_property);
        case INT:
            return to_json_value(cp.int_property);
        default:
            throw std::runtime_error("Unknown config type");
    }
}

json::value to_json_value_from_default(const ConfigProperty &cp) {
    switch (cp.type) {
        case BOOL:
            return to_json_value_from_default(cp.bool_property);
        case INT:
            return to_json_value_from_default(cp.int_property);
        default:
            throw std::runtime_error("Unknown config type");
    }
}

void assign_from_string(
    ConfigProperty *cp,
    const std::string &key,
    const std::string &s
) {
    switch (cp->type) {
        case BOOL:
            assign_from_string(&cp->bool_property, key, s);
            break;
        case INT:
            assign_from_string(&cp->int_property, key, s);
            break;
        default:
            throw std::runtime_error("Unknown config type");
    }
}


typedef std::map<std::string, ConfigProperty> ConfigProperties;

static ConfigProperties& config_properties() {
    static ConfigProperties cp;
    return cp;
}

void loadPersist(ConfigProperty *cp);
void savePersist(ConfigProperty *cp);

template<>
bool& config(
    bool default_value,
    const char *key,
    const char *short_desc,
    const char *long_desc,
    callback<bool> _callback,
    limits<bool> _limits,
    Options options
) {
    logger()->info("{}={}", key, default_value);

    ConfigProperty &cp = config_properties()[key];
    cp.key = key;
    cp.short_desc = short_desc;
    cp.long_desc = long_desc;
    cp.options = options;

    cp.type = BOOL;
    cp.bool_property.value = default_value;
    cp.bool_property.default_value = default_value;
    cp.bool_property._callback = _callback;
    cp.bool_property._limits = _limits;

    loadPersist(&cp);

    return cp.bool_property.value;
}

template<>
int& config<int>(
    int default_value,
    const char *key,
    const char *short_desc,
    const char *long_desc,
    callback<int> _callback,
    limits<int> _limits,
    Options options
) {
    logger()->info("{}={}", key, default_value);

    ConfigProperty &cp = config_properties()[key];
    cp.key = key;
    cp.short_desc = short_desc;
    cp.long_desc = long_desc;
    cp.options = options;

    cp.type = INT;
    cp.int_property.value = default_value;
    cp.int_property.default_value = default_value;
    cp.int_property._callback = _callback;
    cp.int_property._limits = _limits;

    loadPersist(&cp);

    return cp.int_property.value;
}

void handle_get(http_request request) {
    auto body = json::value::object();

    for (auto const & p : config_properties()) {
        auto o = json::value::object();
        auto &cp = p.second;
        o["short_desc"] = json::value::string(cp.short_desc);
        o["long_desc"] = json::value::string(cp.long_desc);
        o["default_value"] = to_json_value_from_default(cp);
        o["value"] = to_json_value(cp);
        o["type"] = json::value::string(to_string(cp.type));

        auto json_limits = to_json_limits(cp);
        if (!json_limits.is_null()) {
            o["limits"] = json_limits;
        }

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
        assign_from_string(cp, key, new_value);
        savePersist(cp);

        logger()->info("{}={}", key, to_string(*cp));

        request.reply(status_codes::OK);
    } catch (const std::out_of_range &ex) {
        request.reply(status_codes::NotFound,
            fmt::format("Key {} not found", key));
    } catch (const boost::bad_lexical_cast &ex) {
        request.reply(status_codes::BadRequest,
            fmt::format("Cannot convert '{}' to {}",
                new_value,
                to_string(cp->type)));
    } catch (const std::exception &ex) {
        request.reply(status_codes::InternalError, ex.what());
    }
}

std::unique_ptr<http_listener> g_listener;
char *g_persistDir = NULL;

void loadPersist(ConfigProperty *cp) {
    if (!g_persistDir)
        return;
    if ((cp->options & Options::NoPersist))
        return;

    auto persistFile = (fs::path(g_persistDir) / cp->key).native();

    std::ifstream ifs(persistFile);
    if (!ifs.is_open()) {
        logger()->info("Did not load {}; {} not found", cp->key, persistFile);
        return;
    }

    try {
        std::string value((std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>());

        assign_from_string(cp, cp->key, value);
        logger()->info("Loaded {} from {}", cp->key, persistFile);
    } catch (const boost::bad_lexical_cast &ex) {
        logger()->info("Did not loaded {} from {}; {}",
            cp->key, persistFile, ex.what());
    }
}

void savePersist(ConfigProperty *cp) {
    if (!g_persistDir)
        return;
    if ((cp->options & Options::NoPersist))
        return;

    auto persistFile = (fs::path(g_persistDir) / cp->key).native();

    std::ofstream ofs(persistFile);
    if (!ofs.is_open()) {
        logger()->info("saving {} (failed)", persistFile);
        return;
    }

    logger()->info("saving {}", persistFile);

    std::string value = to_string(*cp);
    ofs.write(value.c_str(), value.size());
}

void start_server(
    int port,
    const char *basepath,
    const char *persistDir
) {
    if (g_persistDir) {
        free(g_persistDir);
    }
    g_persistDir = NULL;
    if (persistDir) {
        g_persistDir = strdup(persistDir);
        fs::create_directories(g_persistDir);
    }

    logger()->info("current configuration is:");
    for (auto &p : config_properties()) {
        loadPersist(&p.second);
        logger()->info("{}={}", p.first, to_string(p.second));
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
