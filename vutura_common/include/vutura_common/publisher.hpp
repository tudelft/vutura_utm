#pragma once

#include <nng/nng.h>

#include "event_source.hpp"

class Publisher {
public:
        Publisher(const std::string &url);

        void publish(std::string const& message);

        static void fatal(std::string const& function, const int error)
        {
            std::cerr << function << ": " << nng_strerror(error) << std::endl;
        }

private:
        nng_socket _sock;
};

