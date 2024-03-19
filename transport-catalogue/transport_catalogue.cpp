#include "transport_catalogue.h"

#include <cassert>
#include <cmath>
#include <iomanip>
#include <sstream>

using namespace std::literals; 

namespace tcat {

void TransportCatalogue::AddStop(std::string_view name, const geo::Coordinates& coordinates) {
    stops_.push_back({std::string(name), coordinates});
    Stop& placed_stop = stops_.back();
    stopname_to_stop_.emplace(placed_stop.name, &placed_stop);
}

void TransportCatalogue::AddBus(std::string_view name,
        const std::vector<std::string_view>& stops, bool is_roundtrip) {
    buses_.push_back({std::string(name), {}, is_roundtrip});
    Bus& placed_bus = buses_.back();
    busname_to_bus_.emplace(placed_bus.name, &placed_bus);
    for (std::string_view stop_name : stops) {
        const Stop* stop = FindStop(stop_name);
        placed_bus.stops.push_back(stop);
        buses_of_stop_[stop].insert(placed_bus.name);
    }
}

void TransportCatalogue::AddDistance(std::string_view start, const std::string_view& end, int distance) {
    const Stop* stop1 = FindStop(start);
    const Stop* stop2 = FindStop(end);
    if (stop1 && stop2) {
        distances_.emplace(std::pair<const Stop*,const Stop*>{stop1, stop2}, distance);
    }
}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (stopname_to_stop_.count(name)) {
        return stopname_to_stop_.at(name);
    } else {
        return nullptr;
    }
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (busname_to_bus_.count(name)) {
        return busname_to_bus_.at(name);
    } else {
        return nullptr;
    }
}

int TransportCatalogue::GetDistance(const Stop* start_stop, const Stop* end_stop) const {
    std::pair<const Stop*, const Stop*> ps {start_stop, end_stop};
    if (distances_.count(ps)) {
        return distances_.at(ps);
    }
    ps = {end_stop, start_stop};
    if (distances_.count(ps)) {
        return distances_.at(ps);
    }
    return 0;
}

std::vector<const Bus*> TransportCatalogue::GetAllBuses() const {
    std::vector<const Bus*> container;
    container.reserve(buses_.size());
    for (const Bus& bus : buses_) {
        container.push_back(&bus);
    }
    return container;
}

std::vector<const Stop*> TransportCatalogue::GetAllStops() const {
    std::vector<const Stop*> container;
    container.reserve(buses_.size());
    for (const Stop& stop : stops_) {
        container.push_back(&stop);
    }
    return container;
}

size_t TransportCatalogue::GetStopCount() const {
    return stops_.size();
}

double TransportCatalogue::CalculateGeoRouteLength(const Bus* bus) const {
    if (bus->stops.size() < 2) {
        return 0.0;
    }
    const Stop* from = bus->stops[0];
    double total = 0;
    for (auto it = bus->stops.begin() + 1; it < bus->stops.end(); ++it) {
        const Stop* to = *it;
        total += ComputeDistance(from->coordinates, to->coordinates);
        if (!bus->is_roundtrip) {
            total += ComputeDistance(to->coordinates, from->coordinates);
        }
        from = to;
    }
    return total;
}

int TransportCatalogue::CalculateRouteLength(const Bus* bus) const {
    if (bus->stops.size() < 2) {
        return 0;
    }
    const Stop* from = bus->stops[0];
    int total = 0;
    for (auto it = bus->stops.begin() + 1; it < bus->stops.end(); ++it) {
        const Stop* to = *it;
        total += GetDistance(from, to);
        if (!bus->is_roundtrip) {
            total += GetDistance(to, from);
        }
        from = to;
    }
    return total;
}

const std::set<std::string_view>*
        TransportCatalogue::GetBusNamesByStop(const Stop* stop) const {
    if (buses_of_stop_.count(stop)) {
        return &buses_of_stop_.at(stop);
    } else {
        return nullptr;
    }
}

