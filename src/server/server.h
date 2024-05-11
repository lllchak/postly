#pragma once

#include "config.pb.h"

#include <drogon/drogon.h>

#include <string>

class TServer {
public:
    explicit TServer(const std::string& configPath);

    int Run(const std::uint16_t port);

private:
    postly::TServerConfig Config;
};
