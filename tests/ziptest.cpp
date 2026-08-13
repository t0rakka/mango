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

struct Test
{
    const std::string pathname;
    const std::string filename;
    const u32 expected;
    const std::string password;
};

int main()
{
    const Test tests [] =
    {
        // zip tests
        { "deflate.zip",      "test.png",           0xb77cd85d, "" },
        { "deflate64.zip",    "test.png",           0xb77cd85d, "" },
        { "bzip2.zip",        "test.png",           0xb77cd85d, "" },
        { "lzma.zip",         "test.png",           0xb77cd85d, "" },
        { "ppmd.zip",         "test.png",           0xb77cd85d, "" },
        { "bzip2_crypto.zip", "test.png",           0xb77cd85d, "secret1234" },
        { "bzip2_aes256.zip", "test.png",           0xb77cd85d, "secret1234" },
        { "aes128.zip",       "test.png",           0xb77cd85d, "secret1234" },
        { "aes192.zip",       "test.png",           0xb77cd85d, "secret1234" },
        { "aes256.zip",       "test.png",           0xb77cd85d, "secret1234" },

        // 7z tests
        { "bzip2.7z",         "test.png",           0xb77cd85d, "" },
        { "lzma.7z",          "test.png",           0xb77cd85d, "" },
        { "ppmd.7z",          "test.png",           0xb77cd85d, "" },
        { "lzma2.7z",         "logo-apple.png",     0xac7b9dcc, "" },
        { "lzma2.7z",         "logo-archlinux.png", 0x02b0f3ea, "" },
        { "lzma2.7z",         "logo-linux.png",     0x4a42e206, "" },

        // hbs tests
        { "test.hbs",         "lorem-1.txt",        0xe65308f5, "" },
        { "test.hbs",         "lorem-2.txt",        0xe65308f5, "" },
    };

    int failed_count = 0;

    for (const Test& test : tests)
    {
        const std::string datapath = "data/ziptest/" + test.pathname + "/";

        try
        {
            Path path(datapath, test.password);
            File file(path, test.filename);
    
            u32 checksum = crc32(0, file);
    
            if (checksum == test.expected)
            {
                printLine("{:<16} : PASSED", test.pathname);
            }
            else
            {
                printLine("{:<16} : FAILED {:#x}", test.pathname, checksum);
                ++failed_count;
            }
        }
        catch (mango::Exception e)
        {
            printLine("Exception: {}", e.what());
            ++failed_count;
        }
    }

    return failed_count;
}
