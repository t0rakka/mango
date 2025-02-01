/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2021 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

static int g_count_failed = 0;

void printLine()
{
    printf("------------------------------------------------------------------ \n");
}

void print(const Path& path, const std::string& correct_path)
{
    bool status = path.pathname() == correct_path;
    g_count_failed += !status;
 
    printf("[path]        %s \n", correct_path.c_str());
    printf("    pathname: %s [%s]\n", path.pathname().c_str(), status ? "PASSED" : "FAILED");

    if (path.empty())
    {
        printf("      (empty)\n");
    }
    else
    {
        for (auto node : path)
        {
            if (node.isDirectory())
                printf("      > %s    (directory)\n", node.name.c_str());
        }

        for (auto node : path)
        {
            if (node.isContainer())
                printf("      > %s    (container)\n", node.name.c_str());
        }

        for (auto node : path)
        {
            if (!node.isDirectory())
            {
                const char* str_compressed = node.isCompressed() ? "+Compressed " : "";
                const char* str_encrypted = node.isEncrypted() ? "+Encrypted " : "";
                printf("      > %s    (file: %d KB) %s%s\n", node.name.c_str(), int(node.size / 1024),
                    str_compressed, str_encrypted);
            }
        }
    }

    printf("\n");
}

void print(const File& file, const std::string& correct_filename, u32 correct_checksum)
{
    printf("[file]        %s + %s, size: %" PRIu64 " bytes \n",
        getPath(correct_filename).c_str(),
        removePath(correct_filename).c_str(),
        file.size());

    u32 checksum = crc32c(0, file);

    bool status_checksum = checksum == correct_checksum;
    bool status_pathname = file.pathname() == getPath(correct_filename);
    bool status_filename = file.filename() == removePath(correct_filename);
    g_count_failed += !status_checksum;
    g_count_failed += !status_pathname;
    g_count_failed += !status_filename;

    printf("    pathname: %s [%s]\n", file.pathname().c_str(), status_pathname ? "PASSED" : "FAILED");
    printf("    filename: %s [%s]\n", file.filename().c_str(), status_filename ? "PASSED" : "FAILED");
    printf("    checksum: 0x%8x [%s]\n", checksum, status_checksum ? "PASSED" : "FAILED");
    printf("\n");
}

void test0()
{
    Path path1("");
    print(path1, "");

    Path path2("data/");
    print(path2, "data/");
}

void test1()
{
    Path path("data/");
    print(path, "data/");
}

void test2()
{
    Path path("data/foo/");
    print(path, "data/foo/");
}

void test3()
{
    Path path("data/kokopaska.zip/");
    print(path, "data/kokopaska.zip/");
}

void test4()
{
    Path path("data/kokopaska2.zip/foo/");
    print(path, "data/kokopaska2.zip/foo/");
}

void test5()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path, "data/outer.zip/data/inner.zip/");
}

void test6()
{
    Path path1("data/outer.zip/data/");
    print(path1, "data/outer.zip/data/");

    Path path2(path1, "inner.zip/");
    print(path2, "data/outer.zip/data/inner.zip/");
}

// opening a file

void test7()
{
    File file("data/kokopaska.zip/test/flower1.jpg");
    print(file, "data/kokopaska.zip/test/flower1.jpg", 0xbb8abc19);
}

