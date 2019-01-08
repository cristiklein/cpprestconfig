// Copyright 2019 Cristian Klein
#include "cpprestconfig/cpprestconfig.h"

#include <stdio.h>

#include "gtest/gtest.h"

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

TEST(CppRestConfigTest, StartServerDoesNotFail) {
    cpprestconfig::start_server();
}
