#pragma once

#include <string>
#include <vector>
#include "geo.h"

namespace domain {

    struct Stop {
        std::string stop_name;
        geo::Coordinates position;
    };

    struct Bus {
        std::string bus_name;
        std::vector<const Stop*> bus_stops;
        bool is_circular;
    };

    struct BusInfo {
        size_t stops_count = 0;
        size_t unique_stops_count = 0;
        unsigned int route_length = 0;
        double curvature = 0.0;
        bool bus_found = false;
    };

} // namespace domain