void test8()
{
    File file("data/outer.zip/data/inner.zip/test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

// opening a file with pathing

void test9()
{
    Path path("data/kokopaska.zip/");
    print(path, "data/kokopaska.zip/");

    File file(path, "test/flower1.jpg");
    print(file, "data/kokopaska.zip/test/flower1.jpg", 0xbb8abc19);
}

void test10()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path, "data/outer.zip/data/inner.zip/");

    File file(path, "test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test11()
{
    Path path1("data/outer.zip/");
    print(path1, "data/outer.zip/");

    Path path2(path1, "data/inner.zip/");
    print(path2, "data/outer.zip/data/inner.zip/");

    File file(path2, "test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

// pathing with fs folder

void test12()
{
    Path path1("data/");
    print(path1, "data/");

    Path path2(path1, "kokopaska.zip/");
    print(path2, "data/kokopaska.zip/");

    File file(path2, "test/flower1.jpg");
    print(file, "data/kokopaska.zip/test/flower1.jpg", 0xbb8abc19);
}

void test13()
{
    Path path1("data/");
    print(path1, "data/");

    Path path2(path1, "outer.zip/data/inner.zip/");
    print(path2, "data/outer.zip/data/inner.zip/");

    File file(path2, "test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test14()
{
    Path path1("data/");
    print(path1, "data/");

    Path path2(path1, "outer.zip/");
    print(path2, "data/outer.zip/");

    Path path3(path2, "data/inner.zip/");
    print(path3, "data/outer.zip/data/inner.zip/");

    File file(path3, "test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test15()
{
    Path path("data/");
    print(path, "data/");
}

void test16()
{
    Path path1("data/");
    print(path1, "data/");

    Path path2(path1, "foo/");
    print(path2, "data/foo/");
}

void test17()
{
    Path path("data/");
    print(path, "data/");

    File file(path, "foo/test.txt");
    print(file, "data/foo/test.txt", 0x569510f1);
}

void test18()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path, "data/outer.zip/data/inner.zip/");

    File file(path, "test/flower1.jpg");
    print(file, "data/outer.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test19()
{
    File file("data/kokopaska.zip");
    print(file, "data/kokopaska.zip", 0x5d61ea66);

    ConstMemory memory = file;
    Path path(memory, ".zip");
    print(path, "@memory.zip/");

    Path path2(path, "test/");
    print(path2, "@memory.zip/test/");
}

void test20()
{
    File file("data/foo/test.txt");
    print(file, "data/foo/test.txt", 0x569510f1);

    ConstMemory memory = file;
    Path path(memory, ".txt");
    print(path, ""); // the mapping should fail as ".txt" is not a container
}

void test21()
{
    Path path("../");
    print(path, "../");
}

// bench tests

void test22()
{
    File file("data/case.snitch/bench/IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

void test23()
{
    Path path("data/case.snitch/");
    print(path, "data/case.snitch/");

    File file(path, "bench/IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

void test24()
{
    Path path("data/case.snitch/bench/");
    print(path, "data/case.snitch/bench/");

    File file(path, "IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

void test25()
{
    Path path1("data/case.snitch/");
    print(path1, "data/case.snitch/");

    Path path2(path1, "bench/");
    print(path2, "data/case.snitch/bench/");

    File file(path2, "IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

void test26()
{
    Path path1("data/");
    print(path1, "data/");

    Path path2(path1, "case.snitch/");
    print(path2, "data/case.snitch/");

    Path path3(path2, "bench/");
    print(path3, "data/case.snitch/bench/");

    File file(path3, "IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

// memory mapped container

void test27()
{
    File file1("data/outer.zip");
    print(file1, "data/outer.zip", 0x12ea02f3);

    ConstMemory memory = file1;
    Path path1(memory, ".zip");
    print(path1, "@memory.zip/");

    Path path2(path1, "data/inner.zip/");
    print(path2, "@memory.zip/data/inner.zip/");

    File file2(path2, "test/flower1.jpg");
    print(file2, "@memory.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test28()
{
    File file(Path("data/case.snitch/"), "bench/IMG_2177.JPG");
    print(file, "data/case.snitch/bench/IMG_2177.JPG", 0x472da743);
}

void test29()
{
    // bad.xxx is not a file; it is a folder

    Path path("data/bad.xxx/");
    print(path, "data/bad.xxx/");

    File file1(path, "dummy.txt");
    print(file1, "data/bad.xxx/dummy.txt", 0xc96fd51e);

    File file2("data/bad.xxx/dummy.txt");
    print(file2, "data/bad.xxx/dummy.txt", 0xc96fd51e);
}

void test30()
{
    // bad.zip is not a file; it is a folder

    Path path("data/bad.zip/");
    print(path, "data/bad.zip/");

    File file1(path, "dummy.txt");
    print(file1, "data/bad.zip/dummy.txt", 0xfd887d87);

    File file2("data/bad.zip/dummy.txt");
    print(file2, "data/bad.zip/dummy.txt", 0xfd887d87);
}

void test31()
{
    File file1("data/outer.zip");
    print(file1, "data/outer.zip", 0x12ea02f3);

    ConstMemory memory = file1;
    File file2(memory, ".zip", "data/inner.zip/test/flower1.jpg");
    print(file2, "@memory.zip/data/inner.zip/test/flower1.jpg", 0xbb8abc19);
}

void test32()
{
    // These are corrupted / not container files
    // We still should be able to index them and get nothing

    Path path1("data/fake/random.zip/");
    print(path1, "data/fake/random.zip/");

    Path path2("data/fake/random.rar/");
    print(path2, "data/fake/random.rar/");

    Path path3("data/fake/random.snitch/");
    print(path3, "data/fake/random.snitch/");
}

// -----------------------------------------------------------------------------------
// main()
// -----------------------------------------------------------------------------------

#define MAKE_TEST(i) \
    printLine(); \
    printf("Test: %d\n", i); \
    printLine(); \
    printf("\n"); \
    test##i()

int main(int argc, char *argv[])
{
    MAKE_TEST(0);
    MAKE_TEST(1);
    MAKE_TEST(2);
    MAKE_TEST(3);
    MAKE_TEST(4);
    MAKE_TEST(5);
    MAKE_TEST(6);
    MAKE_TEST(7);
    MAKE_TEST(8);
    MAKE_TEST(9);
    MAKE_TEST(10);
    MAKE_TEST(11);
    MAKE_TEST(12);
    MAKE_TEST(13);
    MAKE_TEST(14);
    MAKE_TEST(15);
    MAKE_TEST(16);
    MAKE_TEST(17);
    MAKE_TEST(18);
    MAKE_TEST(19);
    MAKE_TEST(20);
    MAKE_TEST(21);
    MAKE_TEST(22);
    MAKE_TEST(23);
    MAKE_TEST(24);
    MAKE_TEST(25);
    MAKE_TEST(26);
    MAKE_TEST(27);
    MAKE_TEST(28);
    MAKE_TEST(29);
    MAKE_TEST(30);
    MAKE_TEST(31);
    MAKE_TEST(32);

    printLine();
    if (g_count_failed)
        printf("  %d tests FAILED.                     \n", g_count_failed);
    else
        printf("  All tests PASSED.                    \n");
    printLine();
}
