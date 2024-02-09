#pragma once

/*
 * Код, отвечающий за визуализацию карты маршрутов в формате SVG.
 */

#include "domain.h"
#include "geo.h"
#include "svg.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace renderer {

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
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
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    struct RenderSettings {
        // Ширина изображения в пикселях
        double width = 0.0;
        // Высота изображения в пикселях
        double height = 0.0;
        // Отступ краёв карты от границ SVG-документа
        double padding = 0.0;
        // Толщина линий, которыми рисуются автобусные маршруты
        double line_width = 0.0;
        // Радиус окружностей, которыми обозначаются остановки
        double stop_radius = 0.0;
        // Размер текста, которым написаны названия автобусных маршрутов
        int bus_label_font_size = 0;
        // Смещение надписи с названием маршрута относительно координат
        // конечной остановки на карте
        svg::Point bus_label_offset;
        // Размер текста, которым отображаются названия остановок
        int stop_label_font_size = 0;
        // Смещение названия остановки относительно её координат на карте
        svg::Point stop_label_offset;
        // Цвет подложки под названиями остановок и маршрутов
        svg::Color underlayer_color;
        // Толщина подложки под названиями остановок и маршрутов
        double underlayer_width = 0.0;
        // Цветовая палитра
        std::vector<svg::Color> color_palette;
        // Получить цвет из палитры по индексу
        svg::Color PickColor(size_t index) const;
    };

    void SetRenderSettings(const RenderSettings& settings);

    template <typename PointInputIt>
    SphereProjector MakeSphereProjector(PointInputIt points_begin,
            PointInputIt points_end) const {
        return SphereProjector{
            points_begin, points_end, settings_.width,
            settings_.height, settings_.padding};
    }

    void RenderMap(const std::vector<const tcat::Bus*>& buses,
            svg::ObjectContainer& target) const;

private:
    RenderSettings settings_;
};

class BusRouteLine : public svg::Drawable {
public:
    BusRouteLine(const SphereProjector& projector,
            const MapRenderer::RenderSettings& settings,
            const tcat::Bus* bus_ptr, size_t color_index);

    void Draw(svg::ObjectContainer& target) const override;

private:
    const SphereProjector& projector_;
    const MapRenderer::RenderSettings& settings_;
    const tcat::Bus* bus_ptr_;
    size_t color_index_;
};

class BusRouteName : public svg::Drawable {
public:
    BusRouteName(const MapRenderer::RenderSettings& settings,
            svg::Point pos, const std::string& data, size_t color_index);

    void Draw(svg::ObjectContainer& target) const override;

private:
    const MapRenderer::RenderSettings& settings_;
    svg::Point pos_;
    std::string data_;
    size_t color_index_;
};

class StopSymbol : public svg::Drawable {
public:
    StopSymbol(const MapRenderer::RenderSettings& settings, svg::Point pos);

    void Draw(svg::ObjectContainer& target) const override;

private:
    const MapRenderer::RenderSettings& settings_;
    svg::Point pos_;
};

class StopName : public svg::Drawable {
public:
    StopName(const MapRenderer::RenderSettings& settings,
            svg::Point pos, const std::string& data);

    void Draw(svg::ObjectContainer& target) const override;

private:
    const MapRenderer::RenderSettings& settings_;
    svg::Point pos_;
    std::string data_;
};

}