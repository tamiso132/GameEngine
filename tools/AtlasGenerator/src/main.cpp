#include <cstdio>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

bool isImageSizeValid(const std::string &imagePath, int targetWidth,
                      int targetHeight) {
  int width, height, channels;
  unsigned char *image = stbi_load(imagePath.c_str(), &width, &height,
                                   &channels, 3); // 3 channels for RGB

  if (!image) {
    std::cerr << "Error loading image: " << imagePath << std::endl;
    return false;
  }

  bool isValid = (width == targetWidth) && (height == targetHeight);

  stbi_image_free(image); // Release memory allocated by stbi_load

  return isValid;
}

// Function to load an image from a file using stb_image
std::vector<unsigned char> loadImage(const std::string &filePath, int &width,
                                     int &height, int &channels) {
  stbi_uc *data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
  if (!data) {
    std::cerr << "Error loading image: " << filePath << std::endl;
    return {};
  }

  std::vector<unsigned char> imageData(data, data + width * height * channels);
  stbi_image_free(data);

  return imageData;
}

// Function to create a texture atlas from images in a directory
std::vector<unsigned char> createTextureAtlas(const std::string &directoryPath,
                                              int atlasWidth, int atlasHeight) {
  std::vector<unsigned char> atlas(atlasWidth * atlasHeight * 3,
                                   0); // Assuming 3 channels (RGB)
  int atlasIndex = 1;
  int currentX = 0;
  int currentY = 0;

  for (const auto &entry : fs::directory_iterator(directoryPath)) {
    const std::string imagePath = entry.path().string();
    int width, height, channels;
    std::vector<unsigned char> imageData =
        loadImage(imagePath, width, height, channels);

    if (imageData.empty()) {
      continue;
    }

    if (imagePath.length() >= 4 &&
        imagePath.substr(imagePath.length() - 4) != ".png") {
      continue; // Skip non-PNG files
    }
    bool isValid = isImageSizeValid(imagePath, 256, 256);
    if (!isValid) {
      printf("Invalid image format: %s\n", imagePath.c_str());
      continue;
    }

    // Create a new texture atlas if the current one is full
    if (currentX + width > atlasWidth) {
      currentX = 0;
      currentY += height;

      // Check if a new atlas is needed
      if (currentY + height > atlasHeight) {
        // Save the current atlas
        std::ostringstream atlasFileName;
        atlasFileName << "texture_atlas_" << atlasIndex++ << ".png";
        stbi_write_png(atlasFileName.str().c_str(), atlasWidth, atlasHeight, 3,
                       atlas.data(), 0);

        // Reset the current atlas
        currentX = 0;
        currentY = 0;

        // Clear the atlas data
        std::fill(atlas.begin(), atlas.end(), 0);
      }
    }

    // Copy the current image to the texture atlas
    for (int y = 0; y < height; ++y) {
      for (int x = 0; x < width; ++x) {
        for (int c = 0; c < 3; ++c) {
          atlas[((currentY + y) * atlasWidth + (currentX + x)) * 3 + c] =
              imageData[(y * width + x) * channels + c];
        }
      }
    }

    // Update the position for the next image
    currentX += width;

    // Check if we need to move to the next row
    if (currentX + width > atlasWidth) {
      currentX = 0;
      currentY += height;
    }
  }

  return atlas;
}

int main(int argc, char **argv) {

  std::string directoryPath = argv[1];

  // Size of the texture atlas
  printf("hello %d\n", std::stoi(argv[2]));

  int atlasWidth = std::stoi(argv[2]);
  int atlasHeight = std::stoi(argv[2]);

  // Create the texture atlas
  std::vector<unsigned char> textureAtlas =
      createTextureAtlas(directoryPath, atlasWidth, atlasHeight);

  // Save the texture atlas to a PNG file using stb_image_write
  stbi_write_png("texture_atlas.png", atlasWidth, atlasHeight, 3,
                 textureAtlas.data(), 0);

  std::cout << "Texture atlas created successfully." << std::endl;

  return 0;
}