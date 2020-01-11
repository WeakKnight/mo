#pragma once

#include "glad/glad.h"
#include "common.h"

class Texture
{
public:
    Texture()
    {
    }

    ~Texture()
    {
        if (created)
        {
            glDeleteTextures(1, &ID);
        }
    }

    operator unsigned int() const
    {
        return ID;
    }

    void SetData2D(unsigned int width, unsigned int height, int internalFormat, int format, int dataType, void *data)
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
        glTexImage2D(target, 0, internalFormat, width, height, 0, format, dataType, data);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterMin);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterMag);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapT);
        if (isMipmapped)
        {
            glGenerateMipmap(target);
        }
        Unbind();
    }

    void Bind(int texUnit = 0)
    {
        if (texUnit > 0)
        {
            glActiveTexture(GL_TEXTURE0 + texUnit);
        }
        glBindTexture(target, ID);
    }

    void Unbind()
    {
        glBindTexture(target, 0);
    }

    int target = GL_TEXTURE_2D;

    int internalFormat = GL_RGBA;
    int format = GL_RGBA;
    int dataType = GL_UNSIGNED_BYTE;

    int filterMin = GL_LINEAR_MIPMAP_LINEAR;
    int filterMag = GL_LINEAR;

    int wrapR = GL_REPEAT;
    int wrapS = GL_REPEAT;
    int wrapT = GL_REPEAT;

    bool isMipmapped = true;
    bool created = false;

private:
    unsigned int ID;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int depth = 0;
};