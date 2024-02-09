#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

int main() {
    tcat::TransportCatalogue catalogue;
    renderer::MapRenderer map_renderer;
    const handler::RequestHandler request_handler(catalogue, map_renderer);
    JsonReader json_reader(catalogue, map_renderer, request_handler);

    // Прочитать json::Document из cin
    const json::Document input_document = json::Load(cin);
    const json::Node& root = input_document.GetRoot();
    const json::Dict& top_level_obj = root.AsMap();
    
    // Заполнить справочник
    const json::Node& base_req_node = top_level_obj.at("base_requests"s);
    json_reader.PopulateCatalogue(base_req_node);

    // Прочитать настройки рендера
    const json::Node& render_settings_node = top_level_obj.at("render_settings"s);
    json_reader.ReadRenderSettings(render_settings_node);

    // Запросить данные из справочника
    const json::Node& stat_req_node = top_level_obj.at("stat_requests"s);
    json::Array answers = json_reader.ProcessStatRequests(stat_req_node);
    const json::Document output_document{answers};
    
    // Вывести ответный json::Document в cout
    json::Print(output_document, cout);
}