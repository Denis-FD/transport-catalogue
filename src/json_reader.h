#pragma once

#include <iostream>

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_router.h"


namespace transport_catalogue {

	class JsonReader {
	public:
		JsonReader(TransportCatalogue& catalogue);

		void ParseInput(std::istream& input);
		void PrintOutput(std::ostream& output) const;

	private:
		void ProcessBaseRequests(const json::Document& doc);
		json::Array ProcessStatRequests(const json::Document& doc) const;
		map_renderer::RenderSettings ProcessRenderRequest(const json::Document& doc) const;
		void InitializeTransportRouter(const json::Document& doc);

		TransportCatalogue& catalogue_;
		json::Array stat_responses_;
		std::optional<transport_router::TransportRouter> transport_router_;
	};

} // namespace transport_catalogue