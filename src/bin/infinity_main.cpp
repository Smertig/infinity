//
// Created by JinHai on 2022/7/18.
//

#include <iostream>
#include "network/db_server.h"
#include "cxxopts.hpp"

void ParseArguments(int argc, char** argv, infinity::StartupParameter& parameters) {
    cxxopts::Options options("./infinity_server", "");

    options.add_options()
            ("h,help", "Display this help and exit") // NOLINT
            ("addr,address", "Specify the listening address. Default value: 0.0.0.0", cxxopts::value<std::string>()->default_value("0.0.0.0")) // NOLINT
            ("p,port", "Specify the listening port. 0 means randomly select an available one. Default value: 5432", cxxopts::value<uint16_t>()->default_value("5432")) // NOLINT
            ;

    cxxopts::ParseResult result = options.parse(argc, argv);

    if(result.count("help")) {
        std::cout << options.help() << std::endl;
        return ;
    }

    boost::system::error_code error;
    parameters.address = boost::asio::ip::make_address(result["address"].as<std::string>(), error);

    Assert(!error, "Not a valid IPv4 address: " + result["address"].as<std::string>() + ", panic!");

    parameters.port = result["port"].as<uint16_t>();
}

int main(int argc, char** argv) {
    infinity::StartupParameter parameters;
    ParseArguments(argc, argv, parameters);

    infinity::DBServer db_server(parameters);
    db_server.Run();
    return 0;
}
