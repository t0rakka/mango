/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>

using namespace mango;
using namespace mango::image;
using namespace mango::filesystem;

class CustomCallback : public ImageDecoderCallback
{
public:
    void update(const ImageDecodeRect& rect) override
    {
        printLine("[+] Update: [{} x {}] .. ({}, {})",
            rect.width, rect.height, rect.x, rect.y);
    }

    void complete() override
    {
        printLine("[+] Complete!");
    }
};

int main(int argc, const char* argv[])
{
    if (argc < 2)
    {
        printLine("Too few arguments. usage: <filename.jpg> <cancel_ms>");
        exit(1);
    }

    const char* filename = argv[1];

    int cancel_ms = -1;
    if (argc == 3)
    {
        cancel_ms = std::atoi(argv[2]);
    }

    File file(filename);
    std::unique_ptr<Bitmap> bitmap;
    CustomCallback callback;

    u64 time0 = Time::ms();

    {
        AsyncImageDecoder decoder(file, filename);
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            bitmap = std::make_unique<Bitmap>(header.width, header.height, header.format);
            //bitmap = std::make_unique<Bitmap>(header.width, header.height, Format(16, Format::UNORM, Format::BGR, 5, 6, 5));

            decoder.setCallback(&callback);
            decoder.launch(*bitmap);

            if (cancel_ms >= 0)
            {
                Sleep::ms(cancel_ms);
                decoder.cancel();
            }
        }
    }

    u64 time1 = Time::ms();
    printLine("Decoding: {} ms", time1 - time0);

    bitmap->save("test.png");
}
