#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>
#include <iostream>

using namespace std;

namespace img_lib {
    
inline static int BYTES_PER_PIXEL = 3;
inline static int PADDING_FOR_ROUNDING = 3;
inline static int ALIGMENT = 4;
    
static int GetBMPStride(int w) {
    int unalignedStride = w * BYTES_PER_PIXEL;
    return ALIGMENT * ((unalignedStride + PADDING_FOR_ROUNDING) / ALIGMENT);
}    
    
PACKED_STRUCT_BEGIN BitmapFileHeader {
    BitmapFileHeader() = default;
    BitmapFileHeader(int width, int height) {
        sizeOfBitmapFile = height * GetBMPStride(width) + 54;
    }
    
    char bitmapSignatureBytes[2] = {'B', 'M'};
    unsigned int sizeOfBitmapFile;
    uint32_t reservedBytes = 0;
    unsigned int pixelDataOffset = 54;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    BitmapInfoHeader() = default;
    BitmapInfoHeader(int width, int height) : width_(width), height_(height) {
        rawBitmapDataSize = height * GetBMPStride(width);
    }

    unsigned int sizeOfThisHeader = 40;
    int width_;
    int height_;
    uint16_t numberOfColorPlanes = 1;
    uint16_t colorDepth = 24;
    uint32_t compressionMethod = 0;
    uint32_t rawBitmapDataSize;
    int horizontalResolution = 11811;
    int verticalResolution = 11811;
    int colorTableEntries = 0;
    int importantColors = 0x1000000;
}
PACKED_STRUCT_END

bool SaveBMP(const Path& file, const Image& image) {
    BitmapFileHeader file_header_(image.GetWidth(), image.GetHeight());
    BitmapInfoHeader info_header_(image.GetWidth(), image.GetHeight());
    
    ofstream out(file, ios::binary);
    out.write((char*) &file_header_, 14);
    out.write((char*) &info_header_, 40);

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    const int padding = GetBMPStride(w);
    std::vector<char> buff(padding);

    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < w; ++x) {
            buff[x * 3 + 0] = static_cast<char>(line[x].b);
            buff[x * 3 + 1] = static_cast<char>(line[x].g);
            buff[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        out.write(buff.data(), padding);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    BitmapFileHeader file_header_;
    BitmapInfoHeader info_header_;
    ifstream ifs(file, ios::binary);
    
    if (!ifs || ifs.fail()) {
        return {};
    }

    ifs.read((char *) &file_header_, sizeof(BitmapFileHeader));
    
    if (ifs.fail() || file_header_.bitmapSignatureBytes[0] != 'B' || file_header_.bitmapSignatureBytes[1] != 'M') {
        return {};
    }
    
    ifs.read((char *) &info_header_, sizeof(BitmapInfoHeader));
    
    if (ifs.fail() || info_header_.width_ < 0 || info_header_.height_ < 0) {
        return {};
    }

    int w, h, padding;
    w = info_header_.width_;
    h = info_header_.height_;
    padding = GetBMPStride(w);
    Image result(w, h, Color::Black());

    std::vector<char> buff(padding);
    for (int y = h - 1; y >= 0; --y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), padding);

        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].r = static_cast<byte>(buff[x * 3 + 2]);
        }
    }
    return result;
}

}