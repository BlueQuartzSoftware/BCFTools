#pragma once

#include <cstdint>
#include <memory>
#include <vector>


/*
 * @brief These structs are ONLY validated against the Version 6 of the Indexing Results. This
 * version information can be found in the fie "Version" that is extract from the .bcf file.
 * Any updates to the IndexingResults format can change without notice from Bruker.
 */

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct {
    int32_t width;
    int32_t height;
    int32_t patternCount;
} FrameDescriptionHeader_t;


typedef struct
{
  int xIndex;                // The x index of this pattern
  int yIndex;                // The y index of this pattern
  int dataSize;              // The data size (Width* height * bytes per pixel + 17)
  int width;                 // The Width in pixels of the pattern
  int height;                // The Height in pixels of the pattern
  int bytesPerPixel;         // Bytes per pixel
  unsigned char pixelFormat; // Pixel Format. Can be ignored in this version.
} FrameDataHeader_t;

typedef struct
{
    uint16_t xIndex;
    uint16_t yIndex;
    float radonQuality;
    uint16_t detectedBands;
    float   euler1; // radians
    float   euler2; // radians
    float   euler3; // radians
    int16_t phase; //Has byte value of 0xFFFF
    uint16_t indexedBands;
    float   bmm;
} IndexResult_t;

#pragma pack(pop)   /* restore original alignment from stack */



