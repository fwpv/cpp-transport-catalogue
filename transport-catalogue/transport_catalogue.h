#pragma once

/*
 * Код транспортного справочника
 */

#include "domain.h"
#include "geo.h"

#include <deque>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tcat {

class TransportCatalogue {
public:
	void AddStop(std::string_view name, const geo::Coordinates& coordinates);
	void AddBus(std::string_view name,
		const std::vector<std::string_view>& stops, bool is_roundtrip);
	void AddDistance(std::string_view start, const std::string_view& end, int distance);

    const Stop* FindStop(std::string_view name) const;
	const Bus* FindBus(std::string_view name) const;
	int GetDistance(const Stop* start_stop, const Stop* end_stop) const;

	std::vector<const Bus*> GetAllBuses() const;
	std::vector<const Stop*> GetAllStops() const;
	size_t GetStopCount() const;

	double CalculateGeoRouteLength(const Bus* bus) const;
	int CalculateRouteLength(const Bus* bus) const;

	const std::set<std::string_view>* GetBusNamesByStop(const Stop* stop) const;

private:
	struct Hasher {
		size_t operator() (const std::pair<const Stop*, const Stop*>& value) const {
			return hasher_(value.first) + hasher_(value.second);
		}
	private:
		std::hash<const Stop*> hasher_;
	};

	std::deque<Stop> stops_;
	std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
	std::deque<Bus> buses_;
	std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
	std::unordered_map<const Stop*, std::set<std::string_view>> buses_of_stop_;
	std::unordered_map<std::pair<const Stop*, const Stop*>, int, Hasher> distances_;
};

namespace tests {

void GettingBusInfo();
void GettingStopInfo();

}
}