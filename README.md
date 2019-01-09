REST C++ Config
===============

[![Build Status](https://api.travis-ci.org/cristiklein/cpprestconfig.svg?branch=master)](https://travis-ci.org/cristiklein/cpprestconfig)

The aim of this project is to convert constants into runtime configuration variables, easily accessible via a REST interface.

What Does REST C++ Config Do?
-----------------------------
The following sample should clarify intended usage:

        #include "cpprestconfig/cpprestconfig.h"

        #include <stdio.h>
        #include <unistd.h>

        // cpprestconfig is specifically designed to allow usage in static
        // initilization context
        const bool &print_red = cpprestconfig::config(
            true,
            "main.print_red",
            "Print 'red' every second",
            "Totally useless demo, that prints 'red' every second");

        int main(int argc, char **argv) {
            const bool &print_green = cpprestconfig::config(
                false,
                "main.print_green",
                "Print 'green' every second",
                "Totally useless demo, that prints 'green' every second");

            cpprestconfig::start_server(8089);

            printf(
                "Usage:\n"
                "* List all configuration variables:\n"
                "  curl http://localhost:8089/api/config | jq .\n"
                "* Print green every second:\n"
                "  curl -XPUT http://localhost:8089/api/config/main.print_green -d true\n"
                "* Do not print red every second:\n"
                "  curl -XPUT http://localhost:8089/api/config/main.print_red -d false\n"
            );

            while (true) {
                if (print_red) {
                    printf("red %s\n", print_red ? "true" : "false");
                }
                if (print_green) {
                    printf("green %s\n", print_green ? "true" : "false");
                }
                sleep(1);
            }
        }

Requirements
------------
* [Boost](https://www.boost.org/) 1.54 or newer
* [cmake](https://cmake.org/) 2.8 or newer
* [cpplint](https://github.com/cpplint/cpplint)

Note: [cpprest](https://github.com/Microsoft/cpprestsdk) and [googletest](https://github.com/google/googletest) are vendored in as git submodules.

Compiling
---------

        git clone --recursive https://github.com/cristiklein/cpprestconfig.git
        mkdir mybuild
        cd mybuild
        cmake ..
        make

Usage
-----
We suggest vendoring `cpprestconfig` as a git module. Then your superproject's `CMakeLists.txt` might look like this:

        ...
        add_subdirectory(3rdparty/cpprestconfig EXCLUDE_FROM_ALL)
        ...
        target_link_libraries(my_executable
            cpprestconfig)

