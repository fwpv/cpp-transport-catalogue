#include "input_reader.h"

#include "geo.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace tcat {

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит строку вида "55.61, 37.20, 3900m to StopName, ..." и возвращает параметры остановки
 */
StopParameters ParseStopParameters(std::string_view str) {
    using namespace std::literals;
    StopParameters result;
    std::vector<std::string_view> parameters = Split(str, ',');

    static const double nan = std::nan("");
    if (parameters.size() < 2) {
        result.coordinates.lat = nan;
        result.coordinates.lng = nan;
    } else {
        result.coordinates.lat = std::stod(std::string(parameters[0]));
        result.coordinates.lng = std::stod(std::string(parameters[1]));
    }

    if (parameters.size() > 2) {
        for (auto it = parameters.begin() + 2; it != parameters.end(); ++it) {
            std::string_view str = *it;
            size_t to_pos = str.find("to"s);
            if (to_pos == str.npos) {
                break;
            }
  
            std::string_view distance_str = Trim(str.substr(0, to_pos));
            distance_str.remove_suffix(1);
            std::string_view stop_name_str = Trim(str.substr(to_pos + 2));

            StopParameters::DisctanceTo dito;
            dito.distance = std::stoi(std::string(distance_str));
            dito.stop_name = stop_name_str;
            result.distances.push_back(dito);
        }
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    CommandDescription cd {
        std::string(line.substr(0, space_pos)),
        std::string(line.substr(not_space, colon_pos - not_space)),
        std::string(line.substr(colon_pos + 1))
    };

    return cd;
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    // 1. Обработать команды на добавление остановок
    // 1.1 Добавить остановки и их координаты
    std::vector<std::pair<std::string_view, StopParameters>> parameter_cache;
    for (const CommandDescription& command : commands_) {
        if (command.command == "Stop") {
            StopParameters parameters = ParseStopParameters(command.description);
            catalogue.AddStop(command.id, parameters.coordinates);
            parameter_cache.emplace_back(command.id, std::move(parameters));
        }
    }
    // 1.1 Добавить расстояния между остановками
    for (const auto& [id, parameters] : parameter_cache) {
        for (const StopParameters::DisctanceTo& dito : parameters.distances) {
            catalogue.AddDistance(id, dito.stop_name, dito.distance);
        }
    }

    // 2. Обработать команды на добавление маршрутов
    for (const CommandDescription& command : commands_) {
        if (command.command == "Bus") {
            std::vector<std::string_view> route = ParseRoute(command.description);
            catalogue.AddBus(command.id, route);
        }
    }
}

namespace tests {

void ParseLine() {
    using namespace std::literals; 
    InputReader reader;
    reader.ParseLine("Stop Marushkino: 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino"sv);
    reader.ParseLine("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"sv);
    
    const std::vector<CommandDescription>& commands = reader.GetCommands();
    assert(commands.size() == 2);

    const CommandDescription& command1 = commands[0];
    assert(command1.command == "Stop"s);
    assert(command1.id == "Marushkino"s);
    assert(command1.description == " 55.595884, 37.209755, 9900m to Rasskazovka, 100m to Marushkino"s);

    StopParameters parameters = ParseStopParameters(command1.description);
    assert(parameters.coordinates.lat == 55.595884);
    assert(parameters.coordinates.lng == 37.209755);
    assert(parameters.distances.size() == 2);
    assert(parameters.distances[0].distance == 9900);
    assert(parameters.distances[0].stop_name == "Rasskazovka"s);
    assert(parameters.distances[1].distance == 100);
    assert(parameters.distances[1].stop_name == "Marushkino"s);

    const CommandDescription& command2 = commands[1];
    assert(command2.command == "Bus"s);
    assert(command2.id == "750"s);
    assert(command2.description == " Tolstopaltsevo - Marushkino - Rasskazovka"s);

    std::vector<std::string_view> route = ParseRoute(command2.description);
    assert(route.size() == 5);
    assert(route[0] == "Tolstopaltsevo");
    assert(route[1] == "Marushkino");
    assert(route[2] == "Rasskazovka");
    assert(route[3] == "Marushkino");
    assert(route[4] == "Tolstopaltsevo");
}

}
}