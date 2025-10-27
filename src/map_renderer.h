#pragma once

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <deque>

#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace map_renderer {

	inline const double EPSILON = 1e-6;
	bool IsZero(double value);

	class SphereProjector {
	public:
		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end,
			double max_width, double max_height, double padding);

		svg::Point operator()(geo::Coordinates coords) const;

	private:
		double padding_;
		double min_lon_ = 0;
		double max_lat_ = 0;
		double zoom_coeff_ = 0;
	};

	struct RenderSettings {
		double width;
		double height;
		double padding;
		double line_width;
		double stop_radius;
		int bus_label_font_size;
		svg::Point bus_label_offset;
		int stop_label_font_size;
		svg::Point stop_label_offset;
		svg::Color underlayer_color; // цвет подложки под названиями остановок и маршрутов
		double underlayer_width;
		std::vector<svg::Color> color_palette;  // цветовая палитра. Непустой массив.
	};

	class MapRenderer {
	public:
		MapRenderer(const RenderSettings& settings);
		svg::Document RenderMap(const std::deque<domain::Bus>& buses) const;

	private:
		RenderSettings render_settings_;
	};
}