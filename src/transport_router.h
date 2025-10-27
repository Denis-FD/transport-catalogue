#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"


namespace transport_router {

	using namespace graph;
	using namespace transport_catalogue;

	constexpr double KMH_TO_MPM = 1000.0 / 60.0; // [km/h] to [m/min]

	struct RoutingSettings {
		int bus_wait_time; // 1 - 1000 [min] 
		double bus_velocity; // 1 - 1000 [km/h]
	};

	struct EdgeData {
		std::string bus_name;
		size_t span_count = 0;
	};

	struct RouteItems {
		enum class Type { Wait, Bus };
		Type type;
		std::string stop_name;
		std::string bus_name;
		size_t span_count = 0;
		double time = 0.0;
	};

	struct RouteData {
		double total_time = 0.0;
		std::vector<RouteItems> items;
	};

	class TransportRouter {
	public:
		TransportRouter(const TransportCatalogue& catalogue, const RoutingSettings& routing_settings);

		std::optional<RouteData> FindRoute(const std::string& from, const std::string& to) const;

	private:
		const TransportCatalogue& catalogue_;
		RoutingSettings routing_settings_;
		std::unordered_map<const Stop*, VertexId> stops_id_;
		std::unordered_map<VertexId, const Stop*> id_stops_;
		std::unordered_map<EdgeId, EdgeData> edge_data_;
		std::unique_ptr<DirectedWeightedGraph<double>> graph_;
		std::unique_ptr<Router<double>> router_;

		std::unique_ptr<DirectedWeightedGraph<double>> BuildGraph();

		std::unordered_set<const domain::Bus*> GetUniqueBuses(const std::deque<domain::Bus>& buses) const;

		void SetIdForStops(const std::unordered_set<const domain::Bus*>& buses);

		void AddBusEdges(DirectedWeightedGraph<double>& graph, const domain::Bus* bus);

		void AddGraphEdge(DirectedWeightedGraph<double>& graph, 
			const Stop* from, const Stop* to, const std::string& bus_name, 
			double cumulative_distance, size_t span_count);
	};

} // namespace transport_router
