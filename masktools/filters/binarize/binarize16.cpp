#include "binarize.h"
#include "binarize16_c.h"

namespace Filtering { namespace MaskTools { namespace Filters { namespace Binarize {

#define DEFINE_PROCESSORS(layout,bits_per_pixel) \
Processor16 *binarize_upper_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_upper< bits_per_pixel >>; \
Processor16 *binarize_lower_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_lower< bits_per_pixel >>; \
Processor16 *binarize_0_x_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_0_x< bits_per_pixel >>; \
Processor16 *binarize_t_x_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_t_x< bits_per_pixel >>; \
Processor16 *binarize_x_0_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_x_0< bits_per_pixel >>; \
Processor16 *binarize_x_t_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_x_t< bits_per_pixel >>; \
Processor16 *binarize_t_0_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_t_0< bits_per_pixel >>; \
Processor16 *binarize_0_t_##layout##_##bits_per_pixel##_c   = &binarize_##layout##_t<binarize_0_t< bits_per_pixel >>; \
Processor16 *binarize_x_255_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_x_255< bits_per_pixel >>; \
Processor16 *binarize_t_255_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_t_255< bits_per_pixel >>; \
Processor16 *binarize_255_x_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_255_x< bits_per_pixel >>; \
Processor16 *binarize_255_t_##layout##_##bits_per_pixel##_c = &binarize_##layout##_t<binarize_255_t< bits_per_pixel >>; \
 \

DEFINE_PROCESSORS(native,10)
DEFINE_PROCESSORS(native,12)
DEFINE_PROCESSORS(native,14)
DEFINE_PROCESSORS(native,16)

} } } }