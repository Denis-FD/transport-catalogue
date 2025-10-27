#include <iostream>

#include "transport_catalogue.h"
#include "json_reader.h"

int main() {
    using namespace transport_catalogue;

    TransportCatalogue catalogue;
    JsonReader json_reader(catalogue);

    try {
        json_reader.ParseInput(std::cin);
        json_reader.PrintOutput(std::cout);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
