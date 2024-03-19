#pragma once

#include "domain.h"
#include "graph.h"
#include "router.h"

#include <memory>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace tcat { class TransportCatalogue; }

namespace trouter {

struct WaitItem {
    // Название остановки
    std::string_view stop_name;
    // Время ожидания автобуса на остановке
    double time = 0.0;
};

struct BusItem {
    // Название автобуса
    std::string_view bus_name;
    // Количество остановок
    int span_count = 0;
    // Время поездки на автобусе
    double time = 0.0;
};

struct RouteInfo {
    using Item = std::variant<WaitItem, BusItem>;
    // Суммарное время, в минутах
    double total_time = 0.0;
    // Список элементов маршрута
    std::vector<Item> parts;
};  

class TransportRouter {
public:
    struct RoutingSettings {
        // Время ожидания автобуса на остановке, в минутах
        double bus_wait_time = 0.0;
        // Скорость автобуса, в км/ч
        double bus_velocity = 1.0;
    };

    void SetRoutingSettings(const RoutingSettings& settings);

    std::optional<RouteInfo> BuildRoute(const tcat::Stop* from, const tcat::Stop* to) const;

    void InitRouter(const tcat::TransportCatalogue& db);

    bool IsRouterInitialized() const;

private:
    RoutingSettings settings_;
    
    struct EdgeData {
        int span_count; // 0 - остановка, 1... - промежутки пройденные на автобусе
        std::string_view name; // название остановки или автобуса
        double time;
    };

    std::unordered_map<const tcat::Stop*, graph::VertexId> stop_ptr_to_vertex_id_;
    std::unordered_map<graph::EdgeId, EdgeData> edge_id_to_data;

    std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_ptr_;
    std::unique_ptr<graph::Router<double>> router_ptr_;
};

}