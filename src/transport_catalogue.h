#pragma once

#include <deque>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "domain.h"

namespace transport_catalogue {
	using namespace domain;

	struct PairStopsHash {
		size_t operator() (const std::pair<const Stop*, const Stop*>& stops) const {
			size_t stop_from_hash = stop_hasher_(stops.first);
			size_t stop_to_hash = stop_hasher_(stops.second) * 37;

			return stop_from_hash + stop_to_hash;
		}
	private:
		std::hash<const Stop*> stop_hasher_;
	};

	class TransportCatalogue {
	public:
		void AddStop(const Stop& stop);
		const Stop* FindStop(std::string_view stop_name) const;
		void AddBus(const Bus& bus);
		const Bus* FindBus(std::string_view bus_name) const;
		BusInfo GetBusInfo(std::string_view bus_name) const;
		const std::set<std::string_view>& GetBusesForStop(const Stop* stop) const;
		void AddDistanceBetweenStops(std::string_view stop_name_from,
			std::string_view stop_name_to, unsigned int distance);
		unsigned int GetDistanceBetweenStops(const Stop* stop_from, const Stop* stop_to) const;
		const std::deque<Stop>& GetAllStops() const;
		const std::deque<Bus>& GetAllBuses() const;
		size_t GetStopsCount() const;

	private:
		std::deque<Stop> stops_;
		std::unordered_map<std::string_view, const Stop*> stopname_to_stop_;
		std::deque<Bus> buses_;
		std::unordered_map<std::string_view, const Bus*> busname_to_bus_;
		std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
		std::unordered_map<std::pair<const Stop*, const Stop*>, unsigned int, PairStopsHash> distance_between_stops_;
	};

} // namespace transport_catalogue