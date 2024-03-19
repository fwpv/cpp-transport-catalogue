#include "transport_catalogue.h"
#include "transport_router.h"

#include <stdexcept>

using namespace std::literals;

namespace trouter {

void TransportRouter::SetRoutingSettings(const RoutingSettings& settings) {
    if (settings.bus_velocity <= 0.0) {
        throw std::runtime_error("Incorrect bus velocity"s);
    }
    if (settings.bus_wait_time < 0.0) {
        throw std::runtime_error("Incorrect bus wait time"s);
    }
    settings_ = settings;
}

std::optional<RouteInfo> TransportRouter::BuildRoute(const tcat::Stop* from,
        const tcat::Stop* to) const {
    if (!IsRouterInitialized()) {
        throw std::runtime_error("Router is not initialized"s);
    }
    graph::VertexId start_vertex_of_from = stop_ptr_to_vertex_id_.at(from);
    graph::VertexId start_vertex_of_to = stop_ptr_to_vertex_id_.at(to);

    auto route_info = router_ptr_->BuildRoute(start_vertex_of_from, start_vertex_of_to);

    if (route_info) {
        RouteInfo result;
        result.total_time = route_info->weight;
        for (graph::EdgeId edge_id : route_info->edges) {
            const EdgeData& data = edge_id_to_data.at(edge_id);
            if (data.span_count == 0) {
                WaitItem item;
                item.stop_name = data.name;
                item.time = data.time;
                result.parts.emplace_back(item);
            } else {
                BusItem item;
                item.bus_name = data.name;
                item.span_count = data.span_count;
                item.time = data.time;
                result.parts.emplace_back(item);
            }
        }
        return result;
    } else {
        return std::nullopt;
    }
}

void TransportRouter::InitRouter(const tcat::TransportCatalogue& db) {
    std::vector<const tcat::Stop*> stops = db.GetAllStops();

    // Вершин графа в 2 раза больше чем остановок
    // Чётные вершины -> начало ожидания автобуса
    // Нечётные вершины -> конец ожидания автобуса
    size_t vertex_count = stops.size() * 2;
    graph_ptr_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_count);

    for(size_t i = 0; i < stops.size(); ++i) {
        // Сформировать словарь для поиска вершин остановок
        const tcat::Stop* stop = stops[i];
        stop_ptr_to_vertex_id_[stop] = i * 2;

        // Добавить рёбра ожидания автобуса на остановке
        graph::VertexId start_vertex = i * 2;
        graph::VertexId end_vertex = i * 2 + 1;
        graph::Edge<double> edge{start_vertex, end_vertex, settings_.bus_wait_time};
        graph::EdgeId edge_id = graph_ptr_->AddEdge(edge);
        edge_id_to_data.emplace(edge_id, EdgeData{0, stop->name, settings_.bus_wait_time});
    }
    
    std::vector<const tcat::Bus*> buses = db.GetAllBuses();
    for (const tcat::Bus* bus : buses) {
        // Если маршрут не кольцевой, сделать его кольцевым для упрощения расчётов
        std::vector<const tcat::Stop*> rounded_stops = bus->stops;
        if (!bus->is_roundtrip) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                rounded_stops.push_back(*it);
            }
        }

        for (size_t from = 0; from < rounded_stops.size() - 1; ++from) {
            int distance = 0;
            for (size_t to = from + 1; to < rounded_stops.size(); ++to) {
                if (rounded_stops[from] == rounded_stops[to]) {
                    continue;
                }

                int span_count = static_cast<int>(to - from);

                // Определить время поездки на автобусе
                distance += db.GetDistance(rounded_stops[to - 1], rounded_stops[to]);
                double weight = distance / 1000.0 / settings_.bus_velocity * 60.0;

                //Добавить рёбро поездки на автобус
                graph::VertexId end_vertex_of_from = stop_ptr_to_vertex_id_.at(rounded_stops[from]) + 1;
                graph::VertexId start_vertex_of_to = stop_ptr_to_vertex_id_.at(rounded_stops[to]);
                graph::Edge<double> edge{end_vertex_of_from, start_vertex_of_to, weight};
                graph::EdgeId edge_id = graph_ptr_->AddEdge(edge);
                edge_id_to_data.emplace(edge_id, EdgeData{span_count, bus->name, weight});
            }
        }
    }

    router_ptr_ = std::make_unique<graph::Router<double>>(*graph_ptr_);
}

bool TransportRouter::IsRouterInitialized() const {
    return router_ptr_.get() != nullptr;
}

}