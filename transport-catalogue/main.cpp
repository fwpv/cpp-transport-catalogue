#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

int main() {
    tcat::TransportCatalogue catalogue;
    renderer::MapRenderer map_renderer;
    trouter::TransportRouter router;
    handler::RequestHandler request_handler(catalogue, map_renderer, router);
    JsonReader json_reader(catalogue, map_renderer, router, request_handler);

    // Прочитать json::Document из cin
    const json::Document input_document = json::Load(cin);
    const json::Node& root = input_document.GetRoot();
    const json::Dict& top_level_obj = root.AsDict();
    
    // Заполнить справочник
    const json::Node& base_req_node = top_level_obj.at("base_requests"s);
    json_reader.PopulateCatalogue(base_req_node);

    // Прочитать настройки рендера (если есть)
    if (auto it = top_level_obj.find("render_settings"s); it != top_level_obj.end()) {
        json_reader.ReadRenderSettings(it->second);
    }
    
    // Прочитать настройки маршрутизации (если есть)
    if (auto it = top_level_obj.find("routing_settings"s); it != top_level_obj.end()) {
        json_reader.ReadRoutingSettings(it->second);
    }

    // Запросить данные из справочника
    const json::Node& stat_req_node = top_level_obj.at("stat_requests"s);
    json::Node answers = json_reader.ProcessStatRequests(stat_req_node);
    const json::Document output_document{answers};
    
    // Вывести ответный json::Document в cout
    json::Print(output_document, cout);

    return 0;
}