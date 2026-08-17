/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <vector>

using namespace mango;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    CommandLine make_commands(std::initializer_list<const char*> args, std::vector<std::string>& storage)
    {
        storage.clear();
        storage.reserve(args.size());

        for (const char* arg : args)
        {
            storage.emplace_back(arg);
        }

        CommandLine commands;
        commands.reserve(storage.size());

        for (const std::string& arg : storage)
        {
            commands.push_back(arg);
        }

        return commands;
    }

    bool test_flag_and_positional()
    {
        std::vector<std::string> storage;
        bool verbose = false;
        std::string filename;

        CommandLineParser parser;
        parser.flag("--verbose", [&]() { verbose = true; });
        parser.positional([&](std::string_view token) { filename = token; });

        CommandLine commands = make_commands({ "tool", "--verbose", "image.png" }, storage);
        CHECK(parser.parse(commands));
        CHECK(verbose);
        CHECK(filename == "image.png");
        CHECK(parser.positionals().size() == 1);
        CHECK(parser.positionals()[0] == "image.png");

        return true;
    }

    bool test_option_separate_value()
    {
        std::vector<std::string> storage;
        int level = 0;

        CommandLineParser parser;
        parser.option("--level", [&](std::string_view value) { level = std::atoi(value.data()); });

        CommandLine commands = make_commands({ "tool", "--level", "9" }, storage);
        CHECK(parser.parse(commands));
        CHECK(level == 9);

        return true;
    }

    bool test_option_inline_value()
    {
        std::vector<std::string> storage;
        std::string format;

        CommandLineParser parser;
        parser.option("--format", [&](std::string_view value) { format = value; });

        CommandLine commands = make_commands({ "tool", "--format=png" }, storage);
        CHECK(parser.parse(commands));
        CHECK(format == "png");

        return true;
    }

    bool test_double_dash()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;
        parser.positional([&](std::string_view) {});

        CommandLine commands = make_commands({ "tool", "--", "--not-an-option", "-file.png" }, storage);
        CHECK(parser.parse(commands));
        CHECK(parser.positionals().size() == 2);
        CHECK(parser.positionals()[0] == "--not-an-option");
        CHECK(parser.positionals()[1] == "-file.png");

        return true;
    }

    bool test_lone_dash_is_positional()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;
        parser.positional([&](std::string_view) {});

        CommandLine commands = make_commands({ "tool", "-" }, storage);
        CHECK(parser.parse(commands));
        CHECK(parser.positionals().size() == 1);
        CHECK(parser.positionals()[0] == "-");

        return true;
    }

    bool test_unknown_short_option()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;

        CommandLine commands = make_commands({ "tool", "-v" }, storage);
        CHECK(!parser.parse(commands));

        return true;
    }

    bool test_unknown_long_option()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;

        CommandLine commands = make_commands({ "tool", "--typo" }, storage);
        CHECK(!parser.parse(commands));

        return true;
    }

    bool test_missing_option_value()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;
        parser.option("--level", [&](std::string_view) {});

        CommandLine commands = make_commands({ "tool", "--level" }, storage);
        CHECK(!parser.parse(commands));

        return true;
    }

    bool test_help_returns_false()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLineParser parser;
        parser.flag("--verbose", [&]() { called = true; });

        CommandLine commands = make_commands({ "tool", "--help" }, storage);
        CHECK(!parser.parse(commands));
        CHECK(!called);

        return true;
    }

    bool test_required_positional()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;
        parser.requirePositional("filename");
        parser.positional([&](std::string_view) {});

        CommandLine commands = make_commands({ "tool" }, storage);
        CHECK(!parser.parse(commands));

        commands = make_commands({ "tool", "input.png" }, storage);
        CHECK(parser.parse(commands));
        CHECK(parser.positionals().size() == 1);

        return true;
    }

    bool test_flag_with_inline_value_rejected()
    {
        std::vector<std::string> storage;

        CommandLineParser parser;
        parser.flag("--verbose", []() {});

        CommandLine commands = make_commands({ "tool", "--verbose=yes" }, storage);
        CHECK(!parser.parse(commands));

        return true;
    }

    bool test_option2D_inline_value()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLineParser parser;
        parser.option2D("--astc", [&](int w, int h) { width = w; height = h; });

        CommandLine commands = make_commands({ "tool", "--astc=5x5" }, storage);
        CHECK(parser.parse(commands));
        CHECK(width == 5);
        CHECK(height == 5);

        return true;
    }

    bool test_option2D_separate_value()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLineParser parser;
        parser.option2D("--astc", [&](int w, int h) { width = w; height = h; });

        CommandLine commands = make_commands({ "tool", "--astc", "8X8" }, storage);
        CHECK(parser.parse(commands));
        CHECK(width == 8);
        CHECK(height == 8);

        return true;
    }

    bool test_option2D_comma_separator()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLineParser parser;
        parser.option2D("--size", [&](int w, int h) { width = w; height = h; });

        CommandLine commands = make_commands({ "tool", "--size=12,10" }, storage);
        CHECK(parser.parse(commands));
        CHECK(width == 12);
        CHECK(height == 10);

        return true;
    }

    bool test_option2D_invalid_value()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLineParser parser;
        parser.option2D("--astc", [&](int, int) { called = true; });

        CommandLine commands = make_commands({ "tool", "--astc=bad" }, storage);
        CHECK(!parser.parse(commands));
        CHECK(!called);

        return true;
    }

    const Case g_cases [] =
    {
        { "flag and positional",           test_flag_and_positional },
        { "option separate value",         test_option_separate_value },
        { "option inline value",           test_option_inline_value },
        { "double dash",                   test_double_dash },
        { "lone dash positional",          test_lone_dash_is_positional },
        { "unknown short option",          test_unknown_short_option },
        { "unknown long option",           test_unknown_long_option },
        { "missing option value",          test_missing_option_value },
        { "help returns false",            test_help_returns_false },
        { "required positional",           test_required_positional },
        { "flag inline value rejected",    test_flag_with_inline_value_rejected },
        { "option2D inline value",         test_option2D_inline_value },
        { "option2D separate value",       test_option2D_separate_value },
        { "option2D comma separator",      test_option2D_comma_separator },
        { "option2D invalid value",        test_option2D_invalid_value },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_commandline", g_cases, std::size(g_cases), argc, argv);
}
