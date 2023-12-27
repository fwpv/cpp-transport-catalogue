#include "stat_reader.h"

#include <cassert>
#include <iostream>

namespace tcat {

std::pair<std::string_view, std::string_view> ParseCommand(std::string_view line) {
    auto space_pos = line.find(' ');
    if (space_pos == line.npos) {
        return {};
    }

    return {line.substr(0, space_pos), line.substr(space_pos + 1)};
}

void ParseAndPrintStat(const TransportCatalogue& tansport_catalogue, std::string_view request,
                       std::ostream& output) {
    using namespace std::literals;
    auto [command, id] = ParseCommand(request);
    if (command == "Bus"sv) {
        output << tansport_catalogue.BusInfo(id) << std::endl;
    }
    if (command == "Stop"sv) {
        output << tansport_catalogue.StopInfo(id) << std::endl;
    }
}

namespace tests {

void ParseRequest() {
    using namespace std::literals;

    auto [command1, id1] = ParseCommand("Bus 256"sv);
    assert(command1 == "Bus"sv);
    assert(id1 == "256"sv);

    auto [command2, id2] = ParseCommand("Stop Samara"sv);
    assert(command2 == "Stop"sv);
    assert(id2 == "Samara"sv);
}

}
}