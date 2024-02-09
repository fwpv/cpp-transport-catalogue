#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <cassert>
#include <utility>
#include <sstream>

using namespace std::literals;

namespace {
    svg::Point ReadPoint(const json::Node& node) {
        const json::Array& arr = node.AsArray();
        return svg::Point{arr.at(0).AsDouble(), arr.at(1).AsDouble()};
    }

    svg::Color ReadColor(const json::Node& node) {
        if (node.IsArray()) {
            const json::Array& arr = node.AsArray();
            if (arr.size() == 3) {
                uint8_t r = static_cast<uint8_t>(arr[0].AsInt());
                uint8_t g = static_cast<uint8_t>(arr[1].AsInt());
                uint8_t b = static_cast<uint8_t>(arr[2].AsInt());
                return svg::Color{svg::Rgb{r, g, b}};
            } else if (arr.size() == 4) {
                uint8_t r = static_cast<uint8_t>(arr[0].AsInt());
                uint8_t g = static_cast<uint8_t>(arr[1].AsInt());
                uint8_t b = static_cast<uint8_t>(arr[2].AsInt());
                double a = arr[3].AsDouble();
                return svg::Color{svg::Rgba{r, g, b, a}};
            }
        }
        return svg::Color{node.AsString()};
    }
}

JsonReader::JsonReader(tcat::TransportCatalogue& db, renderer::MapRenderer& map_renderer,
        const handler::RequestHandler& handler)
    : db_(db)
    , map_renderer_(map_renderer)
    , handler_(handler) {
}

void JsonReader::PopulateCatalogue(const json::Node& base_req_node) {
    const json::Array& req_array = base_req_node.AsArray();
    // Добавить остановки и их координаты
    for (const json::Node& node : req_array) {
        const json::Dict& obj = node.AsMap();
        if (obj.at("type"s).AsString() == "Stop"s) {
            const std::string& name = obj.at("name"s).AsString();
            double latitude = obj.at("latitude"s).AsDouble();
            double longitude = obj.at("longitude"s).AsDouble();
            geo::Coordinates coordinates{latitude, longitude};
            db_.AddStop(name, coordinates);
        }
    }
    // Добавить расстояния между остановками
    for (const json::Node& node : req_array) {
        const json::Dict& obj = node.AsMap();
        if (obj.at("type"s).AsString() == "Stop"s) {
            const std::string& name = obj.at("name"s).AsString();
            const json::Dict& dist_obj = obj.at("road_distances"s).AsMap();
            for (const auto& [key, distance_node] : dist_obj) {
                db_.AddDistance(name, key, distance_node.AsInt());
            }
        }
    }
    // Добавить маршруты
    for (const json::Node& node : req_array) {
        const json::Dict& obj = node.AsMap();
        if (obj.at("type"s).AsString() == "Bus"s) {
            const std::string& name = obj.at("name"s).AsString();
            const json::Array& stops_array = obj.at("stops"s).AsArray();
            std::vector<std::string_view> stops_vector;
            for (const json::Node& stop_name_node : stops_array) {
                stops_vector.push_back(stop_name_node.AsString());
            }
            bool is_roundtrip = obj.at("is_roundtrip"s).AsBool();
            db_.AddBus(name, stops_vector, is_roundtrip);
        }
    }
}

json::Array JsonReader::ProcessStatRequests(const json::Node& stat_req_node) const {
    json::Array result{};
    const json::Array& req_array = stat_req_node.AsArray();
    for (const json::Node& req_node : req_array) {
        const json::Dict& req_obj = req_node.AsMap();

        // Прочитать ключ запроса
        int id = req_obj.at("id"s).AsInt();
        const std::string& type = req_obj.at("type"s).AsString();

        // Сформировать ответ в зависимости от типа запроса
        json::Dict answer_obj{};
        answer_obj.emplace("request_id"s, id);
        if (type == "Map"s) {
            svg::Document document = handler_.RenderMap();
            std::ostringstream out;
            document.Render(out);
            answer_obj.emplace("map"s, out.str());
        } else if (type == "Bus"s) {
            const std::string& name = req_obj.at("name"s).AsString();
            std::optional<handler::BusStat> stat = handler_.GetBusStat(name);
            if (!stat) {
                answer_obj.emplace("error_message"s, "not found"s);
            } else {
                answer_obj.emplace("curvature"s, stat->curvature);
                answer_obj.emplace("route_length"s, stat->route_length);
                answer_obj.emplace("stop_count"s, stat->stop_count);
                answer_obj.emplace("unique_stop_count"s, stat->unique_stop_count);
            }

        } else if (type == "Stop"s) {
            const std::string& name = req_obj.at("name"s).AsString();
            const std::set<std::string_view>* bus_set_ptr = handler_.GetBusNamesByStop(name);
            if (!bus_set_ptr) {
                answer_obj.emplace("error_message"s, "not found"s);
            } else {
                json::Array buses_array{};
                for (const std::string_view bus_name : *bus_set_ptr) {
                    buses_array.emplace_back(std::string(bus_name));
                }
                answer_obj.emplace("buses"s, std::move(buses_array));
            }
        }
        result.emplace_back(std::move(answer_obj));
    }
    return result;
}

void JsonReader::ReadRenderSettings(const json::Node& render_settings_node) {
    renderer::MapRenderer::RenderSettings settings;
    const json::Dict& obj = render_settings_node.AsMap();

    settings.width = obj.at("width"s).AsDouble();
    settings.height = obj.at("height"s).AsDouble();

    settings.padding = obj.at("padding"s).AsDouble();

    settings.line_width = obj.at("line_width"s).AsDouble();
    settings.stop_radius = obj.at("stop_radius"s).AsDouble();

    settings.bus_label_font_size = obj.at("bus_label_font_size"s).AsInt();
    settings.bus_label_offset = ReadPoint(obj.at("bus_label_offset"s));

    settings.stop_label_font_size = obj.at("stop_label_font_size"s).AsInt();
    settings.stop_label_offset = ReadPoint(obj.at("stop_label_offset"s));

    settings.underlayer_color = ReadColor(obj.at("underlayer_color"s));
    settings.underlayer_width = obj.at("underlayer_width"s).AsDouble();

    const json::Array& color_palette = obj.at("color_palette"s).AsArray();
    for (const json::Node& color_node : color_palette) {
        settings.color_palette.push_back(ReadColor(color_node));
    }

    map_renderer_.SetRenderSettings(settings);
}