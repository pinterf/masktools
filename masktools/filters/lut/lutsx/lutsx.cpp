#include "lutsx.h"
#include "../functions.h"

using namespace Filtering;

template<class T, class U>
static void custom_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, const uint8_t *pLut, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2)
{
   T new_value1( mode1 );
   U new_value2( mode2 );
   for ( int j = 0; j < nHeight; j++ )
   {
      for ( int i = 0; i < nWidth; i++ )
      {
         new_value1.reset();
         new_value2.reset();
         for ( int k = 0; k < nCoordinates; k+=2 )
         {
            int x = pCoordinates[k] + i;
            int y = pCoordinates[k+1] + j;

            if ( x < 0 ) x = 0;
            if ( x >= nWidth ) x = nWidth - 1;
            if ( y < 0 ) y = 0;
            if ( y >= nHeight ) y = nHeight - 1;

            new_value1.add( pSrc1[x + ( y - j ) * nSrc1Pitch] );
            new_value2.add( pSrc2[x + ( y - j ) * nSrc2Pitch] );
         }
         pDst[i] = pLut[ (new_value2.finalize() << 16) + (pDst[ i ] << 8) + new_value1.finalize() ]; // ZXY order at lut fill-up: x=pDst, Y=val1 Z=val2
      }
      pSrc1 += nSrc1Pitch;
      pSrc2 += nSrc2Pitch;
      pDst += nDstPitch;
   }
}

template<class T, class U>
static void custom_realtime_8_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2)
{
  T new_value1(mode1);
  U new_value2(mode2);
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value1.reset();
      new_value2.reset();
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        new_value1.add(pSrc1[x + (y - j) * nSrc1Pitch]);
        new_value2.add(pSrc2[x + (y - j) * nSrc2Pitch]);
      }
      pDst[i] = ctx->compute_byte_xyz(pDst[i], new_value1.finalize(), new_value2.finalize());
    }
    pSrc1 += nSrc1Pitch;
    pSrc2 += nSrc2Pitch;
    pDst += nDstPitch;
  }
}


template<bool realtime, int bits_per_pixel, class T, class U>
static void custom_realtime_uint16_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2)
{
  T new_value1(mode1);
  U new_value2(mode2);
  const uint16_t *pSrc1_16 = reinterpret_cast<const uint16_t *>(pSrc1);
  const uint16_t *pSrc2_16 = reinterpret_cast<const uint16_t *>(pSrc2);
  uint16_t *pDst_16 = reinterpret_cast<uint16_t *>(pDst);
  nSrc1Pitch /= sizeof(uint16_t);
  nSrc2Pitch /= sizeof(uint16_t);
  nDstPitch /= sizeof(uint16_t);
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value1.reset();
      new_value2.reset();
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        new_value1.add(pSrc1_16[x + (y - j) * nSrc1Pitch]);
        new_value2.add(pSrc2_16[x + (y - j) * nSrc2Pitch]);
      }
      pDst_16[i] = ctx->compute_word_xyz(pDst_16[i], new_value1.finalize(), new_value2.finalize(), bits_per_pixel);
    }
    pSrc1_16 += nSrc1Pitch;
    pSrc2_16 += nSrc2Pitch;
    pDst_16 += nDstPitch;
  }
}

template<class T, class U>
static void custom_realtime_32_c(uint8_t *pDst, ptrdiff_t nDstPitch, const uint8_t *pSrc1, ptrdiff_t nSrc1Pitch, const uint8_t *pSrc2, ptrdiff_t nSrc2Pitch, Parser::Context *ctx, 
  const int *pCoordinates, int nCoordinates, int nWidth, int nHeight, const String &mode1, const String &mode2, bool chroma)
{
  T new_value1(mode1);
  U new_value2(mode2);
  const float *pSrc1_32 = reinterpret_cast<const float *>(pSrc1);
  const float *pSrc2_32 = reinterpret_cast<const float *>(pSrc2);
  float *pDst_32 = reinterpret_cast<float *>(pDst);
  nSrc1Pitch /= sizeof(float);
  nSrc2Pitch /= sizeof(float);  
  nDstPitch /= sizeof(float);
  for (int j = 0; j < nHeight; j++)
  {
    for (int i = 0; i < nWidth; i++)
    {
      new_value1.reset();
      new_value2.reset();
      for (int k = 0; k < nCoordinates; k += 2)
      {
        int x = pCoordinates[k] + i;
        int y = pCoordinates[k + 1] + j;

        if (x < 0) x = 0;
        if (x >= nWidth) x = nWidth - 1;
        if (y < 0) y = 0;
        if (y >= nHeight) y = nHeight - 1;

        new_value1.add(pSrc1_32[x + (y - j) * nSrc1Pitch]);
        new_value2.add(pSrc2_32[x + (y - j) * nSrc2Pitch]);
      }
      pDst_32[i] = (float)ctx->compute_float_xyz(pDst_32[i], new_value1.finalize(), new_value2.finalize(), chroma);
    }
    pSrc1_32 += nSrc1Pitch;
    pSrc2_32 += nSrc2Pitch;
    pDst_32 += nDstPitch;
  }
}


