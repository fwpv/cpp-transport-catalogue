#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "transport_catalogue.h"

namespace tcat {

struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

struct StopParameters {
    geo::Coordinates coordinates;
    struct DisctanceTo {
        int distance;
        std::string stop_name;
    };
    std::vector<DisctanceTo> distances;
};

class InputReader {
public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(TransportCatalogue& catalogue) const;

    /**
     * Получает сохранённые команды
    */
    const std::vector<CommandDescription>& GetCommands() const {
        return commands_;
    }

private:
    std::vector<CommandDescription> commands_;
};

namespace tests {

void ParseLine();

}
}