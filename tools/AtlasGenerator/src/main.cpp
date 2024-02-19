#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

bool is_image_size_valid(const std::string &imagePath, int targetWidth, int targetHeight) {
    int width, height, channels;
    unsigned char *image = stbi_load(imagePath.c_str(), &width, &height, &channels, 4); // 3 channels for RGB

    if (!image) {
        std::cerr << "Error loading image: " << imagePath << std::endl;
        return false;
    }

    bool isValid = (width == targetWidth) && (height == targetHeight);

    stbi_image_free(image); // Release memory allocated by stbi_load

    return isValid;
}

// Function to load an image from a file using stb_image
std::vector<unsigned char> load_image(const std::string &filePath, int &width, int &height, int &channels) {
    stbi_uc *data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Error loading image: " << filePath << std::endl;
        return {};
    }

    if (channels == 4) {
        std::cerr << "Error, This has RGBA format color format" << std::endl;
    }

    std::vector<unsigned char> imageData(data, data + width * height * channels);
    stbi_image_free(data);

    return imageData;
}

const uint32_t IMAGE_SIZE = 256;
const uint32_t PIXEL_TYPE = STBI_rgb_alpha;

// Function to create a texture atlas from images in a directory
void create_texture_atlas(std::string directoryPath, int atlasWidth, int atlasHeight) {
    unsigned char *atlas = new unsigned char[atlasWidth * atlasHeight * PIXEL_TYPE];
    int atlasIndex = 0;
    int xBlock = 0;
    int yBlock = 0;

    assert(atlasWidth % IMAGE_SIZE == 0 && atlasHeight % IMAGE_SIZE == 0);

    int maxWidthBlocks = atlasWidth / IMAGE_SIZE;
    int maxHeightBlocks = atlasHeight / IMAGE_SIZE;

    bool has_started = false;
    std::string fullPath = std::string(PROJECT_ROOT_PATH) + "/" + directoryPath;
    std::vector<std::string> files;
    for (const auto &entry : fs::directory_iterator(fullPath)) {
        const std::string imagePath = entry.path().string();

        if (imagePath.length() >= 4 && imagePath.substr(imagePath.length() - 4) != ".png") {
            continue; // Skip non-PNG files
        }

        files.push_back(entry.path().string());
    }

    std::sort(files.begin(), files.end());
    for (const auto imagePath : files) {
        int width, height, channels;

        bool isValid = is_image_size_valid(imagePath, 256, 256);
        if (!isValid) {
            printf("Invalid image format: %s\n", imagePath.c_str());
            continue;
        }
        std::vector<unsigned char> imageData = load_image(imagePath, width, height, channels);

        if (imageData.empty()) {
            continue;
        }

        int topLeftOffset = xBlock * IMAGE_SIZE * PIXEL_TYPE + yBlock * maxWidthBlocks * IMAGE_SIZE * IMAGE_SIZE * PIXEL_TYPE;
        has_started = true;
        // copy the image to the atlas
        for (int y = 0; y < IMAGE_SIZE; y++) {
            int offsetAtlas = topLeftOffset + y * atlasWidth * PIXEL_TYPE;
            int offset = y * IMAGE_SIZE * PIXEL_TYPE;

            memcpy(&atlas[offsetAtlas], &imageData[offset], IMAGE_SIZE * PIXEL_TYPE);
        }

        xBlock++;

        if (xBlock == maxWidthBlocks) {
            yBlock++;
            xBlock = 0;

            if (yBlock == maxHeightBlocks) {

                std::ostringstream atlasFileName;
                atlasFileName << "texture_atlas_" << atlasIndex++ << ".png";
                stbi_write_png(atlasFileName.str().c_str(), atlasWidth, atlasHeight, PIXEL_TYPE, atlas, 0);
                has_started = false;
                // Reset the buffer
                for (int i = 0; i < atlasWidth * atlasHeight * PIXEL_TYPE; i++) {
                    atlas[i] = 0;
                }
                yBlock = 0;
            }
        }
    }
    if (has_started) {

        // TODO RESIZE

        // char *resized = new char[(xBlock + 1) * (yBlock + 1) * IMAGE_SIZE * IMAGE_SIZE * PIXEL_TYPE];

        // for (int y = 0; y < yBlock; y++) {

        //     int blocks = yBlock * maxWidthBlocks;
        //     int pixelsOffsetY = blocks * IMAGE_SIZE * IMAGE_SIZE * PIXEL_TYPE;

        //     for (int x = 0; x < xBlock; x++) {
        //         int pixelsOffsetX = x * IMAGE_SIZE;

        //        for(int g = 0; g < IMAGE_SIZE; g++){

        //             memcpy(, const void *__restrict src, size_t n) 
        //        }
        //     }
        // }

        std::ostringstream atlasFileName;
        int totalBlocks = (xBlock + 1) * (yBlock + 1);
        atlasFileName << "texture_atlas_" << atlasIndex++ << ".png";
        stbi_write_png(atlasFileName.str().c_str(), atlasWidth, atlasHeight, PIXEL_TYPE, atlas, 0);
    }
}

int main(int argc, char **argv) {

    std::string directoryPath = std::string(argv[1]);

    // Size of the texture atlas
    printf("hello %d\n", std::stoi(argv[2]));

    int atlasWidth = std::stoi(argv[2]);
    int atlasHeight = std::stoi(argv[2]);

    // Create the texture atlas
    create_texture_atlas(directoryPath, atlasWidth, atlasHeight);

    std::cout << "Texture atlas created successfully." << std::endl;

    return 0;
}