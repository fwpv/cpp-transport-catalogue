#pragma once

/*
 * Классы/структуры, которые являются частью предметной области (domain)
 * приложения и не зависят от транспортного справочника.
 */

#include "geo.h"

#include <string>
#include <vector>

namespace tcat {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
    int StopCount() const;
    int CountUniqueStops() const;
};

}  // namespace tcat