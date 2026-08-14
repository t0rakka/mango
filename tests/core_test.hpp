/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#pragma once

#include <cstring>
#include <string_view>
#include <vector>

#include <mango/mango.hpp>

#define CORE_CHECK(cond) \
    do { \
        if (!(cond)) { \
            mango::printLine("    FAILED: {}", #cond); \
            return false; \
        } \
    } while (0)

namespace mango::test
{

    struct Case
    {
        const char* name;
        bool (*func)();
    };

    inline bool matches_filter(std::string_view name, const std::vector<std::string_view>& filters)
    {
        if (filters.empty())
        {
            return true;
        }

        for (std::string_view filter : filters)
        {
            if (name.find(filter) != std::string_view::npos)
            {
                return true;
            }
        }

        return false;
    }

    inline int run_cases(const char* suite, const Case* cases, size_t count, int argc, char* argv[])
    {
        std::vector<std::string_view> filters;

        if (argc > 1)
        {
            for (int i = 1; i < argc; ++i)
            {
                std::string_view arg = argv[i];

                if (arg == "help" || arg == "-h" || arg == "--help")
                {
                    printLine("Usage:");
                    printLine("  {}            run all tests", suite);
                    printLine("  {} list       list test names", suite);
                    printLine("  {} <filter>   run matching tests", suite);
                    return 0;
                }

                if (arg == "list")
                {
                    for (size_t i = 0; i < count; ++i)
                    {
                        printLine("  {}", cases[i].name);
                    }
                    return 0;
                }

                filters.push_back(arg);
            }
        }

        printLine("------------------------------------------------------------");
        printLine(" {}", suite);
        printLine("------------------------------------------------------------");

        int passed = 0;
        int failed = 0;
        int skipped = 0;

        for (size_t i = 0; i < count; ++i)
        {
            if (!matches_filter(cases[i].name, filters))
            {
                ++skipped;
                continue;
            }

            printLine("");
            printLine("  {}", cases[i].name);

            const bool ok = cases[i].func();
            printLine("  => {}", ok ? "PASSED" : "FAILED");

            if (ok)
            {
                ++passed;
            }
            else
            {
                ++failed;
            }
        }

        printLine("");
        printLine("------------------------------------------------------------");
        printLine(" Summary: {} passed, {} failed, {} skipped", passed, failed, skipped);
        printLine("------------------------------------------------------------");

        return failed > 0 ? 1 : 0;
    }

} // namespace mango::test
