#include "texture-utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <iostream>

our::Texture2D* our::texture_utils::empty(GLenum format, glm::ivec2 size){
    our::Texture2D* texture = new our::Texture2D();
    //TODO: (Req 11) Finish this function to create an empty texture with the given size and format
    glBindTexture(GL_TEXTURE_2D, texture->getOpenGLName());
    GLenum pixel_format;
    GLenum pixel_type;

    switch(format){
        case GL_RGBA8:
            pixel_format = GL_RGBA;
            pixel_type = GL_UNSIGNED_BYTE;
            break;
        case GL_DEPTH_COMPONENT24:
            pixel_format = GL_DEPTH_COMPONENT;
            pixel_type = GL_FLOAT; 
            break;
        default:
            pixel_format = GL_RGB;
            pixel_type = GL_UNSIGNED_BYTE;
            break;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, pixel_format, pixel_type, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);

    return texture;
}

our::Texture2D* our::texture_utils::loadImage(const std::string& filename, bool generate_mipmap) {
    glm::ivec2 size;
    int channels;
    //Since OpenGL puts the texture origin at the bottom left while images typically has the origin at the top left,
    //We need to till stb to flip images vertically after loading them
    stbi_set_flip_vertically_on_load(true);
    //Load image data and retrieve width, height and number of channels in the image
    //The last argument is the number of channels we want and it can have the following values:
    //- 0: Keep number of channels the same as in the image file
    //- 1: Grayscale only
    //- 2: Grayscale and Alpha
    //- 3: RGB
    //- 4: RGB and Alpha (RGBA)
    //Note: channels (the 4th argument) always returns the original number of channels in the file
    unsigned char* pixels = stbi_load(filename.c_str(), &size.x, &size.y, &channels, 4);
    if(pixels == nullptr){
        std::cerr << "Failed to load image: " << filename << std::endl;
        return nullptr;
    }
    // Create a texture
    our::Texture2D* texture = new our::Texture2D();
    //Bind the texture such that we upload the image data to its storage
    //TODO: (Req 5) Finish this function to fill the texture with the data found in "pixels"
    texture->bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if(generate_mipmap){
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    our::Texture2D::unbind();
    
    stbi_image_free(pixels); //Free image data after uploading to GPU
    return texture;
}