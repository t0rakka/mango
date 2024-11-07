/*
    MANGO Multimedia Development Platform
    Copyright (C) 2012-2024 Twilight Finland 3D Oy Ltd. All rights reserved.
*/
#include <mango/core/core.hpp>
#include <mango/image/image.hpp>
#include <mango/filesystem/filesystem.hpp>

using namespace std::chrono_literals;

using namespace mango;
using namespace mango::image;
using namespace mango::filesystem;

class CustomCallback : public ImageDecoderCallback
{
public:
    void update(const ImageDecodeState& state) override
    {
        printLine("Update: {} x {} ({}, {})", state.width, state.height, state.x, state.y);
    }

    void complete() override
    {
        printLine("complete!");
    }
};

int main()
{
    File file("conquer.jpg");
    std::unique_ptr<Bitmap> bitmap;
    CustomCallback callback;

    u64 time0 = Time::ms();

    {
        AsyncImageDecoder decoder(file, "jpg");
        if (decoder.isDecoder())
        {
            ImageHeader header = decoder.header();
            bitmap = std::make_unique<Bitmap>(header.width, header.height, header.format);

            decoder.setCallback(&callback);
            decoder.launch(*bitmap);

            Sleep::ms(14);
            decoder.cancel();
        }
    }

    u64 time1 = Time::ms();
    printLine("Decoding: {} ms", time1 - time0);

    bitmap->save("test.png");
}
