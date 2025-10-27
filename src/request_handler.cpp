#include "request_handler.h"

namespace request_handler {

	RequestHandler::RequestHandler(const TransportCatalogue& db,
		const map_renderer::MapRenderer& renderer) :
		db_(db), renderer_(renderer) {
	}

	std::optional<domain::BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
		if (!db_.FindBus(bus_name)) {
			return std::nullopt;
		}
		return db_.GetBusInfo(bus_name);
	}

	const std::set<std::string_view>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
		const auto* stop = db_.FindStop(stop_name);
		if (!stop) {
			return nullptr;
		}
		return &db_.GetBusesForStop(stop);
	}

	svg::Document RequestHandler::RenderMap() const {
		return renderer_.RenderMap(db_.GetAllBuses());
	}

} // request_handler