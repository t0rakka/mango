/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include "core_test.hpp"

#include <vector>

using namespace mango;
using namespace mango::filesystem;
using mango::test::Case;
using mango::test::run_cases;

#define CHECK CORE_CHECK

namespace
{

    bool indexHas(const Path& path, std::string_view name)
    {
        for (const FileInfo& info : path)
        {
            if (info.name == name)
                return true;
        }
        return false;
    }

    bool test_copy_same_pathname()
    {
        Path original("data/pathtest/foo/");
        Path copy = original;

        CHECK(copy.pathname() == original.pathname());
        CHECK(copy.pathname() == "data/pathtest/foo/");

        return true;
    }

    bool test_copy_file_access()
    {
        Path original("data/pathtest/kokopaska.zip/");
        Path copy = original;

        File viaOriginal(original, "test/flower1.jpg");
        File viaCopy(copy, "test/flower1.jpg");

        CHECK(viaOriginal.size() == viaCopy.size());
        CHECK(viaOriginal.size() > 0);
        CHECK(crc32c(0, viaOriginal) == crc32c(0, viaCopy));
        CHECK(crc32c(0, viaCopy) == 0xbb8abc19u);

        return true;
    }

    bool test_copy_nested_container()
    {
        Path original("data/pathtest/outer.zip/data/inner.zip/");
        Path copy = original;

        File file(copy, "test/flower1.jpg");
        CHECK(file.size() > 0);
        CHECK(crc32c(0, file) == 0xbb8abc19u);

        return true;
    }

    bool test_copy_child_path()
    {
        Path root("data/pathtest/");
        Path child(root, "kokopaska.zip/");
        Path copy = child;

        CHECK(copy.pathname() == child.pathname());
        CHECK(copy.pathname() == "data/pathtest/kokopaska.zip/");

        File file(copy, "test/flower1.jpg");
        CHECK(crc32c(0, file) == 0xbb8abc19u);

        return true;
    }

    bool test_copy_assignment()
    {
        Path a("data/pathtest/foo/");
        Path b("data/pathtest/kokopaska.zip/");

        b = a;

        CHECK(b.pathname() == a.pathname());

        File file(b, "test.data");
        CHECK(crc32c(0, file) == 0x149cd379u);

        return true;
    }

    bool test_move()
    {
        Path original("data/pathtest/kokopaska.zip/");
        const std::string pathname = original.pathname();

        Path moved = std::move(original);
        CHECK(moved.pathname() == pathname);

        File file(moved, "test/flower1.jpg");
        CHECK(crc32c(0, file) == 0xbb8abc19u);

        return true;
    }

    bool test_copy_preserves_index()
    {
        Path original("data/pathtest/foo/");
        const size_t originalSize = original.size();
        CHECK(originalSize > 0);

        Path copy = original;
        CHECK(copy.size() == originalSize);
        CHECK(&copy.getIndex() == &original.getIndex());

        return true;
    }

    bool test_store_in_container()
    {
        Path root("data/pathtest/kokopaska.zip/");

        std::vector<Path> paths;
        paths.push_back(root);
        paths.emplace_back(root, "test/");

        CHECK(paths[0].pathname() == "data/pathtest/kokopaska.zip/");
        CHECK(paths[1].pathname() == "data/pathtest/kokopaska.zip/test/");

        File file(paths[0], "test/flower1.jpg");
        CHECK(crc32c(0, file) == 0xbb8abc19u);

        return true;
    }

    bool test_fresh_string_still_independent()
    {
        Path a("data/pathtest/kokopaska.zip/");
        Path b("data/pathtest/kokopaska.zip/");

        CHECK(a.pathname() == b.pathname());

        File fileA(a, "test/flower1.jpg");
        File fileB(b, "test/flower1.jpg");
        CHECK(crc32c(0, fileA) == crc32c(0, fileB));

        return true;
    }

    bool test_zip_logical_subfolder_index()
    {
        Path zip("data/pathtest/kokopaska.zip/");
        Path sub(zip, "test/");

        CHECK(sub.size() > 0);
        CHECK(indexHas(sub, "flower1.jpg"));

        // Logical subfolder: same zip mapper, different basepath, separate index cache.
        CHECK(&zip.getIndex() != &sub.getIndex());

        Path subCopy = sub;
        CHECK(&subCopy.getIndex() == &sub.getIndex());

        // Zip root lists the folder entry, not nested files as direct children.
        CHECK(!indexHas(zip, "flower1.jpg"));

        return true;
    }

    const Case cases[] =
    {
        { "copy_same_pathname", test_copy_same_pathname },
        { "copy_file_access", test_copy_file_access },
        { "copy_nested_container", test_copy_nested_container },
        { "copy_child_path", test_copy_child_path },
        { "copy_assignment", test_copy_assignment },
        { "move", test_move },
        { "copy_preserves_index", test_copy_preserves_index },
        { "store_in_container", test_store_in_container },
        { "fresh_string_still_independent", test_fresh_string_still_independent },
        { "zip_logical_subfolder_index", test_zip_logical_subfolder_index },
    };

} // namespace

int main(int argc, char* argv[])
{
    return run_cases("filesystem_path", cases, std::size(cases), argc, argv);
}
