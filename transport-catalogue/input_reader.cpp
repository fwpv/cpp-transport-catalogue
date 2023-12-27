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

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands([[maybe_unused]] TransportCatalogue& catalogue) const {
    // 1. Обработать команды на добавление остановок
    geo::Coordinates coordinates;
    for (const CommandDescription& command : commands_) {
        if (command.command == "Stop") {
            coordinates = ParseCoordinates(command.description);
            catalogue.AddStop(command.id, coordinates);
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
    reader.ParseLine("Stop Tolstopaltsevo: 55.611087, 37.208290"sv);
    reader.ParseLine("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"sv);
    
    const std::vector<CommandDescription>& commands = reader.GetCommands();
    assert(commands.size() == 2);

    const CommandDescription& command1 = commands[0];
    assert(command1.command == "Stop"s);
    assert(command1.id == "Tolstopaltsevo"s);
    assert(command1.description == " 55.611087, 37.208290"s);

    geo::Coordinates coordinates = ParseCoordinates(command1.description);
    assert(coordinates.lat == 55.611087);
    assert(coordinates.lng == 37.208290);

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