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
    EXPECT_EQ(show_fps, false);
    EXPECT_EQ(show_cool_stuff, true);
}

TEST(CppRestConfigTest, NormalInitializationIsSetToDefault) {
    // of course, you can also use it in "normal" initialization context :)
    const bool &show_lorem = cpprestconfig::config(
        false,
        "main.show_lorem",
        "Show a lorem ipsum message",
        "This option is really useless, but you can enable it anyway for fun");

    EXPECT_EQ(show_lorem, false);
}

TEST(CppRestConfigTest, ListAllConfigurationValues) {
    using namespace web;  // NOLINT
    using namespace web::http;  // NOLINT
    using namespace web::http::client;  // NOLINT
    using utility::conversions::to_string_t;

    cpprestconfig::start_server(8088);

    http_client client(U("http://localhost:8088/api/config"));
    client.request(methods::GET, to_string_t(""))
        .then([](http_response response) {
            if (response.status_code() == status_codes::OK) {
                return response.extract_json();
            }
            return pplx::task_from_result(json::value());
        })
        .then([](pplx::task<json::value> previousTask) {
            const char *body = previousTask.get().serialize().c_str();
            fprintf(stderr, "%s\n", body);
        })
        .wait();

    EXPECT_EQ(true, false);
}
