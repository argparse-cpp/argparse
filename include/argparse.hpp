//
// Created by aviallon on 22/04/2021.
//

#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <unordered_map>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <typeindex>
#include <exception>
#include <string>
#include <sstream>
#include <stack>
#include <iterator>
#include <optional>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

inline bool to_bool(const std::string& str) {
    return (str == "true" || str == "t" || str == "True");
}
inline bool to_bool(const char* str) {
    return to_bool(std::string(str));
}

template<typename T>
class simple_stack {

public:
    void push(T element) {
        stack.push(element);
    }

    std::optional<T> pop() {
        if(stack.empty())
            return {};
        T element = stack.top();
        stack.pop();
        return element;
    }

    [[nodiscard]] bool empty() const {
        return stack.empty();
    }

private:
    std::stack<T> stack;
};

struct Argument {
public:
    std::string name;
    std::string metavar;
    std::string value;
    std::string default_value;
    std::string store_value;
    std::string help;
    std::type_index type;

    bool flag = false;
    bool positional = false;

    template <typename T>
    explicit operator T() {
        if (typeid(T) == typeid(std::string)) {
            return value;
        } else if (typeid(T) == typeid(int)) {
            return std::stoi(value);
        } else if (typeid(T) == typeid(double)) {
            return std::stod(value);
        } else if (typeid(T) == typeid(bool)) {
            return to_bool(value);
        } else {
            throw std::bad_typeid();
        }
    }

    void parse(const std::string& v) {
        if (type == typeid(std::string)) {
            value = v;
        } else if (type == typeid(int)) {
            value = std::to_string(std::stoi(v));
        } else if (type == typeid(double)) {
            value = std::to_string(std::stod(v));
        } else if (type == typeid(bool)) {
            value = std::to_string(to_bool(value));
        }
    }
};

namespace std {
    string to_string(const string& s) {
        return s;
    }

    string to_string(nullptr_t s){
        return std::string("");
    }
}

#include <ranges>
template<typename T>
concept ElementIterable = std::ranges::range<std::ranges::range_value_t<T>>;


class Argparse {
public:
    Argparse() = default;
    std::string program_name = "<unknown>";

    template <typename T>
    void add_argument(const std::string& name, const std::type_index& type, const std::string& help, T default_value, T store_value) {
        const bool positional = name[0] != '-';
        const bool flag = store_value != T() && !positional;
        arguments.insert_or_assign(name, Argument {
            .name = name,
            .metavar = name,
            .value = std::to_string(default_value),
            .default_value = std::to_string(default_value),
            .store_value = flag ? std::to_string(store_value) : std::string(""),
            .help = help,
            .type = type,
            .flag = flag,
            .positional = positional,
        });
    }

    template <typename T>
    void add_argument(const std::string& name, const std::type_index& type, const std::string& help, T default_value) {
        add_argument(name, type, help, default_value, T());
    }

    void show_arguments() {
        for (const auto& argument : arguments) {
            spdlog::debug("{}: value={}",
                          argument.first,
                          argument.second.value);
        }
    }

    void show_help() {
        simple_stack<Argument> positional_arguments;
        simple_stack<Argument> standard_arguments;
        std::stringstream ss;
        ss << "Usage: " << program_name << " [OPTIONS]";
        for (const auto& argument : arguments) {
            if (argument.second.positional)
                positional_arguments.push(argument.second);
            else
                standard_arguments.push(argument.second);
        }
        std::optional<Argument> argument;
        while (argument = positional_arguments.pop(), argument.has_value()) {
            ss << " " << argument.value().name;
        }
        ss << std::endl << std::endl << "Options:" << std::endl;

        while (argument = standard_arguments.pop(), argument.has_value()) {
            ss << "\t" << argument.value().name << "\t\t" << argument.value().help << std::endl;
        }

        std::cout << ss.str() << std::flush;
    }

    void parse_args(std::vector<std::string> args) {

        program_name = *args.begin();

        simple_stack<std::string> positionals;

        for (auto arg = args.begin()+1; arg != args.end(); arg++) {
            if ((*arg)[0] != '-') {
                positionals.push(*arg);
                continue;
            }
            if (*arg == "--help" || *arg == "-h") {
                show_help();
                return;
            }
            if (arguments.contains(*arg)) {
                Argument& argument = arguments.at(*arg);
                if (argument.flag) {
                    argument.value = argument.store_value;
                } else {
                    if (arg != args.end() - 1) {
                        argument.parse(*(arg + 1));
                        arg++;
                    } else {
                        throw std::invalid_argument(fmt::format("Invalid argument: {} requires a parameter", *arg));
                    }
                }
            }
        }

        std::optional<std::string> arg;
        while (arg = positionals.pop(), arg.has_value()) {
            spdlog::debug("Got positional {}", arg.value());
        }
    }

    void parse_args(int argc, char** argv) {
        std::vector<std::string> args(argc);
        for(int i=0; i<argc; i++) {
            args[i] = std::string(argv[i]);
        }

        parse_args(args);
    }
private:
    std::unordered_map<std::string, Argument> arguments;
};

#endif //ARGPARSE_HPP
