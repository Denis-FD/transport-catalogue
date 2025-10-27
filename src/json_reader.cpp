#include "json_reader.h"

#include <vector>
#include <sstream>

#include "json_builder.h"

namespace detail {
	svg::Color ParseColor(const json::Node& color_node) {
		if (color_node.IsString()) {
			return color_node.AsString();
		}
		else if (color_node.IsArray()) {
			const auto& color_array = color_node.AsArray();
			if (color_array.size() == 3) {
				return svg::Rgb{
					static_cast<uint8_t>(color_array[0].AsInt()),
					static_cast<uint8_t>(color_array[1].AsInt()),
					static_cast<uint8_t>(color_array[2].AsInt())
				};
			}
			else if (color_array.size() == 4) {
				return svg::Rgba{
					static_cast<uint8_t>(color_array[0].AsInt()),
					static_cast<uint8_t>(color_array[1].AsInt()),
					static_cast<uint8_t>(color_array[2].AsInt()),
					color_array[3].AsDouble()
				};
			}
		}
		return svg::NoneColor;
	}

	std::vector<svg::Color> ReadColorPalette(const json::Array& color_array) {
		std::vector<svg::Color> color_palette;
		color_palette.reserve(color_array.size());
		for (const auto& color : color_array) {
			color_palette.push_back(ParseColor(color));
		}
		return color_palette;
	}

	svg::Point ReadLabelOffset(const json::Array& label_offset) {
		return {
			label_offset[0].AsDouble(), // dx
			label_offset[1].AsDouble() // dy
		};
	}
} // namespace detail


namespace transport_catalogue {
	using namespace std::literals;

	JsonReader::JsonReader(TransportCatalogue& catalogue) :
		catalogue_(catalogue) {
	}

	void JsonReader::ProcessBaseRequests(const json::Document& doc) {
		const json::Array& base_requests = doc.GetRoot()
			.AsMap().at("base_requests").AsArray();

		// 1. Add Stops
		for (const auto& request : base_requests) {
			const auto& request_map = request.AsMap();

			if (request_map.at("type").AsString() == "Stop") {
				std::string stop_name = request_map.at("name").AsString();

				catalogue_.AddStop({ stop_name,
					{request_map.at("latitude").AsDouble(),
					request_map.at("longitude").AsDouble()} });
			}
		}
		// 2. Add Distances
		for (const auto& request : base_requests) {
			const auto& request_map = request.AsMap();

			if (request_map.at("type").AsString() == "Stop") {
				std::string stop_name = request_map.at("name").AsString();

				if (request_map.count("road_distances")) {
					for (const auto& [stop_name_to, distance] :
						request_map.at("road_distances").AsMap())
						catalogue_.AddDistanceBetweenStops(
							stop_name,
							stop_name_to,
							distance.AsInt()
						);
				}
			}
		}
		// 3. Add Buses
		for (const auto& request : base_requests) {
			const auto& request_map = request.AsMap();

			if (request_map.at("type").AsString() == "Bus") {
				std::string bus_name = request_map.at("name").AsString();
				const auto& stop_names = request_map.at("stops").AsArray();
				bool is_roundtrip = request_map.at("is_roundtrip").AsBool();

				std::vector<const Stop*> bus_stops;
				if (is_roundtrip) {
					bus_stops.reserve(stop_names.size());
				}
				else {
					bus_stops.reserve(stop_names.size() * 2 - 1);
				}

				for (const auto& stop_name : stop_names) {
					bus_stops.push_back(catalogue_.FindStop(stop_name.AsString()));
				}

				if (!is_roundtrip && !bus_stops.empty()) {
					bus_stops.insert(bus_stops.end(), bus_stops.rbegin() + 1, bus_stops.rend());
				}

				catalogue_.AddBus({
					bus_name,
					std::move(bus_stops),
					is_roundtrip
					});
			}
		}
	}

	void JsonReader::InitializeTransportRouter(const json::Document& doc) {
		const json::Dict& routing_settings = doc.GetRoot()
			.AsMap().at("routing_settings").AsMap();

		transport_router_.emplace(catalogue_, transport_router::RoutingSettings{
			routing_settings.at("bus_wait_time").AsInt(),
			routing_settings.at("bus_velocity").AsDouble()
			});
	}

