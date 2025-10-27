#include "transport_catalogue.h"

#include <sstream>
#include <unordered_set>

using namespace std::string_literals;

namespace transport_catalogue {

	void TransportCatalogue::AddStop(const Stop& stop) {
		auto it = stopname_to_stop_.find(stop.stop_name);
		if (it != stopname_to_stop_.end()) {
			return;
		}
		stops_.push_back(stop);
		const Stop& added_stop = stops_.back();
		stopname_to_stop_.insert({ added_stop.stop_name, &added_stop });
	}

	const Stop* TransportCatalogue::FindStop(std::string_view stop_name) const {
		auto it = stopname_to_stop_.find(stop_name);
		if (it == stopname_to_stop_.end()) {
			return nullptr;
		}
		return it->second;
	}

	void TransportCatalogue::AddBus(const Bus& bus) {
		auto it = busname_to_bus_.find(bus.bus_name);
		if (it != busname_to_bus_.end()) {
			return;
		}
		buses_.push_back(bus);
		const Bus& added_bus = buses_.back();
		busname_to_bus_.insert({ added_bus.bus_name, &added_bus });

		for (const Stop* stop_name : added_bus.bus_stops) {
			stop_to_buses_[stop_name].insert(added_bus.bus_name);
		}
	}

	const Bus* TransportCatalogue::FindBus(std::string_view bus_name) const {
		auto it = busname_to_bus_.find(bus_name);
		if (it == busname_to_bus_.end()) {
			return nullptr;
		}
		return it->second;
	}

	BusInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
		BusInfo bus_info;

		auto it = busname_to_bus_.find(bus_name);
		if (it == busname_to_bus_.end()) {
			return bus_info;
		}
		bus_info.bus_found = true;

		const Bus* bus = it->second;
		bus_info.stops_count = bus->bus_stops.size();
		std::unordered_set<const Stop*> unique_stops(bus->bus_stops.begin(), bus->bus_stops.end());
		bus_info.unique_stops_count = unique_stops.size();

		double geo_distance = 0.0;
		for (size_t i = 0; i < bus->bus_stops.size() - 1; i++) {
			const Stop* stop_1 = bus->bus_stops[i];
			const Stop* stop_2 = bus->bus_stops[i + 1];

			geo_distance += geo::ComputeDistance(stop_1->position, stop_2->position);
			bus_info.route_length += GetDistanceBetweenStops(stop_1, stop_2);
		}

		if (geo_distance > 0.0) {
			bus_info.curvature = bus_info.route_length / geo_distance;
		}

		return bus_info;
	}

	const std::set<std::string_view>& TransportCatalogue::GetBusesForStop(const Stop* stop) const {
		static const std::set<std::string_view> empty_buses;

		auto it = stop_to_buses_.find(stop);
		if (it == stop_to_buses_.end()) {
			return empty_buses;
		}
		return it->second;
	}

	void TransportCatalogue::AddDistanceBetweenStops(std::string_view stop_name_from,
		std::string_view stop_name_to, unsigned int distance) {
		const Stop* stop_from = FindStop(stop_name_from);
		const Stop* stop_to = FindStop(stop_name_to);

		if (!stop_from || !stop_to) {
			return;
		}

		if (distance_between_stops_.find({ stop_from , stop_to }) != distance_between_stops_.end()) {
			return;
		}

		distance_between_stops_[{ stop_from, stop_to}] = distance;
	}

	unsigned int TransportCatalogue::GetDistanceBetweenStops(const Stop* stop_from, const Stop* stop_to) const {
		auto it = distance_between_stops_.find({ stop_from , stop_to });

		if (it == distance_between_stops_.end()) {
			it = distance_between_stops_.find({ stop_to , stop_from });
			if (it == distance_between_stops_.end()) {
				return 0;
			}
		}

		return it->second;
	}

	const std::deque<Stop>& TransportCatalogue::GetAllStops() const {
		return stops_;
	}

	const std::deque<Bus>& TransportCatalogue::GetAllBuses() const {
		return buses_;
	}

	size_t TransportCatalogue::GetStopsCount() const {
		return stops_.size();
	}

} // namespace transport_catalogue