#include "luts.h"
#include "../functions.h"

using namespace Filtering;

//similar template to lutf
template<bool realtime, class T>
static void custom_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode)
{
   UNUSED(pLut_w);
   UNUSED(ctx_w);

   T new_value( mode );
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
      {
         new_value.reset();
         for ( int k = 0; k < nCoordinates; k+=2 )
         {
            int x = pCoordinates[k] + i;
            int y = pCoordinates[k+1] + j;

            if ( x < 0 ) x = 0;
            if ( x >= nWidth ) x = nWidth - 1;
            if ( y < 0 ) y = 0;
            if ( y >= nHeight ) y = nHeight - 1;

            int PixelX = pDst[i];
            int PixelY = pSrc[x + (y - j) * nSrcPitch];

            if (realtime) {
              new_value.add(ctx->compute_byte_xy(PixelX, PixelY));
            }
            else {
              new_value.add(pLut[(PixelX << 8) + PixelY]);
            }
         }
         pDst[i] = new_value.finalize();
      }
      pSrc += nSrcPitch;
      pDst += nDstPitch;
   }
}

template<bool realtime, class T>
static void custom_weight_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode)
{
  T new_value(mode);
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value.reset_w(); // different from non-weight version
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        int PixelX = pDst[i];
        int PixelY = pSrc[x + (y - j) * nSrcPitch];

        // different from non-weight version
        float weight;
        if (realtime) {
          weight = ctx_w->compute_float_xy_intinput<8>(PixelX, PixelY); // yes, weights are float, keep precision and not convert back to int
          new_value.add_w(ctx->compute_byte_xy(PixelX, PixelY), weight);
        }
        else {
          weight = pLut_w[(PixelX << 8) + PixelY]; // byte xy but float content!
          new_value.add_w(pLut[(PixelX << 8) + PixelY], weight);
        }
      }
      pDst[i] = new_value.finalize_w(); // different from non-weight version
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}

template<bool realtime, int bits_per_pixel, class T>
static void custom16_c(uint8_t *pDst8, ptrdiff_t nDstPitch, const uint8_t *pSrc8, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode)
{
  UNUSED(pLut_w);
  UNUSED(ctx_w);

  T new_value(mode);

  const uint16_t *pSrc = reinterpret_cast<const uint16_t *>(pSrc8);
  uint16_t *pDst = reinterpret_cast<uint16_t *>(pDst8);
  nSrcPitch /= sizeof(uint16_t);
  nDstPitch /= sizeof(uint16_t);
  const int max_pixel_value = (1 << bits_per_pixel) - 1;

  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value.reset();
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        int PixelX = pDst[i];
        int PixelY = pSrc[x + (y - j) * nSrcPitch];

        if (bits_per_pixel < 16) {
          PixelX = min(PixelX, max_pixel_value);
          PixelY = min(PixelY, max_pixel_value);
        }

        if (realtime) {
          new_value.add(ctx->compute_word_xy<bits_per_pixel>(PixelX, PixelY));
        }
        else {
          new_value.add(reinterpret_cast<const uint16_t *>(pLut)[(PixelX << bits_per_pixel) + PixelY]);
        }
      }
      pDst[i] = new_value.finalize(); // cannot overflow
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}

//similar template to lutf
template<bool realtime, int bits_per_pixel, class T>
static void custom16_weight_c(uint8_t *pDst8, ptrdiff_t nDstPitch, const uint8_t *pSrc8, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode)
{
  T new_value(mode);

  const uint16_t *pSrc = reinterpret_cast<const uint16_t *>(pSrc8);
  uint16_t *pDst = reinterpret_cast<uint16_t *>(pDst8);
  nSrcPitch /= sizeof(uint16_t);
  nDstPitch /= sizeof(uint16_t);
  const int max_pixel_value = (1 << bits_per_pixel) - 1;

  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value.reset_w(); // different from non-weight version
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        int PixelX = pDst[i];
        int PixelY = pSrc[x + (y - j) * nSrcPitch];

        if (bits_per_pixel < 16) {
          PixelX = min(PixelX, max_pixel_value);
          PixelY = min(PixelY, max_pixel_value);
        }

        // different from non-weight version
        float weight;
        if (realtime) {
          weight = ctx_w->compute_float_xy_intinput<bits_per_pixel>(PixelX, PixelY); // keep precision and not convert back to int
        }
        else {
          weight = pLut_w[(PixelX << bits_per_pixel) + PixelY]; // byte xy but float content!
        }

        if (realtime) {
          new_value.add_w(ctx->compute_word_xy<bits_per_pixel>(PixelX, PixelY), weight);
        }
        else {
          new_value.add_w(reinterpret_cast<const uint16_t *>(pLut)[(PixelX << bits_per_pixel) + PixelY], weight);
        }
      }
      if(bits_per_pixel == 16)
        pDst[i] = new_value.finalize_w();  // different from non-weight version
      else
        pDst[i] = min(new_value.finalize_w(), (uint16_t)max_pixel_value);
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}


