/*
*/
#include <mango/mango.hpp>

using namespace mango;
using namespace mango::filesystem;
using namespace mango::image;

void printSize(const File &f)
{
    printf("'%s' is %" PRIu64 " bytes\n", f.filename().c_str(), f.size());
}

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

    printSize(icc);

    Bitmap bitmap(512, 512, Format(32, Format::UNORM, Format::RGBA, 8, 8, 8, 8));

    // clear left half with srgb full red in display p3 colorspace
    Surface(bitmap, 0, 0, 256, 512).clear(Color(0xff2333ec));

    // clear right half with p3 full red
    Surface(bitmap, 256, 0, 256, 512).clear(Color(0xff0000ff));

    bitmap.save("icc-mango.jpg", options);
    bitmap.save("icc-mango.png", options);

    printSize(File("icc-mango.jpg"));
    printSize(File("icc-mango.png"));

    // now we have jpeg, png showing srgb full red on left, more saturated p3 full red on right side.
}
