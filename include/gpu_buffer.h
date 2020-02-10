#pragma once
#include "common.h"

enum class BUFFER_DRAW_TYPE
{
    STATIC_DRAW,
    DYNAMIC_DRAW,
    STREAM_DRAW
};

enum class BUFFER_USAGE
{
    ARRAY,
    ELEMENT,
    UNIFORM
};

class GPUBuffer
{
public:
    GPUBuffer();
    ~GPUBuffer();

    void BindBuffer(BUFFER_USAGE usage);
    void UnBindBuffer(BUFFER_USAGE usage);
    void SetData(BUFFER_USAGE usage, void* data, unsigned int size, BUFFER_DRAW_TYPE drawType);

    void BindArrayBuffer();
    void UnBindArrayBuffer();
    void SetDataArrayBuffer(void* data, unsigned int size, BUFFER_DRAW_TYPE drawType);

    void BindElementBuffer();
    void UnBindElementBuffer();
    void SetDataElementBuffer(void* data, unsigned int size, BUFFER_DRAW_TYPE drawType);

private:
    unsigned int ID;
    unsigned int size = 0;
};