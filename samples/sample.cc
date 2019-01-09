// Copyright 2019 Cristian Klein
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
