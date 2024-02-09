#pragma once

#include "json.h"

/*
 * Код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

// forward declarations
namespace tcat { class TransportCatalogue; }
namespace renderer { class MapRenderer; }
namespace handler { class RequestHandler; }

class JsonReader {
public:
    JsonReader(tcat::TransportCatalogue& db, renderer::MapRenderer& map_renderer,
            const handler::RequestHandler& handler);

    // Наполнить справочник данными из JSON-ноды
    void PopulateCatalogue(const json::Node& base_req_node);

    // Обработать запросы к базе и сформировать ответ в формате JSON-массива
    json::Array ProcessStatRequests(const json::Node& stat_req_node) const;

    // Прочитать настройки map_renderer из JSON-ноды
    void ReadRenderSettings(const json::Node& render_settings_node);

private:
    tcat::TransportCatalogue& db_;
    renderer::MapRenderer& map_renderer_;
    const handler::RequestHandler& handler_; 
};