namespace tests {

void GettingBusInfo() {
    TransportCatalogue catalogue;

    catalogue.AddStop("Tolstopaltsevo"sv, {55.611087, 37.208290});
    catalogue.AddStop("Marushkino"sv, {55.595884, 37.209755});
    catalogue.AddStop("Rasskazovka"sv, {55.632761, 37.333324});
    catalogue.AddStop("Biryulyovo Zapadnoye"sv, {55.574371, 37.651700});
    catalogue.AddStop("Biryusinka"sv, {55.581065, 37.648390});
    catalogue.AddStop("Universam"sv, {55.587655, 37.645687});
    catalogue.AddStop("Biryulyovo Tovarnaya"sv, {55.592028, 37.653656});
    catalogue.AddStop("Biryulyovo Passazhirskaya"sv, {55.580999, 37.659164});
    catalogue.AddStop("Rossoshanskaya ulitsa"sv, {55.595579, 37.605757});
    catalogue.AddStop("Prazhskaya"sv, {55.611678, 37.603831});

    catalogue.AddDistance("Tolstopaltsevo"sv, "Marushkino"sv, 3900);
    catalogue.AddDistance("Marushkino"sv, "Rasskazovka"sv, 9900);
    catalogue.AddDistance("Marushkino"sv, "Marushkino"sv, 100);
    catalogue.AddDistance("Rasskazovka"sv, "Marushkino"sv, 9500);
    catalogue.AddDistance("Biryulyovo Zapadnoye"sv, "Rossoshanskaya ulitsa"sv, 7500);
    catalogue.AddDistance("Biryulyovo Zapadnoye"sv, "Biryusinka"sv, 1800);
    catalogue.AddDistance("Biryulyovo Zapadnoye"sv, "Universam"sv, 2400);
    catalogue.AddDistance("Biryusinka"sv, "Universam"sv, 750);
    catalogue.AddDistance("Universam"sv, "Rossoshanskaya ulitsa"sv, 5600);
    catalogue.AddDistance("Universam"sv, "Biryulyovo Tovarnaya"sv, 900);
    catalogue.AddDistance("Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv, 1300);
    catalogue.AddDistance("Biryulyovo Passazhirskaya"sv, "Biryulyovo Zapadnoye"sv, 1200);

    catalogue.AddBus("256"sv, {"Biryulyovo Zapadnoye"sv, "Biryusinka"sv,
        "Universam"sv, "Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv,
        "Biryulyovo Zapadnoye"sv}, true);

    catalogue.AddBus("750"sv, {"Tolstopaltsevo"sv, "Marushkino"sv,
        "Marushkino"sv, "Rasskazovka"sv}, false);

    const Bus* bus = catalogue.FindBus("256");
    int stops_on_route = bus->StopCount();
    int unique_stops = bus->CountUniqueStops();
    int route_length = catalogue.CalculateRouteLength(bus);
    double geo_route_length = catalogue.CalculateGeoRouteLength(bus);
    double curvature = route_length / geo_route_length;

    assert(bus != nullptr);
    assert(stops_on_route == 6);
    assert(unique_stops == 5);
    assert(route_length == 5950);
    assert(fabs(curvature - 1.36124) < 0.00001);
    
    bus = catalogue.FindBus("750");
    stops_on_route = bus->StopCount();
    unique_stops = bus->CountUniqueStops();
    route_length = catalogue.CalculateRouteLength(bus);
    geo_route_length = catalogue.CalculateGeoRouteLength(bus);
    curvature = route_length / geo_route_length;

    assert(bus != nullptr);
    assert(stops_on_route == 7);
    assert(unique_stops == 3);
    assert(route_length == 27400);
    assert(fabs(curvature - 1.30853) < 0.00001);

    bus = catalogue.FindBus("751");
    assert(bus == nullptr);
}

void GettingStopInfo() {
    TransportCatalogue catalogue;
    catalogue.AddStop("Tolstopaltsevo"sv, {55.611087, 37.208290});
    catalogue.AddStop("Marushkino"sv, {55.595884, 37.209755});
    catalogue.AddStop("Rasskazovka"sv, {55.632761, 37.333324});
    catalogue.AddStop("Biryulyovo Zapadnoye"sv, {55.574371, 37.651700});
    catalogue.AddStop("Biryusinka"sv, {55.581065, 37.648390});
    catalogue.AddStop("Universam"sv, {55.587655, 37.645687});
    catalogue.AddStop("Biryulyovo Tovarnaya"sv, {55.592028, 37.653656});
    catalogue.AddStop("Biryulyovo Passazhirskaya"sv, {55.580999, 37.659164});
    catalogue.AddStop("Rossoshanskaya ulitsa"sv, {55.595579, 37.605757});
    catalogue.AddStop("Prazhskaya"sv, {55.611678, 37.603831});

    catalogue.AddBus("256"sv, {"Biryulyovo Zapadnoye"sv, "Biryusinka"sv,
        "Universam"sv, "Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv,
        "Biryulyovo Zapadnoye"sv}, true);
    catalogue.AddBus("750"sv, {"Tolstopaltsevo"sv, "Marushkino"sv,
        "Rasskazovka"sv}, false);
     catalogue.AddBus("828"sv, {"Biryulyovo Zapadnoye"sv, "Universam"sv,
        "Rossoshanskaya ulitsa"sv, "Biryulyovo Zapadnoye"sv}, true);

    const Stop* stop = catalogue.FindStop("Samara"sv);
    assert(stop == nullptr);

    stop = catalogue.FindStop("Prazhskaya"sv);
    assert(stop != nullptr);
    const std::set<std::string_view>* bus_set_ptr = catalogue.GetBusNamesByStop(stop);
    assert(bus_set_ptr == nullptr);

    stop = catalogue.FindStop("Biryulyovo Zapadnoye"sv);
    assert(stop != nullptr);
    bus_set_ptr = catalogue.GetBusNamesByStop(stop);
    assert(bus_set_ptr != nullptr);
    assert(bus_set_ptr->size() == 2);
    auto item = bus_set_ptr->begin();
    assert(*item == "256"sv);
    assert(*(++item) == "828"sv);
}

}
}