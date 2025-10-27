#pragma once

#include <optional>

#include "transport_catalogue.h"
#include "map_renderer.h"

namespace request_handler {
	using namespace transport_catalogue;

	class RequestHandler {
	public:
		RequestHandler(const TransportCatalogue& db, const map_renderer::MapRenderer& renderer);
		std::optional<domain::BusInfo> GetBusStat(const std::string_view& bus_name) const;
		const std::set<std::string_view>* GetBusesByStop(const std::string_view& stop_name) const;
		svg::Document RenderMap() const;

	private:
		const TransportCatalogue& db_;
		const map_renderer::MapRenderer& renderer_;
	};

} // request_handler