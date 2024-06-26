#include "domain.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

namespace handler {

RequestHandler::RequestHandler(const tcat::TransportCatalogue& db,
        const renderer::MapRenderer& map_renderer,
        trouter::TransportRouter& transport_router)
    : db_(db)
    , map_renderer_(map_renderer)
    , transport_router_(transport_router) {
}

std::optional<BusStat>
        RequestHandler::GetBusStat(std::string_view bus_name) const {
    const tcat::Bus* bus = db_.FindBus(bus_name);
    if (bus) {
        BusStat result;
        result.stop_count = bus->StopCount();
        result.unique_stop_count = bus->CountUniqueStops();
        result.route_length = db_.CalculateRouteLength(bus);
        double geo_route_length = db_.CalculateGeoRouteLength(bus);
        result.curvature = result.route_length / geo_route_length;
        return result;
    } else {
        return std::nullopt;
    }
}

const std::set<std::string_view>* 
        RequestHandler::GetBusNamesByStop(std::string_view stop_name) const {
    const tcat::Stop* stop = db_.FindStop(stop_name);
    if (!stop) {
        return nullptr;
    }
    const std::set<std::string_view>* bus_set_ptr = db_.GetBusNamesByStop(stop);
    if (!bus_set_ptr) {
        static std::set<std::string_view> empty_set{};
        return &empty_set;
    }
    return bus_set_ptr;
}

svg::Document RequestHandler::RenderMap() const {
    // Запросить из базы информацию обо всех автобусах
    std::vector<const tcat::Bus*> buses = db_.GetAllBuses();
    std::sort(buses.begin(), buses.end(), 
        [](const tcat::Bus* a, const tcat::Bus* b) { 
            return a->name < b->name; 
        });

    // Отобразить карту маршрутов в формате svg::Document
    return map_renderer_.RenderMap(buses);
}

std::optional<trouter::RouteInfo>
        RequestHandler::BuildRoute(std::string_view from, std::string_view to) {
    if (!transport_router_.IsRouterInitialized()) {
        transport_router_.InitRouter(db_);
    }
    const tcat::Stop* from_stop = db_.FindStop(from);
    const tcat::Stop* to_stop = db_.FindStop(to);

    return transport_router_.BuildRoute(from_stop, to_stop);
}
}