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

struct Listener : ImageDecodeListener
{
    Bitmap bitmap;

    Listener(int width, int height, const Format& format)
        : bitmap(width, height, format)
    {
    }

    void update(const ImageDecodeState& state) override
    {
        printLine("Update!");
    }
};

int main()
{
    File file("conquer.jpg");
    std::unique_ptr<Listener> listener;
    ImageDecoder decoder(file, "jpg");
    if (decoder.isDecoder())
    {
        ImageHeader header = decoder.header();

        listener = std::make_unique<Listener>(header.width, header.height, header.format);

        ImageDecodeOptions options;
        options.decode_listener = listener.get();

        decoder.decode(listener->bitmap, options);
    }
}
