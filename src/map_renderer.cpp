#include "map_renderer.h"

#include <set>

#include <map>
#include <unordered_set>


namespace map_renderer {

	bool IsZero(double value) {
		return std::abs(value) < EPSILON;
	}

	// ===== class SphereProjector =====

	// points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
	template <typename PointInputIt>
	SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
		double max_width, double max_height, double padding)
		: padding_(padding) //
	{
		// Если точки поверхности сферы не заданы, вычислять нечего
		if (points_begin == points_end) {
			return;
		}

		// Находим точки с минимальной и максимальной долготой
		const auto [left_it, right_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
		min_lon_ = left_it->lng;
		const double max_lon = right_it->lng;

		// Находим точки с минимальной и максимальной широтой
		const auto [bottom_it, top_it] = std::minmax_element(
			points_begin, points_end,
			[](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
		const double min_lat = bottom_it->lat;
		max_lat_ = top_it->lat;

		// Вычисляем коэффициент масштабирования вдоль координаты x
		std::optional<double> width_zoom;
		if (!IsZero(max_lon - min_lon_)) {
			width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
		}

		// Вычисляем коэффициент масштабирования вдоль координаты y
		std::optional<double> height_zoom;
		if (!IsZero(max_lat_ - min_lat)) {
			height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
		}

		if (width_zoom && height_zoom) {
			// Коэффициенты масштабирования по ширине и высоте ненулевые,
			// берём минимальный из них
			zoom_coeff_ = std::min(*width_zoom, *height_zoom);
		}
		else if (width_zoom) {
			// Коэффициент масштабирования по ширине ненулевой, используем его
			zoom_coeff_ = *width_zoom;
		}
		else if (height_zoom) {
			// Коэффициент масштабирования по высоте ненулевой, используем его
			zoom_coeff_ = *height_zoom;
		}
	}

	// Проецирует широту и долготу в координаты внутри SVG-изображения
	svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
		return {
			(coords.lng - min_lon_) * zoom_coeff_ + padding_,
			(max_lat_ - coords.lat) * zoom_coeff_ + padding_
		};
	}

	// ----- namespace -----

	namespace {
		struct BusComparator {
			bool operator()(const domain::Bus* lhs, const domain::Bus* rhs) const {
				return lhs->bus_name < rhs->bus_name;
			}
		};

		std::set<const domain::Bus*, BusComparator> GetSortedUniqueBuses(const std::deque<domain::Bus>& buses) {
			std::set<const domain::Bus*, BusComparator> sorted_buses;
			for (const auto& bus : buses) {
				sorted_buses.insert(&bus);
			}
			return sorted_buses;
		}

		std::map<std::string, geo::Coordinates> GetSortedUniqueStops(const std::set<const domain::Bus*, BusComparator>& sorted_buses) {
			std::map<std::string, geo::Coordinates> unique_stops;
			for (const auto& bus : sorted_buses) {
				for (const auto& stop : bus->bus_stops) {
					unique_stops[stop->stop_name] = stop->position;
				}
			}
			return unique_stops;
		}

		std::vector<geo::Coordinates> GetUniqueCoordinates(const std::map<std::string, geo::Coordinates>& unique_stops) {
			std::vector<geo::Coordinates> geo_coords;
			geo_coords.reserve(unique_stops.size());
			for (const auto& [stop_name, coordinates] : unique_stops) {
				geo_coords.push_back(coordinates);
			}
			return geo_coords;
		}


	} // namespace

	// ----- namespace detail -----

	namespace detail {
		void RenderRoutes(svg::Document& doc, const SphereProjector& projector, 
			const RenderSettings& render_settings,
			const std::set<const domain::Bus*, BusComparator>& sorted_buses) {

			size_t color_index = 0;

			for (const auto& bus : sorted_buses) {
				if (bus->bus_stops.empty()) {
					continue;
				}

				svg::Polyline polyline;
				polyline.SetStrokeColor(render_settings.color_palette[color_index % render_settings.color_palette.size()])
					.SetStrokeWidth(render_settings.line_width)
					.SetFillColor(svg::NoneColor)
					.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
					.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

				for (const auto* stop : bus->bus_stops) {
					polyline.AddPoint(projector(stop->position));
				}

				doc.Add(polyline);
				++color_index;
			}
		}

		void RenderBusNames(svg::Document& doc, const SphereProjector& projector,
			const RenderSettings& render_settings,
			const std::set<const domain::Bus*, BusComparator>& sorted_buses) {

			size_t color_index = 0;

			for (const auto& bus : sorted_buses) {
				if (bus->bus_stops.empty()) {
					continue;
				}

				std::vector<geo::Coordinates> bus_names_coord;
				bus_names_coord.push_back(bus->bus_stops.front()->position);

				if (!bus->is_circular) {
					size_t stops_count = bus->bus_stops.size();
					geo::Coordinates last = bus->bus_stops[stops_count / 2]->position;
					if (bus->bus_stops.front()->position != last) {
						bus_names_coord.push_back(last);
					}
				}

				for (const auto& position : bus_names_coord) {
					// Подложка
					doc.Add(svg::Text()
						.SetPosition(projector(position))
						.SetOffset(render_settings.bus_label_offset)
						.SetFontSize(render_settings.bus_label_font_size)
						.SetFontFamily("Verdana")
						.SetFontWeight("bold")
						.SetData(bus->bus_name)
						.SetFillColor(render_settings.underlayer_color)
						.SetStrokeColor(render_settings.underlayer_color)
						.SetStrokeWidth(render_settings.underlayer_width)
						.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
						.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
					);
					// Текст
					doc.Add(svg::Text()
						.SetPosition(projector(position))
						.SetOffset(render_settings.bus_label_offset)
						.SetFontSize(render_settings.bus_label_font_size)
						.SetFontFamily("Verdana")
						.SetFontWeight("bold")
						.SetData(bus->bus_name)
						.SetFillColor(render_settings.color_palette[color_index % render_settings.color_palette.size()])
					);
				}
				++color_index;
			}
		}

		void RenderStops(svg::Document& doc, const SphereProjector& projector,
			const RenderSettings& render_settings,
			const std::map<std::string, geo::Coordinates>& unique_stops) {

			for (const auto& [stop, position] : unique_stops) {
				// Круги
				doc.Add(svg::Circle()
					.SetCenter(projector(position))
					.SetRadius(render_settings.stop_radius)
					.SetFillColor("white")
				);
			}
		}

		void RenderStopNames(svg::Document& doc, const SphereProjector& projector,
			const RenderSettings& render_settings,
			const std::map<std::string, geo::Coordinates>& unique_stops) {

			for (const auto& [stop, position] : unique_stops) {
				// Подложка
				doc.Add(svg::Text()
					.SetPosition(projector(position))
					.SetOffset(render_settings.stop_label_offset)
					.SetFontSize(render_settings.stop_label_font_size)
					.SetFontFamily("Verdana")
					.SetData(stop)
					.SetFillColor(render_settings.underlayer_color)
					.SetStrokeColor(render_settings.underlayer_color)
					.SetStrokeWidth(render_settings.underlayer_width)
					.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
					.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
				);
				// Текст
				doc.Add(svg::Text()
					.SetPosition(projector(position))
					.SetOffset(render_settings.stop_label_offset)
					.SetFontSize(render_settings.stop_label_font_size)
					.SetFontFamily("Verdana")
					.SetData(stop)
					.SetFillColor("black")
				);
			}
		}
	}

	// ===== class MapRenderer =====

	MapRenderer::MapRenderer(const RenderSettings& settings) :
		render_settings_(settings) {
	}

	svg::Document MapRenderer::RenderMap(const std::deque<domain::Bus>& buses) const {
		svg::Document doc;

		if (buses.empty()) { return doc; }

		const auto& sorted_buses = GetSortedUniqueBuses(buses);
		const auto& unique_stops = GetSortedUniqueStops(sorted_buses);
		const auto& geo_coords = GetUniqueCoordinates(unique_stops);

		SphereProjector projector(
			geo_coords.begin(), geo_coords.end(),
			render_settings_.width, render_settings_.height, render_settings_.padding);

		detail::RenderRoutes(doc, projector, render_settings_, sorted_buses);
		detail::RenderBusNames(doc, projector, render_settings_, sorted_buses);
		detail::RenderStops(doc, projector, render_settings_, unique_stops);
		detail::RenderStopNames(doc, projector, render_settings_, unique_stops);

		return doc;
	}

} // map_renderer