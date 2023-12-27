#pragma once

#include <deque>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "geo.h"

namespace tcat {

class TransportCatalogue {
public:
	void AddStop(std::string_view name, const geo::Coordinates& coordinates);
	void AddBus(std::string_view name, const std::vector<std::string_view>& stops);
	
	std::string BusInfo(std::string_view name) const;
	std::string StopInfo(std::string_view name) const;

private:
	struct Stop {
		std::string name;
		geo::Coordinates coordinates;
	};

	struct Bus {
		std::string name;
		std::vector<Stop*> stops;
	};

	Stop* FindStop(std::string_view name) const;
	Bus* FindBus(std::string_view name) const;

	size_t CountUniqueStops(const Bus* const bus) const;
	double CalculateRouteLength(const Bus* const bus) const;

	std::list<std::string_view> GetBusList(Stop* stop) const;

	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, Bus*> busname_to_bus_;

	std::unordered_map<Stop*, std::set<std::string_view>> buses_of_stop_;
};

namespace tests {

void GettingBusInfo();
void GettingStopInfo();

}
}