namespace Filtering { namespace MaskTools { namespace Filters { namespace Lut { namespace SpatialExtended {

Processor *processors_array[NUM_MODES][NUM_MODES] = 
{
   MPROCESSOR_DUAL( custom_c, Nonizer ),
   MPROCESSOR_DUAL( custom_c, Averager<int>  ),
   MPROCESSOR_DUAL( custom_c, Minimizer ),
   MPROCESSOR_DUAL( custom_c, Maximizer ),
   MPROCESSOR_DUAL( custom_c, Deviater<int> ),
   MPROCESSOR_DUAL( custom_c, Rangizer ),
   MPROCESSOR_DUAL( custom_c, Medianizer ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<4> ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<6> ),
   MPROCESSOR_DUAL( custom_c, MedianizerBetter<2> ),
};

ProcessorCtx *processors_realtime_8_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR_DUAL(custom_realtime_8_c, Nonizer),
  MPROCESSOR_DUAL(custom_realtime_8_c, Averager<int>),
  MPROCESSOR_DUAL(custom_realtime_8_c, Minimizer),
  MPROCESSOR_DUAL(custom_realtime_8_c, Maximizer),
  MPROCESSOR_DUAL(custom_realtime_8_c, Deviater<int>),
  MPROCESSOR_DUAL(custom_realtime_8_c, Rangizer),
  MPROCESSOR_DUAL(custom_realtime_8_c, Medianizer),
  MPROCESSOR_DUAL(custom_realtime_8_c, MedianizerBetter<4>),
  MPROCESSOR_DUAL(custom_realtime_8_c, MedianizerBetter<6>),
  MPROCESSOR_DUAL(custom_realtime_8_c, MedianizerBetter<2>),
};

ProcessorCtx *processors_realtime_10_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Nonizer16<10>, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Averager16, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Minimizer16<10>, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Maximizer16, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Deviater16, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Rangizer16<10>, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Medianizer16, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<4>, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<6>, true, 10),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<2>, true, 10),
};

ProcessorCtx *processors_realtime_12_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Nonizer16<12>, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Averager16, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Minimizer16<12>, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Maximizer16, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Deviater16, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Rangizer16<12>, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Medianizer16, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<4>, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<6>, true, 12),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<2>, true, 12),
};

ProcessorCtx *processors_realtime_14_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Nonizer16<14>, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Averager16, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Minimizer16<14>, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Maximizer16, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Deviater16, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Rangizer16<14>, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Medianizer16, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<4>, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<6>, true, 14),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<2>, true, 14),
};

ProcessorCtx *processors_realtime_16_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Nonizer16<16>, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Averager16, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Minimizer16<16>, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Maximizer16, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Deviater16, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Rangizer16<16>, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, Medianizer16, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<4>, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<6>, true, 16),
  MPROCESSOR16_DUAL(custom_realtime_uint16_c, MedianizerBetter16<2>, true, 16),
};

ProcessorCtx32 *processors_realtime_32_array[NUM_MODES][NUM_MODES] =
{
  MPROCESSOR32_DUAL(custom_realtime_32_c, Nonizer32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Averager32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Minimizer32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Maximizer32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Deviater32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Rangizer32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, Medianizer32),
  MPROCESSOR32_DUAL(custom_realtime_32_c, MedianizerBetter32<4>),
  MPROCESSOR32_DUAL(custom_realtime_32_c, MedianizerBetter32<6>),
  MPROCESSOR32_DUAL(custom_realtime_32_c, MedianizerBetter32<2>),
};

} } } } }

