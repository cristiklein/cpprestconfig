// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <stdio.h>

#include "gtest/gtest.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

// cpprestconfig is specifically designed to allow usage in static
// initilization context
const bool &show_fps = cpprestconfig::config(
    false,
    "main.show_fps",
    "Show the number of frames per second",
    "Enable this option to display the currently measured number "
    "of frames per second on the output video");

const bool &show_cool_stuff = cpprestconfig::config(
    true,
    "main.show_cool_stuff",
    "Show something cool",
    "Enable this option to display something cool in the video output");

TEST(CppRestConfigTest, StaticInitializationIsSetToDefault) {
    EXPECT_FALSE(show_fps);
    EXPECT_TRUE(show_cool_stuff);
}

TEST(CppRestConfigTest, NormalInitializationIsSetToDefault) {
    // of course, you can also use it in "normal" initialization context :)
    const bool &show_lorem = cpprestconfig::config(
        false,
        "main.show_lorem",
        "Show a lorem ipsum message",
        "This option is really useless, but you can enable it anyway for fun");

    EXPECT_FALSE(show_lorem);
}

TEST(CppRestConfigTest, ListAllConfigurationValues) {
    using namespace web;  // NOLINT
    using namespace web::http;  // NOLINT
    using namespace web::http::client;  // NOLINT
    using utility::conversions::to_string_t;

    cpprestconfig::start_server(8088);

    http_client client(U("http://127.0.0.1:8088/api/config"));
    auto response = client.request(methods::GET).get();
    EXPECT_EQ(response.status_code(), status_codes::OK);

    auto body = response.extract_json().get();

    EXPECT_EQ(body["main.show_lorem"]["type"].as_string(), "boolean");
    EXPECT_EQ(body["main.show_lorem"]["default_value"].as_bool(), false);

    cpprestconfig::stop_server();
}

TEST(CppRestConfigTest, ChangeConfigurationValue) {
    using namespace web;  // NOLINT
    using namespace web::http;  // NOLINT
    using namespace web::http::client;  // NOLINT
    using utility::conversions::to_string_t;

    // of course, you can also use it in "normal" initialization context :)
    const bool &show_lorem_ipsum = cpprestconfig::config(
        false,
        "main.show_lorem_ipsum",
        "Show a lorem ipsum message",
        "This option is really useless, but you can enable it anyway for fun");

    EXPECT_FALSE(show_lorem_ipsum);

    cpprestconfig::start_server(8088);

    http_client client(U("http://127.0.0.1:8088/api/config"));
    auto response = client.request(
        methods::PUT,
        "main.show_lorem_ipsum",
        "true").get();
    EXPECT_TRUE(show_lorem_ipsum);
    EXPECT_EQ(response.status_code(), status_codes::OK);

    response = client.request(
        methods::PUT,
        "main.show_lorem_ipsum",
        "false").get();
    EXPECT_FALSE(show_lorem_ipsum);
    EXPECT_EQ(response.status_code(), status_codes::OK);

    cpprestconfig::stop_server();
}

TEST(CppRestConfigTest, InvalidKey) {
    using namespace web;  // NOLINT
    using namespace web::http;  // NOLINT
    using namespace web::http::client;  // NOLINT
    using utility::conversions::to_string_t;

    cpprestconfig::start_server(8088);

    http_client client(U("http://127.0.0.1:8088/api/config"));
    auto response = client.request(
        methods::PUT,
        "key_does_not_exist",
        "true").get();
    EXPECT_EQ(response.status_code(), status_codes::NotFound);
}

TEST(CppRestConfigTest, InvalidValue) {
    using namespace web;  // NOLINT
    using namespace web::http;  // NOLINT
    using namespace web::http::client;  // NOLINT
    using utility::conversions::to_string_t;

    cpprestconfig::start_server(8088);

    http_client client(U("http://127.0.0.1:8088/api/config"));
    auto response = client.request(
        methods::PUT,
        "main.show_fps",
        "weird_value").get();
    EXPECT_EQ(response.status_code(), status_codes::BadRequest);
}