	json::Array JsonReader::ProcessStatRequests(const json::Document& doc) const {
		const json::Array& stat_requests = doc.GetRoot()
			.AsMap().at("stat_requests").AsArray();
		json::Array result;

		for (const auto& request : stat_requests) {
			const auto& request_map = request.AsMap();

			json::Builder response;
			auto dict_context = response.StartDict();

			dict_context.Key("request_id").Value(request_map.at("id").AsInt());

			// "Map" command
			if (request_map.at("type").AsString() == "Map") {

				map_renderer::MapRenderer renderer(ProcessRenderRequest(doc));
				request_handler::RequestHandler request_handler(catalogue_, renderer);
				svg::Document map = request_handler.RenderMap();
				std::ostringstream svg_output;
				map.Render(svg_output);

				dict_context.Key("map").Value(svg_output.str());
			}

			// "Stop" command
			if (request_map.at("type").AsString() == "Stop") {
				const Stop* stop = catalogue_.FindStop(request_map.at("name").AsString());
				if (!stop) {
					dict_context.Key("error_message").Value("not found"s);
				}
				else {
					const std::set<std::string_view>& buses = catalogue_.GetBusesForStop(stop);
					json::Array buses_resp;
					for (const auto& bus : buses) {
						buses_resp.emplace_back(std::string(bus));
					}
					dict_context.Key("buses").Value(std::move(buses_resp));
				}
			}

			// "Bus" command
			if (request_map.at("type").AsString() == "Bus") {
				BusInfo bus_info = catalogue_.GetBusInfo(request_map.at("name").AsString());

				if (!bus_info.bus_found) {
					dict_context.Key("error_message").Value("not found"s);
				}
				else {
					dict_context.Key("curvature").Value(bus_info.curvature)
						.Key("route_length").Value(static_cast<double>(bus_info.route_length))
						.Key("stop_count").Value(static_cast<int>(bus_info.stops_count))
						.Key("unique_stop_count").Value(static_cast<int>(bus_info.unique_stops_count));
				}
			}

			// "Route" command
			if (request_map.at("type").AsString() == "Route") {
				const auto& route_data = transport_router_->FindRoute(
					request_map.at("from").AsString(),
					request_map.at("to").AsString()
				);

				if (!route_data) {
					dict_context.Key("error_message").Value("not found"s);
				}
				else {
					dict_context.Key("total_time").Value(route_data->total_time);

					json::Array items;
					for (const auto& item : route_data->items) {
						json::Dict val;
						if (item.type == transport_router::RouteItems::Type::Wait) {
							val["type"] = "Wait";
							val["stop_name"] = item.stop_name;
						}
						else if (item.type == transport_router::RouteItems::Type::Bus) {
							val["type"] = "Bus";
							val["bus"] = item.bus_name;
							val["span_count"] = static_cast<int>(item.span_count);
						}
						val["time"] = item.time;
						items.push_back(std::move(val));
					}
					dict_context.Key("items").Value(std::move(items));
				}
			}

			dict_context.EndDict();
			result.emplace_back(response.Build());
		}

		return result;
	}

	map_renderer::RenderSettings JsonReader::ProcessRenderRequest(const json::Document& doc) const {
		const json::Dict& render_settings = doc.GetRoot()
			.AsMap().at("render_settings").AsMap();

		return {
		render_settings.at("width").AsDouble(),
		render_settings.at("height").AsDouble(),
		render_settings.at("padding").AsDouble(),
		render_settings.at("line_width").AsDouble(),
		render_settings.at("stop_radius").AsDouble(),
		render_settings.at("bus_label_font_size").AsInt(),
		detail::ReadLabelOffset(render_settings.at("bus_label_offset").AsArray()),
		render_settings.at("stop_label_font_size").AsInt(),
		detail::ReadLabelOffset(render_settings.at("stop_label_offset").AsArray()),
		detail::ParseColor(render_settings.at("underlayer_color")),
		render_settings.at("underlayer_width").AsDouble(),
		detail::ReadColorPalette(render_settings.at("color_palette").AsArray())
		};
	}

	void JsonReader::ParseInput(std::istream& input) {
		json::Document doc = json::Load(input);
		ProcessBaseRequests(doc);
		InitializeTransportRouter(doc);
		stat_responses_ = ProcessStatRequests(doc);
	}

	void JsonReader::PrintOutput(std::ostream& output) const {
		json::Print(json::Document{ stat_responses_ }, output);
	}

} // namespace transport_catalogue