//similar template to lutf
template<class T>
static void custom32_c(uint8_t *pDst8, ptrdiff_t nDstPitch, const uint8_t *pSrc8, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, 
  const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode, bool chroma)
{
  UNUSED(pLut);
  UNUSED(pLut_w);
  UNUSED(ctx_w);

  T new_value(mode);

  const float *pSrc = reinterpret_cast<const float *>(pSrc8);
  float *pDst = reinterpret_cast<float *>(pDst8);
  nSrcPitch /= sizeof(float);
  nDstPitch /= sizeof(float);

  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value.reset();
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        float PixelX = pDst[i];
        float PixelY = pSrc[x + (y - j) * nSrcPitch];

        // float is always realtime
        new_value.add(ctx->compute_float_xy(PixelX, PixelY, chroma));
      }
      pDst[i] = new_value.finalize();
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}

template<class T>
static void custom32_weight_c(uint8_t *pDst8, ptrdiff_t nDstPitch, const uint8_t *pSrc8, ptrdiff_t nSrcPitch, const uint8_t *pLut, const float *pLut_w, Parser::Context *ctx, Parser::Context *ctx_w, 
  const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode, bool chroma)
{
  UNUSED(pLut);
  UNUSED(pLut_w);

  T new_value(mode);

  const float *pSrc = reinterpret_cast<const float *>(pSrc8);
  float *pDst = reinterpret_cast<float *>(pDst8);
  nSrcPitch /= sizeof(float);
  nDstPitch /= sizeof(float);

  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value.reset_w(); // different from non-weight version
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        float PixelX = pDst[i];
        float PixelY = pSrc[x + (y - j) * nSrcPitch];

        // different from non-weight version
        float weight = ctx_w->compute_float_xy(PixelX, PixelY, chroma);

        // float is always realtime
        new_value.add_w(ctx->compute_float_xy(PixelX, PixelY, chroma), weight);
      }
      pDst[i] = new_value.finalize_w(); // different from non-weight version
    }
    pSrc += nSrcPitch;
    pDst += nDstPitch;
  }
}

namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace Spatial {

Processor *processors_array[NUM_MODES] = MPROCESSOR_SINGLE( custom_c, false );
Processor *processors_10_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, false, 10);
Processor *processors_12_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, false, 12);
Processor *processors_14_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, false, 14);
Processor *processors_16_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, false, 16);

Processor *processors_weight_array[NUM_MODES] = MPROCESSOR_SINGLE(custom_weight_c, false);
Processor *processors_weight_10_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, false, 10);
Processor *processors_weight_12_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, false, 12);
Processor *processors_weight_14_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, false, 14);
Processor *processors_weight_16_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, false, 16);


Processor *processors_realtime_8_array[NUM_MODES] = MPROCESSOR_SINGLE(custom_c, true);
Processor *processors_realtime_10_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, true, 10);
Processor *processors_realtime_12_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, true, 12);
Processor *processors_realtime_14_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, true, 14);
Processor *processors_realtime_16_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_c, true, 16);
Processor32 *processors_realtime_32_array[NUM_MODES] = MPROCESSOR32_SINGLE(custom32_c);

Processor *processors_weight_realtime_8_array[NUM_MODES] = MPROCESSOR_SINGLE(custom_weight_c, true);
Processor *processors_weight_realtime_10_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, true, 10);
Processor *processors_weight_realtime_12_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, true, 12);
Processor *processors_weight_realtime_14_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, true, 14);
Processor *processors_weight_realtime_16_array[NUM_MODES] = MPROCESSOR16_SINGLE(custom16_weight_c, true, 16);
Processor32 *processors_weight_realtime_32_array[NUM_MODES] = MPROCESSOR32_SINGLE(custom32_weight_c);

} } } } }

