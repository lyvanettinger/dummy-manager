#pragma once

struct SceneResources // TEMP
{
    matrix MVP;
};

struct RenderResources
{
    matrix MVP;
    uint positionBufferIndex;
    uint normalBufferIndex;
    uint uvBufferindex;
    uint textureBufferIndex;
};