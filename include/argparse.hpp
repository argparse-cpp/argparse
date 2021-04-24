//
// Created by aviallon on 22/04/2021.
//

#ifndef ARGPARSE_HPP
#define ARGPARSE_HPP

#include <iostream>
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
#include <ranges>
#include <vector>

namespace argparse {

    inline bool to_bool(const ::std::string& str) {
        return (str == "true" || str == "t" || str == "True");
    }
    inline bool to_bool(const char* str) {
        return to_bool(::std::string(str));
    }

    template<typename T>
    class simple_stack {

    public:
        void push(T element) {
            stack.push(element);
        }

        ::std::optional<T> pop() {
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
        ::std::stack<T> stack;
    };

    class argument {
    public:
        ::std::string name;
        ::std::string metavar;
        ::std::string value;
        ::std::string default_value;
        ::std::string store_value;
        ::std::string help;
        ::std::type_index type;

        bool flag = false;
        bool positional = false;

        explicit operator std::string() const {
            return value;
        }
        explicit operator int() const {
            return ::std::stoi(value);
        }
        explicit operator bool() const {
            return to_bool(value);
        }
        explicit operator double() const {
            return ::std::stod(value);
        }

        void parse(const ::std::string& v) {
            if (type == typeid(::std::string)) {
                value = v;
            } else if (type == typeid(int)) {
                value = ::std::to_string(::std::stoi(v));
            } else if (type == typeid(double)) {
                value = ::std::to_string(::std::stod(v));
            } else if (type == typeid(bool)) {
                value = ::std::to_string(to_bool(value));
            }
        }
    };

    ::std::string to_string(const ::std::string& s){
        return s;
    }

    ::std::string to_string(const char* s) {
        return ::std::string(s);
    }

    template<typename T>
    ::std::string to_string(const T& v) {
        return ::std::to_string(v);
    }

    ::std::string to_string(nullptr_t s){
        return ::std::string("");
    }

    template<typename T>
    concept ElementIterable = ::std::ranges::range<::std::ranges::range_value_t<T>>;


    class argument_parser {
    public:
        argument_parser() = default;
        ::std::string program_name = "<unknown>";

        template <typename T>
        void add_argument(const ::std::string& name, const ::std::type_index& type, const ::std::string& help, T default_value, T store_value) {
            const bool positional = name[0] != '-';
            const bool flag = store_value != T() && !positional;
            arguments.insert_or_assign(name, argument {
                    .name = name,
                    .metavar = name,
                    .value = to_string(default_value),
                    .default_value = to_string(default_value),
                    .store_value = flag ? to_string(store_value) : ::std::string(""),
                    .help = help,
                    .type = type,
                    .flag = flag,
                    .positional = positional,
            });
        }

        template <typename T>
        void add_argument(const ::std::string& name, const ::std::type_index& type, const ::std::string& help, T default_value) {
            add_argument(name, type, help, default_value, T());
        }

        void show_arguments() {
            for (const auto& argument : arguments) {
                ::std::cout << argument.first << ": value=" << argument.second.value << std::endl;
            }
        }

        void show_help() {
            simple_stack<argument> positional_arguments;
            simple_stack<argument> standard_arguments;
            ::std::stringstream ss;
            ss << "Usage: " << program_name << " [OPTIONS]";
            for (const auto& argument : arguments) {
                if (argument.second.positional)
                    positional_arguments.push(argument.second);
                else
                    standard_arguments.push(argument.second);
            }
            ::std::optional<argument> argument;
            while (argument = positional_arguments.pop(), argument.has_value()) {
                ss << " " << argument.value().name;
            }
            ss << ::std::endl << ::std::endl << "Options:" << ::std::endl;

            while (argument = standard_arguments.pop(), argument.has_value()) {
                ss << "\t" << argument.value().name << "\t\t" << argument.value().help << ::std::endl;
            }

            ::std::cout << ss.str() << ::std::flush;
        }

        void parse_args(::std::vector<::std::string> args) {

            program_name = *args.begin();

            simple_stack<::std::string> positionals;

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
                    argument& argument = arguments.at(*arg);
                    if (argument.flag) {
                        argument.value = argument.store_value;
                    } else {
                        if (arg != args.end() - 1) {
                            argument.parse(*(arg + 1));
                            arg++;
                        } else {
                            ::std::stringstream ss;
                            ss << "Invalid argument: " << *arg << " requires a parameter";
                            throw ::std::invalid_argument(ss.str());
                        }
                    }
                }
            }

            ::std::optional<::std::string> arg;
            while (arg = positionals.pop(), arg.has_value()) {
                ::std::cout << "Got positional " << arg.value();
            }
        }

        void parse_args(int argc, char** argv) {
            ::std::vector<::std::string> args(argc);
            for(int i=0; i<argc; i++) {
                args[i] = ::std::string(argv[i]);
            }

            parse_args(args);
        }

//        template<typename T>
//        T operator[](const char* name) {
//            return this->template operator[]<T>(::std::string(name));
//        }

        template<typename T>
        T at(const ::std::string& name) {
            if (arguments.contains(name)) {
                const argument& arg = arguments.at(name);
                if (arg.type == typeid(T)) {
                    return (T)arg;
                } else {
                    throw std::runtime_error("Invalid type to get argument value");
                }
            } else {
                throw std::range_error("Argument is not defined");
            }
        }

        template<typename T>
        T operator[](const ::std::string& name) {
            return at<T>(name);
        }
    private:
        ::std::unordered_map<::std::string, argument> arguments;
    };
}


#endif //ARGPARSE_HPP
