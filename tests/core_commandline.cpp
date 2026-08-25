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

        return CommandLine(storage);
    }

    bool test_flag_and_positional()
    {
        std::vector<std::string> storage;
        bool verbose = false;
        std::string filename;

        CommandLine commands = make_commands({ "tool", "--verbose", "image.png" }, storage);
        commands.flag("--verbose", [&]() { verbose = true; });
        commands.positional([&](std::string_view token) { filename = token; });

        CHECK(commands.parse());
        CHECK(verbose);
        CHECK(filename == "image.png");
        CHECK(commands.positionals().size() == 1);
        CHECK(commands.positionals()[0] == "image.png");

        return true;
    }

    bool test_option_separate_value()
    {
        std::vector<std::string> storage;
        int level = 0;

        CommandLine commands = make_commands({ "tool", "--level", "9" }, storage);
        commands.optionInt("--level", [&](int value) { level = value; });

        CHECK(commands.parse());
        CHECK(level == 9);

        return true;
    }

    bool test_option_inline_value()
    {
        std::vector<std::string> storage;
        std::string format;

        CommandLine commands = make_commands({ "tool", "--format=png" }, storage);
        commands.option("--format", [&](std::string_view value) { format = value; });

        CHECK(commands.parse());
        CHECK(format == "png");

        return true;
    }

    bool test_double_dash()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "--", "--not-an-option", "-file.png" }, storage);
        commands.positional([&](std::string_view) {});

        CHECK(commands.parse());
        CHECK(commands.positionals().size() == 2);
        CHECK(commands.positionals()[0] == "--not-an-option");
        CHECK(commands.positionals()[1] == "-file.png");

        return true;
    }

    bool test_lone_dash_is_positional()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "-" }, storage);
        commands.positional([&](std::string_view) {});

        CHECK(commands.parse());
        CHECK(commands.positionals().size() == 1);
        CHECK(commands.positionals()[0] == "-");

        return true;
    }

    bool test_unknown_short_option()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "-v" }, storage);
        CHECK(!commands.parse());

        return true;
    }

    bool test_unknown_long_option()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "--typo" }, storage);
        CHECK(!commands.parse());

        return true;
    }

    bool test_missing_option_value()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "--level" }, storage);
        commands.option("--level", [&](std::string_view) {});

        CHECK(!commands.parse());

        return true;
    }

    bool test_help_returns_false()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLine commands = make_commands({ "tool", "--help" }, storage);
        commands.flag("--verbose", [&]() { called = true; });

        CHECK(!commands.parse());
        CHECK(!called);

        return true;
    }

    bool test_required_positional()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool" }, storage);
        commands.requirePositional("filename");
        commands.positional([&](std::string_view) {});

        CHECK(!commands.parse());

        commands = make_commands({ "tool", "input.png" }, storage);
        commands.requirePositional("filename");
        commands.positional([&](std::string_view) {});

        CHECK(commands.parse());
        CHECK(commands.positionals().size() == 1);

        return true;
    }

    bool test_flag_with_inline_value_rejected()
    {
        std::vector<std::string> storage;

        CommandLine commands = make_commands({ "tool", "--verbose=yes" }, storage);
        commands.flag("--verbose", []() {});

        CHECK(!commands.parse());

        return true;
    }

    bool test_option2D_inline_value()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLine commands = make_commands({ "tool", "--astc=5x5" }, storage);
        commands.option2D("--astc", [&](int w, int h) { width = w; height = h; });

        CHECK(commands.parse());
        CHECK(width == 5);
        CHECK(height == 5);

        return true;
    }

    bool test_option2D_separate_value()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLine commands = make_commands({ "tool", "--astc", "8X8" }, storage);
        commands.option2D("--astc", [&](int w, int h) { width = w; height = h; });

        CHECK(commands.parse());
        CHECK(width == 8);
        CHECK(height == 8);

        return true;
    }

    bool test_option2D_comma_separator()
    {
        std::vector<std::string> storage;
        int width = 0;
        int height = 0;

        CommandLine commands = make_commands({ "tool", "--size=12,10" }, storage);
        commands.option2D("--size", [&](int w, int h) { width = w; height = h; });

        CHECK(commands.parse());
        CHECK(width == 12);
        CHECK(height == 10);

        return true;
    }

    bool test_option2D_invalid_value()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLine commands = make_commands({ "tool", "--astc=bad" }, storage);
        commands.option2D("--astc", [&](int, int) { called = true; });

        CHECK(!commands.parse());
        CHECK(!called);

        return true;
    }

    bool test_optionInt_inline_value()
    {
        std::vector<std::string> storage;
        int level = 0;

        CommandLine commands = make_commands({ "tool", "--level=9" }, storage);
        commands.optionInt("--level", [&](int value) { level = value; });

        CHECK(commands.parse());
        CHECK(level == 9);

        return true;
    }

    bool test_optionInt_invalid_value()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLine commands = make_commands({ "tool", "--level=fast" }, storage);
        commands.optionInt("--level", [&](int) { called = true; });

        CHECK(!commands.parse());
        CHECK(!called);

        return true;
    }

    bool test_optionFloat_inline_value()
    {
        std::vector<std::string> storage;
        float quality = 0.0f;

        CommandLine commands = make_commands({ "tool", "--quality=0.85" }, storage);
        commands.optionFloat("--quality", [&](float value) { quality = value; });

        CHECK(commands.parse());
        CHECK(quality > 0.84f && quality < 0.86f);

        return true;
    }

    bool test_optionFloat_invalid_value()
    {
        std::vector<std::string> storage;
        bool called = false;

        CommandLine commands = make_commands({ "tool", "--quality=high" }, storage);
        commands.optionFloat("--quality", [&](float) { called = true; });

        CHECK(!commands.parse());
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
        { "optionInt inline value",        test_optionInt_inline_value },
        { "optionInt invalid value",       test_optionInt_invalid_value },
        { "optionFloat inline value",      test_optionFloat_inline_value },
        { "optionFloat invalid value",     test_optionFloat_invalid_value },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("core_commandline", g_cases, std::size(g_cases), argc, argv);
}
