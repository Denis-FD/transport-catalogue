#include "transport_router.h"

namespace transport_router {

	using namespace graph;
	using namespace transport_catalogue;

	TransportRouter::TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings)
		: catalogue_(catalogue)
		, routing_settings_(routing_settings)
		, graph_(BuildGraph())
		, router_(std::make_unique<Router<double>>(*graph_)) {
	}

	std::optional<RouteData> TransportRouter::FindRoute(const std::string& from, const std::string& to) const {
		const Stop* stop_from = catalogue_.FindStop(from);
		const Stop* stop_to = catalogue_.FindStop(to);

		if (!stop_from || !stop_to || stops_id_.count(stop_from) == 0 || stops_id_.count(stop_to) == 0) {
			return std::nullopt;
		}

		auto route_info = router_->BuildRoute(stops_id_.at(stop_from), stops_id_.at(stop_to));
		if (!route_info) {
			return std::nullopt;
		}

		RouteData route_data;
		route_data.total_time = route_info->weight;

		for (const auto& edge_id : route_info->edges) {
			const auto& edge = edge_data_.at(edge_id);
			const auto& graph_edge = graph_->GetEdge(edge_id);

			route_data.items.push_back({
				RouteItems::Type::Wait,
				id_stops_.at(graph_edge.from)->stop_name,
				{},
				0,
				static_cast<double>(routing_settings_.bus_wait_time)
				});

			route_data.items.push_back({
				RouteItems::Type::Bus,
				{},
				edge.bus_name,
				edge.span_count,
				graph_edge.weight - routing_settings_.bus_wait_time
				});
		}
		return route_data;
	}

	std::unique_ptr<DirectedWeightedGraph<double>> TransportRouter::BuildGraph() {
		auto unique_buses = GetUniqueBuses(catalogue_.GetAllBuses());
		SetIdForStops(unique_buses);

		auto graph = std::make_unique<DirectedWeightedGraph<double>>(stops_id_.size());

		for (const auto* bus : unique_buses) {
			AddBusEdges(*graph, bus);
		}
		return graph;
	}

	std::unordered_set<const domain::Bus*> TransportRouter::GetUniqueBuses(const std::deque<domain::Bus>& buses) const {
		std::unordered_set<const domain::Bus*> unique_buses;
		for (const auto& bus : buses) {
			unique_buses.insert(&bus);
		}
		return unique_buses;
	}

	void TransportRouter::SetIdForStops(const std::unordered_set<const domain::Bus*>& buses) {
		VertexId id = 0;
		for (const auto& bus : buses) {
			for (const auto& stop : bus->bus_stops) {
				if (stops_id_.find(stop) == stops_id_.end()) {
					stops_id_[stop] = id;
					id_stops_[id] = stop;
					++id;
				}
			}
		}
	}

	void TransportRouter::AddBusEdges(DirectedWeightedGraph<double>& graph, const domain::Bus* bus) {
		const auto& stops = bus->bus_stops;
		for (size_t i = 0; i + 1 < stops.size(); ++i) {
			double total_forward_distance = 0.0;
			double total_backward_distance = 0.0;

			for (size_t j = i + 1; j < stops.size(); ++j) {
				total_forward_distance += catalogue_.GetDistanceBetweenStops(stops[j - 1], stops[j]);

				AddGraphEdge(graph, stops[i], stops[j], bus->bus_name, total_forward_distance, j - i);

				if (!bus->is_circular) {
					total_backward_distance += catalogue_.GetDistanceBetweenStops(stops[j], stops[j - 1]);
					AddGraphEdge(graph, stops[j], stops[i], bus->bus_name, total_backward_distance, j - i);
				}
			}
		}
	}

	void TransportRouter::AddGraphEdge(DirectedWeightedGraph<double>& graph, 
		const Stop* from, const Stop* to, const std::string& bus_name, 
		double cumulative_distance, size_t span_count) {

		double travel_time = cumulative_distance / (routing_settings_.bus_velocity * KMH_TO_MPM); // [min]
		double total_time = travel_time + routing_settings_.bus_wait_time;

		EdgeId edge_id = graph.AddEdge({ stops_id_.at(from), stops_id_.at(to), total_time });
		edge_data_[edge_id] = { bus_name, span_count };
	}


} // namespace transport_router

