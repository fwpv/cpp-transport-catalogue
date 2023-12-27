#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;

template <typename TestFunc>
void RunTestImpl(const TestFunc& func, const string& test_name) {
    func();
    cerr << test_name << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

void RunAllTests() {
    RUN_TEST(tcat::tests::ParseLine);
    RUN_TEST(tcat::tests::GettingBusInfo);
    RUN_TEST(tcat::tests::GettingStopInfo);
    RUN_TEST(tcat::tests::ParseRequest);

    cerr << "All test passed!"s << std::endl;
}

int main() {
    // Тесты мешают пройти проверку тренажёра
    // RunAllTests();
    // return 0;
    
    tcat::TransportCatalogue catalogue;

    int base_request_count;
    cin >> base_request_count >> ws;

    {
        tcat::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        tcat::ParseAndPrintStat(catalogue, line, cout);
    }

    return 0;
}