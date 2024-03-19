#pragma once

/*
 * Код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 */

#include "svg.h"
#include "transport_router.h"

#include <optional>
#include <set>
#include <string_view>

namespace tcat { class TransportCatalogue; }
namespace renderer { class MapRenderer; }
namespace trouter { class TransportRouter; }

namespace handler {

struct BusStat {
    double curvature = 0.0;
    int route_length = 0;
    int stop_count = 0;
    int unique_stop_count = 0;
};

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие
// JSON reader-а с другими подсистемами приложения
class RequestHandler {
public:
    RequestHandler(const tcat::TransportCatalogue& db,
            const renderer::MapRenderer& map_renderer,
            trouter::TransportRouter& transport_router);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(std::string_view bus_name) const;

    // Возвращает маршруты, проходящие через остановку (запрос Stop)
    const std::set<std::string_view>*
            GetBusNamesByStop(std::string_view stop_name) const;

    // Отобразить карту маршрутов в формате svg::Document (запрос Map)
    svg::Document RenderMap() const;

    // Построить маршрут (запрос Route)
    std::optional<trouter::RouteInfo> BuildRoute(std::string_view from, std::string_view to);

private:
    const tcat::TransportCatalogue& db_;
    const renderer::MapRenderer& map_renderer_;
    trouter::TransportRouter& transport_router_;
};

}