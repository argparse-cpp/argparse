#include "argparse.hpp"
#include <string>
#include <iostream>

int main(int argc, char** argv){
    argparse::argument_parser parser {};
    parser.add_argument("--output", typeid(std::string), "Output file path", "/dev/stdout");
    parser.add_argument("--integer", typeid(int), "Integer value only", 0);
    parser.parse_args(argc, argv);

    parser.show_arguments();

    std::string output = parser.at<std::string>("--output");
    bool assertion = true;
    assertion &= output == "test_successful";
    assertion &= parser.at<int>("--integer") == 42;

    bool type_checking = false;
    try {
        std::cout << "Trying to display --integer as bool: " << parser.at<bool>("--integer") << std::endl;
    } catch (std::exception) {
        type_checking = true;
        std::cerr << "Yep, got type checking runtime_error";
    }

    assertion &= type_checking;


    return assertion ? EXIT_SUCCESS : EXIT_FAILURE;
}