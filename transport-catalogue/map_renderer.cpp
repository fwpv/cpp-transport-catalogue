#include "map_renderer.h"

#include <algorithm>

using namespace std::literals;

namespace renderer {

svg::Color MapRenderer::RenderSettings::PickColor(size_t index) const {
    if (color_palette.size() > 0) {
        size_t i = index % color_palette.size();
        svg::Color color = color_palette[i];
        return color;
    }
    return svg::Color{};
}

void MapRenderer::SetRenderSettings(const RenderSettings& settings) {
    settings_ = settings;
}

void MapRenderer::RenderMap(const std::vector<const tcat::Bus*>& buses,
        svg::ObjectContainer& target) const {
    
    // Сформировать массив уникальных остановок
    std::vector<const tcat::Stop*> stops;
    for (const tcat::Bus* bus: buses) {
        stops.insert(stops.end(), bus->stops.begin(), bus->stops.end());
    }
    std::sort(stops.begin(), stops.end(), 
        [](const tcat::Stop* a, const tcat::Stop* b) { 
            return a->name < b->name; 
        });
    auto last = std::unique(stops.begin(), stops.end());
    
    // Сформировать вектор координат
    std::vector<geo::Coordinates> coordinates;
    coordinates.reserve(stops.size());
    for (auto it = stops.begin(); it != last; ++it) {
        const tcat::Stop* stop = *it;
        coordinates.push_back(stop->coordinates);
    }

    // Создать shpere_projector
    SphereProjector projector
        = MakeSphereProjector(coordinates.begin(), coordinates.end());

    // Создать изображения линий маршрутов
    std::vector<std::unique_ptr<svg::Drawable>> pictures;
    size_t color_index = 0;
    for (const tcat::Bus* bus: buses) {
        if (bus->StopCount() == 0) {
            continue;
        }
        std::unique_ptr<svg::Drawable> pic =
            std::make_unique<BusRouteLine>(projector, settings_, bus, color_index);
        pictures.emplace_back(std::move(pic));
        ++color_index;
    }

    // Создать изображения названий маршрутов
    color_index = 0;
    for (const tcat::Bus* bus: buses) {
        if (bus->StopCount() == 0) {
            continue;
        }
        svg::Point pos1 = projector(bus->stops.at(0)->coordinates);
        std::unique_ptr<svg::Drawable> pic =
            std::make_unique<BusRouteName>(settings_, pos1, bus->name, color_index);
        pictures.emplace_back(std::move(pic));

        if (!bus->is_roundtrip && !bus->stops.empty()
                && bus->stops[0] != bus->stops.back()) {
            svg::Point pos2 = projector(bus->stops.back()->coordinates);
            std::unique_ptr<svg::Drawable> pic =
                std::make_unique<BusRouteName>(settings_, pos2, bus->name, color_index);
            pictures.emplace_back(std::move(pic));
        }
        ++color_index;
    }

    // Создать изображения символов остановок
    for (auto it = stops.begin(); it != last; ++it) {
        const tcat::Stop* stop = *it;
        svg::Point pos = projector(stop->coordinates);
        std::unique_ptr<svg::Drawable> pic =
            std::make_unique<StopSymbol>(settings_, pos);
        pictures.emplace_back(std::move(pic));
    }

    // Создать изображения названий остановок
    for (auto it = stops.begin(); it != last; ++it) {
        const tcat::Stop* stop = *it;
        svg::Point pos = projector(stop->coordinates);
        std::unique_ptr<svg::Drawable> pic =
            std::make_unique<StopName>(settings_, pos, stop->name);
        pictures.emplace_back(std::move(pic));
    }

    // Отобразить все созданные изображения в формате svg::Document
    for (const std::unique_ptr<svg::Drawable>& pic : pictures) {
        pic->Draw(target);
    }
}

BusRouteLine::BusRouteLine(const SphereProjector& projector,
        const MapRenderer::RenderSettings& settings,
        const tcat::Bus* bus_ptr, size_t color_index)
    : projector_(projector)
    ,settings_(settings)
    ,bus_ptr_(bus_ptr)
    , color_index_(color_index) {
}

void BusRouteLine::Draw(svg::ObjectContainer& target) const {
    // Нарисовать линии маршрутов
    svg::Polyline polyline;
    for (const tcat::Stop* stop: bus_ptr_->stops) {
        polyline.AddPoint(projector_(stop->coordinates));
    }
    if (!bus_ptr_->is_roundtrip && !bus_ptr_->stops.empty()) {
        for (auto it = bus_ptr_->stops.rbegin() + 1; it != bus_ptr_->stops.rend(); ++it) {
            const tcat::Stop* stop = *it;
            polyline.AddPoint(projector_(stop->coordinates));
        }
    }
    polyline.SetStrokeColor(settings_.PickColor(color_index_));
    polyline.SetStrokeWidth(settings_.line_width);
    polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    polyline.SetFillColor(svg::Color{});
    target.Add(polyline);
}

BusRouteName::BusRouteName(const MapRenderer::RenderSettings& settings,
        svg::Point pos, const std::string& data, size_t color_index)
    : settings_(settings)
    , pos_(pos)
    , data_(data)
    , color_index_(color_index) {
}

void BusRouteName::Draw(svg::ObjectContainer& target) const {
    svg::Text underlayer_text, text;

    underlayer_text.SetPosition(pos_);
    text.SetPosition(pos_);

    underlayer_text.SetOffset(settings_.bus_label_offset);
    text.SetOffset(settings_.bus_label_offset);

    underlayer_text.SetFontSize(settings_.bus_label_font_size);
    text.SetFontSize(settings_.bus_label_font_size);

    underlayer_text.SetFontFamily("Verdana"s);
    text.SetFontFamily("Verdana"s);

    underlayer_text.SetFontWeight("bold"s);
    text.SetFontWeight("bold"s);

    underlayer_text.SetData(data_);
    text.SetData(data_);

    underlayer_text.SetFillColor(settings_.underlayer_color);
    underlayer_text.SetStrokeColor(settings_.underlayer_color);
    underlayer_text.SetStrokeWidth(settings_.underlayer_width);
    underlayer_text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer_text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    text.SetFillColor(settings_.PickColor(color_index_));

    target.Add(underlayer_text);
    target.Add(text);
}

StopSymbol::StopSymbol(const MapRenderer::RenderSettings& settings, svg::Point pos)
    : settings_(settings)
    , pos_(pos) {
}

void StopSymbol::Draw(svg::ObjectContainer& target) const {
    svg::Circle circle;
    circle.SetCenter(pos_);
    circle.SetRadius(settings_.stop_radius);
    circle.SetFillColor(svg::Color{"white"s});
    target.Add(circle);
}

StopName::StopName(const MapRenderer::RenderSettings& settings,
        svg::Point pos, const std::string& data)
    : settings_(settings)
    , pos_(pos)
    , data_(data) {
}

void StopName::Draw(svg::ObjectContainer& target) const {
    svg::Text underlayer_text, text;

    underlayer_text.SetPosition(pos_);
    text.SetPosition(pos_);

    underlayer_text.SetOffset(settings_.stop_label_offset);
    text.SetOffset(settings_.stop_label_offset);

    underlayer_text.SetFontSize(settings_.stop_label_font_size);
    text.SetFontSize(settings_.stop_label_font_size);

    underlayer_text.SetFontFamily("Verdana"s);
    text.SetFontFamily("Verdana"s);

    underlayer_text.SetData(data_);
    text.SetData(data_);

    underlayer_text.SetFillColor(settings_.underlayer_color);
    underlayer_text.SetStrokeColor(settings_.underlayer_color);
    underlayer_text.SetStrokeWidth(settings_.underlayer_width);
    underlayer_text.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    underlayer_text.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    text.SetFillColor(svg::Color{"black"s});

    target.Add(underlayer_text);
    target.Add(text);
}

}