#include "json_builder.h"
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
        const json::Dict& obj = node.AsDict();
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
        const json::Dict& obj = node.AsDict();
        if (obj.at("type"s).AsString() == "Stop"s) {
            const std::string& name = obj.at("name"s).AsString();
            const json::Dict& dist_obj = obj.at("road_distances"s).AsDict();
            for (const auto& [key, distance_node] : dist_obj) {
                db_.AddDistance(name, key, distance_node.AsInt());
            }
        }
    }
    // Добавить маршруты
    for (const json::Node& node : req_array) {
        const json::Dict& obj = node.AsDict();
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

json::Node JsonReader::ProcessStatRequests(const json::Node& stat_req_node) const {
    json::Builder builder{};
    json::ArrayValueContext array_builder = builder.StartArray();
    const json::Array& req_array = stat_req_node.AsArray();
    for (const json::Node& req_node : req_array) {
        const json::Dict& req_obj = req_node.AsDict();

        // Прочитать ключ запроса
        int id = req_obj.at("id"s).AsInt();
        const std::string& type = req_obj.at("type"s).AsString();

        // Сформировать ответ в зависимости от типа запроса
        json::DictItemContext dict_builder = array_builder.StartDict();
        dict_builder.Key("request_id"s).Value(id);
        if (type == "Map"s) {
            svg::Document document = handler_.RenderMap();
            std::ostringstream out;
            document.Render(out);
            dict_builder.Key("map"s).Value(out.str());
        } else if (type == "Bus"s) {
            const std::string& name = req_obj.at("name"s).AsString();
            std::optional<handler::BusStat> stat = handler_.GetBusStat(name);
            if (!stat) {
                dict_builder.Key("error_message"s).Value("not found"s);
            } else {
                dict_builder.Key("curvature"s).Value(stat->curvature)
                    .Key("route_length"s).Value(stat->route_length)
                    .Key("stop_count"s).Value(stat->stop_count)
                    .Key("unique_stop_count"s).Value(stat->unique_stop_count);
            }

        } else if (type == "Stop"s) {
            const std::string& name = req_obj.at("name"s).AsString();
            const std::set<std::string_view>* bus_set_ptr = handler_.GetBusNamesByStop(name);
            if (!bus_set_ptr) {
                dict_builder.Key("error_message"s).Value("not found"s);
            } else {
                json::ArrayValueContext array_builder = dict_builder.Key("buses"s).StartArray();
                for (const std::string_view bus_name : *bus_set_ptr) {
                    array_builder.Value(std::string(bus_name));
                }
                array_builder.EndArray();
            }
        }
        dict_builder.EndDict();
    }
    array_builder.EndArray();
    return builder.Build();
}

void JsonReader::ReadRenderSettings(const json::Node& render_settings_node) {
    renderer::MapRenderer::RenderSettings settings;
    const json::Dict& obj = render_settings_node.AsDict();

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