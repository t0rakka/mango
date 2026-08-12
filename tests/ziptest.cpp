/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2026 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

/*

# create AES128..256 encrypted test files
import pyzipper

with pyzipper.AESZipFile('aes128.zip', 'w',
                         compression=pyzipper.ZIP_DEFLATED,
                         encryption=pyzipper.WZ_AES) as zf:
    zf.setpassword(b'secret1234')
    zf.setencryption(pyzipper.WZ_AES, nbits=128)   # 128 / 192 / 256
    zf.write('test.png')

*/

int test(const std::string& pathname, const std::string& filename, u32 expected, const std::string& password)
{
    const std::string datapath = "data/ziptest/" + pathname + "/";

    try
    {
        Path path(datapath, password);
        File file(path, filename);

        u32 checksum = crc32(0, file);

        if (checksum == expected)
        {
            printLine("{:<16} : PASSED", pathname);
            return 0;
        }
        else
        {
            printLine("{:<16} : FAILED {:#x}", pathname, checksum);
            return 1;
        }
    }
    catch (mango::Exception e)
    {
        printLine("Exception: {}", e.what());
        return 1;
    }

    return 0;
}

int main()
{
    int failed_count = 0;
    failed_count += test("deflate.zip",      "test.png", 0xb77cd85d, "");
    failed_count += test("deflate64.zip",    "test.png", 0xb77cd85d, "");
    failed_count += test("bzip2.zip",        "test.png", 0xb77cd85d, "");
    failed_count += test("lzma.zip",         "test.png", 0xb77cd85d, "");
    failed_count += test("ppmd.zip",         "test.png", 0xb77cd85d, "");
    failed_count += test("bzip2_crypto.zip", "test.png", 0xb77cd85d, "secret1234");
    failed_count += test("bzip2_aes256.zip", "test.png", 0xb77cd85d, "secret1234");
    failed_count += test("aes128.zip",       "test.png", 0xb77cd85d, "secret1234");
    failed_count += test("aes192.zip",       "test.png", 0xb77cd85d, "secret1234");
    failed_count += test("aes256.zip",       "test.png", 0xb77cd85d, "secret1234");
    return !!failed_count;
}
