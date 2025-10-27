#include "svg.h"

namespace svg {

    using namespace std::literals;

    namespace detail {
        void HtmlEncodeString(std::ostream& out, std::string_view sv) {
            for (char ch : sv) {
                switch (ch) {
                case '"': out << "&quot;"sv; break;
                case '<': out << "&lt;"sv; break;
                case '>': out << "&gt;"sv; break;
                case '&': out << "&amp;"sv; break;
                case '\'': out << "&apos;"sv; break;
                default: out.put(ch);
                }
            }
        }
    } // namespace detail

    void PrintColor(std::ostream& out, std::monostate) {
        out << "none"sv;
    }
    void PrintColor(std::ostream& out, std::string color) {
        out << color;
    }
    void PrintColor(std::ostream& out, svg::Rgb rgb) {
        out << "rgb("sv
            << static_cast<int>(rgb.red) << ','
            << static_cast<int>(rgb.green) << ','
            << static_cast<int>(rgb.blue)
            << ')';
    }
    void PrintColor(std::ostream& out, svg::Rgba rgba) {
        out << "rgba("sv
            << static_cast<int>(rgba.red) << ','
            << static_cast<int>(rgba.green) << ','
            << static_cast<int>(rgba.blue) << ','
            << rgba.opacity
            << ')';
    }

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit([&out](const auto& value) {
            PrintColor(out, value);
            }, color);
        return out;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тэга своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        using namespace std::literals;
        switch (line_cap) {
        case StrokeLineCap::BUTT: out << "butt"sv; break;
        case StrokeLineCap::ROUND: out << "round"sv; break;
        case StrokeLineCap::SQUARE: out << "square"sv; break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        using namespace std::literals;
        switch (line_join) {
        case StrokeLineJoin::ARCS: out << "arcs"sv; break;
        case StrokeLineJoin::BEVEL: out << "bevel"sv; break;
        case StrokeLineJoin::MITER: out << "miter"sv; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"sv; break;
        case StrokeLineJoin::ROUND: out << "round"sv; break;
        }
        return out;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Polyline ------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        bool is_first = true;
        for (const Point& p : points_) {
            if (is_first) {
                is_first = false;
            }
            else {
                out << ' ';
            }
            out << p.x << ',' << p.y;
        }
        out << "\""sv;
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // ---------- Text ------------------

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;
        RenderAttrs(context.out);
        using detail::RenderAttr;
        RenderAttr(out, " x"sv, position_.x);
        RenderAttr(out, " y"sv, position_.y);
        RenderAttr(out, " dx"sv, offset_.x);
        RenderAttr(out, " dy"sv, offset_.y);
        RenderAttr(out, " font-size"sv, font_size_);
        if (!font_family_.empty()) {
            RenderAttr(out, " font-family"sv, font_family_);
        }
        if (!font_weight_.empty()) {
            RenderAttr(out, " font-weight"sv, font_weight_);
        }
        out.put('>');
        detail::HtmlEncodeString(out, data_);
        out << "</text>"sv;
    }

    // Document

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx{ out, 2, 2 };
        for (const auto& obj : objects_) {
            obj->Render(ctx);
        }
        out << "</svg>"sv;
    }

}  // namespace svg