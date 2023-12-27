#include "transport_catalogue.h"

#include <cassert>
#include <sstream>
#include <iomanip>

namespace tcat {

void TransportCatalogue::AddStop(std::string_view name, const geo::Coordinates& coordinates) {
    stops_.push_back({std::string(name), coordinates});
    Stop& placed_stop = stops_.back();
    stopname_to_stop_.emplace(placed_stop.name, &placed_stop);
}

void TransportCatalogue::AddBus(std::string_view name, const std::vector<std::string_view>& stops) {
    buses_.push_back({std::string(name), {}});
    Bus& placed_bus = buses_.back();
    busname_to_bus_.emplace(placed_bus.name, &placed_bus);
    for (std::string_view stop_name : stops) {
        Stop* stop = FindStop(stop_name);
        placed_bus.stops.push_back(stop);
        buses_of_stop_[stop].insert(placed_bus.name);
    }
}
	
std::string TransportCatalogue::BusInfo(std::string_view name) const {
    using namespace std::literals; 
    std::ostringstream output;
    output << "Bus "s << name << ": "s;
    Bus* bus = FindBus(name);
    if (!bus) {
        output << "not found"s;
    } else {
        size_t stops_on_route = bus->stops.size();
        size_t unique_stops = CountUniqueStops(bus);
        double route_length = CalculateRouteLength(bus);
        output << stops_on_route << " stops on route, "s
               << unique_stops << " unique stops, "s
               << route_length << " route length";
    }
    return output.str();
}

std::string TransportCatalogue::StopInfo(std::string_view name) const {
    using namespace std::literals; 
    std::ostringstream output;
    output << "Stop "s << name << ": "s;
    Stop* stop = FindStop(name);
    if (!stop) {
        output << "not found"s;
    } else {
        std::list<std::string_view> bus_list = GetBusList(stop);
        if (bus_list.empty()) {
            output << "no buses"s;
        } else {
            output << "buses"s;
            for (std::string_view name : bus_list) {
                output << " "s << name;
            }
        }
    }
    return output.str();
}

TransportCatalogue::Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (stopname_to_stop_.count(name)) {
        return stopname_to_stop_.at(name);
    } else {
        return nullptr;
    }
}

TransportCatalogue::Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (busname_to_bus_.count(name)) {
        return busname_to_bus_.at(name);
    } else {
        return nullptr;
    }
}

size_t TransportCatalogue::CountUniqueStops(const TransportCatalogue::Bus* const bus) const {
    const std::unordered_set<Stop*> us = {bus->stops.begin(), bus->stops.end()};
    return us.size();
}

double TransportCatalogue::CalculateRouteLength(const TransportCatalogue::Bus* const bus) const {
    if (bus->stops.size() < 2) {
        return 0.0;
    }
    const Stop* from = bus->stops[0];
    double total = 0;
    for (auto it = bus->stops.begin() + 1; it < bus->stops.end(); ++it) {
        const Stop* to = *it;
        total += ComputeDistance(from->coordinates, to->coordinates);
        from = to;
    }
    return total;
}

std::list<std::string_view> TransportCatalogue::GetBusList(TransportCatalogue::Stop* stop) const {
    if (buses_of_stop_.count(stop) == 0) {
        return {};
    }
    const std::set<std::string_view>& buses = buses_of_stop_.at(stop);
    return {buses.begin(), buses.end()};
}

namespace tests {

void GettingBusInfo() {
    using namespace std::literals; 
    TransportCatalogue catalogue;
    catalogue.AddStop("Tolstopaltsevo"sv, {55.611087, 37.208290});
    catalogue.AddStop("Marushkino"sv, {55.595884, 37.209755});
    catalogue.AddStop("Rasskazovka"sv, {55.632761, 37.333324});
    catalogue.AddStop("Biryulyovo Zapadnoye"sv, {55.574371, 37.651700});
    catalogue.AddStop("Biryusinka"sv, {55.581065, 37.648390});
    catalogue.AddStop("Universam"sv, {55.587655, 37.645687});
    catalogue.AddStop("Biryulyovo Tovarnaya"sv, {55.592028, 37.653656});
    catalogue.AddStop("Biryulyovo Passazhirskaya"sv, {55.580999, 37.659164});

    catalogue.AddBus("256"sv, {"Biryulyovo Zapadnoye"sv, "Biryusinka"sv,
        "Universam"sv, "Biryulyovo Tovarnaya"sv, "Biryulyovo Passazhirskaya"sv,
        "Biryulyovo Zapadnoye"sv});

    catalogue.AddBus("750"sv, {"Tolstopaltsevo"sv, "Marushkino"sv,
        "Rasskazovka"sv, "Marushkino"sv, "Tolstopaltsevo"sv});

    std::string info1 = catalogue.BusInfo("256"sv);
    assert(info1 == "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length"s);

    std::string info2 = catalogue.BusInfo("750"sv);
    assert(info2 == "Bus 750: 5 stops on route, 3 unique stops, 20939.5 route length"s);

    std::string info3 = catalogue.BusInfo("751"sv);
    assert(info3 == "Bus 751: not found"s);
}

void GettingStopInfo() {
    using namespace std::literals; 
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
        "Biryulyovo Zapadnoye"sv});
    catalogue.AddBus("750"sv, {"Tolstopaltsevo"sv, "Marushkino"sv,
        "Rasskazovka"sv, "Marushkino"sv, "Tolstopaltsevo"sv});
     catalogue.AddBus("828"sv, {"Biryulyovo Zapadnoye"sv, "Universam"sv,
        "Rossoshanskaya ulitsa"sv, "Biryulyovo Zapadnoye"sv});

    std::string info1 = catalogue.StopInfo("Samara"sv);
    assert(info1 == "Stop Samara: not found"s);

    std::string info2 = catalogue.StopInfo("Prazhskaya"sv);
    assert(info2 == "Stop Prazhskaya: no buses"s);

    std::string info3 = catalogue.StopInfo("Biryulyovo Zapadnoye"sv);
    assert(info3 == "Stop Biryulyovo Zapadnoye: buses 256 828"s);
}

}
}