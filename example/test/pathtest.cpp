/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2020 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

void printLine(int i)
{
    printf("---------------------------------- %d\n\n", i);
}

void print(const Path& path)
{
    printf("[path] %s\n", path.pathname().c_str());
    for (auto node : path)
    {
        if (node.isDirectory())
            printf("  > %s \n", node.name.c_str());
    }

    for (auto node : path)
    {
        if (!node.isDirectory())
            printf("  > %s \n", node.name.c_str());
    }
    printf("\n");
}

void print(const File& file)
{
    printf("[file] %s + %s, size: %" PRIu64 " bytes \n", 
        file.pathname().c_str(), 
        file.filename().c_str(), 
        file.size());

    // sum of every byte in the file to validate the memory mapping
    u64 sum = 0;
    ConstMemory memory = file;
    for (u64 i = 0; i < memory.size; ++i)
    {
        sum += memory.address[i];
    }
    printf("    checksum: %d\n", u32(sum));

    printf("\n");
}

void test0()
{
    Path path1("");
    print(path1);

    Path path2("data/");
    print(path2);

    File file("pathtest.cpp");
    print(file);
}

void test1()
{
    Path path("data/");
    print(path);
}

void test2()
{
    Path path("data/foo/");
    print(path);
}

void test3()
{
    Path path("data/kokopaska.zip/");
    print(path);
}

void test4()
{
    Path path("data/kokopaska2.zip/foo/");
    print(path);
}

void test5()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path);
}

void test6()
{
    Path path1("data/outer.zip/data/");
    print(path1);

    Path path2(path1, "inner.zip/");
    print(path2);
}

// opening a file

void test7()
{
    File file("data/kokopaska.zip/test/flower1.jpg");
    print(file);
}

void test8()
{
    File file("data/outer.zip/data/inner.zip/test/flower1.jpg");
    print(file);
}

// opening a file with pathing

void test9()
{
    Path path("data/kokopaska.zip/");
    print(path);

    File file(path, "test/flower1.jpg");
    print(file);
}

void test10()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path);

    File file(path, "test/flower1.jpg");
    print(file);
}

void test11()
{
    Path path1("data/outer.zip/");
    print(path1);

    Path path2(path1, "data/inner.zip/");
    print(path2);

    File file(path2, "test/flower1.jpg");
    print(file);
}

// pathing with fs folder

void test12()
{
    Path path1("data/");
    print(path1);

    Path path2(path1, "kokopaska.zip/");
    print(path2);

    File file(path2, "test/flower1.jpg");
    print(file);
}

void test13()
{
    Path path1("data/");
    print(path1);

    Path path2(path1, "outer.zip/data/inner.zip/");
    print(path2);

    File file(path2, "test/flower1.jpg");
    print(file);
}

void test14()
{
    Path path1("data/");
    print(path1);

    Path path2(path1, "outer.zip/");
    print(path2);

    Path path3(path2, "data/inner.zip/");
    print(path3);

    File file(path3, "test/flower1.jpg");
    print(file);
}

void test15()
{
    Path path("data/");
    print(path);
}

void test16()
{
    Path path1("data/");
    print(path1);

    Path path2(path1, "foo/");
    print(path2);
}

void test17()
{
    Path path("data/");
    print(path);

    File file(path, "foo/test.txt");
    print(file);
}

void test18()
{
    Path path("data/outer.zip/data/inner.zip/");
    print(path);

    File file(path, "test/flower1.jpg");
    print(file);
}

void test19()
{
    File file("data/kokopaska.zip");
    print(file);

    ConstMemory memory = file;
    Path path(memory, ".zip");
    print(path);

    Path path2(path, "test/");
    print(path2);
}

void test20()
{
    // bullshit container; will not "contain" anything :)
    File file("data/foo/test.txt");
    print(file);

    ConstMemory memory = file;
    Path path(memory, ".txt");
    print(path);
}

void test21()
{
    Path path("../");
    print(path);
}

// bench tests

void test22()
{
    File file("data/case.snitch/bench/IMG_2177.JPG");
    print(file);
}

void test23()
{
    Path path("data/case.snitch/");
    print(path);

    File file(path, "bench/IMG_2177.JPG");
    print(file);
}

void test24()
{
    Path path("data/case.snitch/bench/");
    print(path);

    File file(path, "IMG_2177.JPG");
    print(file);
}

void test25()
{
    Path path1("data/case.snitch/");
    print(path1);

    Path path2(path1, "bench/");
    print(path2);

    File file(path2, "IMG_2177.JPG");
    print(file);
}

void test26()
{
    Path path1("data/");
    print(path1);

    Path path2(path1, "case.snitch/");
    print(path2);

    Path path3(path2, "bench/");
    print(path3);

    File file(path3, "IMG_2177.JPG");
    print(file);
}

// memory mapped container

void test27()
{
    File file1("data/outer.zip");
    print(file1);

    ConstMemory memory = file1;
    Path path1(memory, ".zip");
    print(path1);

    Path path2(path1, "data/inner.zip/");
    print(path2);

    File file2(path2, "test/flower1.jpg");    
    print(file2);
}

void test28()
{
#if 1
    File file(Path("data/case.snitch/"), "bench/IMG_2177.JPG");
    print(file);
#endif
}

// -----------------------------------------------------------------------------------
// main()
// -----------------------------------------------------------------------------------

#define MAKE_TEST(i) \
    printLine(i); \
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

    printf("---------------------------------- done\n\n");
}
