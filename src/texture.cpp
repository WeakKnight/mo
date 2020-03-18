#pragma once
#include "texture.h"
#ifndef __glad_h_
#include <glad/glad.h>
#endif

Texture::Texture()
{
    ID = 0;
    target = GL_TEXTURE_2D;

    internalFormat = GL_RGBA;
    format = GL_RGBA;
    dataType = GL_UNSIGNED_BYTE;

    filterMin = GL_LINEAR;
    filterMag = GL_LINEAR;

    wrapR = GL_CLAMP_TO_EDGE;
    wrapS = GL_CLAMP_TO_EDGE;
    wrapT = GL_CLAMP_TO_EDGE;
}

Texture::~Texture()
{
    if (created)
    {
        spdlog::info("texture destroyed");
        glDeleteTextures(1, &ID);
    }
}

void Texture::SetData2D(unsigned int width, unsigned int height, int internalFormat, int format, int dataType, void *data)
{
    glGenTextures(1, &ID);
    created = true;

    this->width = width;
    this->height = height;
    this->depth = 0;
    this->internalFormat = internalFormat;
    this->format = format;
    this->dataType = dataType;

    assert(target == GL_TEXTURE_2D);

    Bind();
    
   
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterMag);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);

    if (isMipmapped)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMin);
       // glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(target, 0, internalFormat, width, height, 0, format, dataType, data);
        glGenerateMipmap(target);
    }
    else
    {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterMin);
        glTexImage2D(target, 0, internalFormat, width, height, 0, format, dataType, data);
    }

    Unbind();
}

void Texture::SetDataCubeMap(unsigned int width, unsigned int height, int internalFormat, int format, int dataType, const std::vector<void*>& datas)
{
    glGenTextures(1, &ID);
    created = true;

    this->target = GL_TEXTURE_CUBE_MAP;

    this->width = width;
    this->height = height;
    this->depth = 0;
    this->internalFormat = internalFormat;
    this->format = format;
    this->dataType = dataType;

    Bind();

    assert(datas.size() == 6);

    for (int i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, dataType, datas[i]);
    }

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterMag);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(target, GL_TEXTURE_WRAP_R, wrapR);

    if (isMipmapped)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filterMin);
        glGenerateMipmap(target);
    }
    else
    {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterMin);
    }

    Unbind();
}

void Texture::Bind(int texUnit)
{
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(target, ID);
}

void Texture::Unbind()
{
    glBindTexture(target, 0);
}

void Texture::SetAnisotropy(float anisotropy)
{
    Bind();
    glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
    Unbind();
}

void Texture::Resize(unsigned int width, unsigned int height)
{
    assert(target == GL_TEXTURE_2D);
    Bind();
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, 0);
    Unbind();
}

unsigned int Texture::AsID() const
{
    return ID;
}