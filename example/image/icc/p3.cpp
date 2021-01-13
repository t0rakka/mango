#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;

int main(int argc, const char* argv[])
{
    // Display P3 icc profile.  this non-copyrighted profile from https://github.com/saucecontrol/Compact-ICC-Profiles
    // on macOS, you could use system profile instead: 
    // File icc("/System/Library/ColorSync/Profiles/Display P3.icc");
    File icc("DisplayP3-v2-micro.icc");

    ImageEncodeOptions options;
    options.quality = 0.70f;
    options.compression = 4;
    options.icc = icc;

    Bitmap bitmapMoreRed(256, 512, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    bitmapMoreRed.clear(ColorRGBA(0xff0000ff)); // p3 full red

    Bitmap bitmap(512, 512, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));
    bitmap.clear(ColorRGBA(0xff2333ec)); // srgb full red in display p3 colorspace

    bitmap.blit(256, 0, bitmapMoreRed); // copy more reddish bar to right side of the buffer
    
    bitmap.save("icc-mango.jpg", options);
    bitmap.save("icc-mango.png", options);

    // now we have jpeg, png showing srgb full red on left, more saturated p3 full red on right side.
}

