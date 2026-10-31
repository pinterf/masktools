#include "constraints.h"

namespace Filtering {

Modulo toModulo(int64_t n)
{
   if ( !(n & MODULO_32) )
      return MODULO_32;
   else if ( !(n & MODULO_16) )
      return MODULO_16;
   else if ( !(n & MODULO_8) )
      return MODULO_8;
   else if ( !(n & MODULO_4) )
      return MODULO_4;
   else if ( !(n & MODULO_2) )
      return MODULO_2;
   else
      return MODULO_NONE;
}

static Modulo operator &(Modulo m1, Modulo m2) { return toModulo( ~(static_cast<int>(m1) & static_cast<int>(m2)) ); }

Alignment toAlignment(int64_t n)
{
   if ( !(n & ALIGNMENT_32) )
      return ALIGNMENT_32;
   else if ( !(n & ALIGNMENT_16) )
      return ALIGNMENT_16;
   else if ( !(n & ALIGNMENT_8) )
      return ALIGNMENT_8;
   else if ( !(n & ALIGNMENT_4) )
      return ALIGNMENT_4;
   else if ( !(n & ALIGNMENT_2) )
      return ALIGNMENT_2;
   else
      return ALIGNMENT_NONE;
}

static Alignment operator &(Alignment a1, Alignment a2) { return toAlignment( ~(static_cast<int>(a1) & static_cast<int>(a2)) ); }

Constraint::Constraint() : flags(CPU_NONE) { }

Constraint::Constraint(CpuFlags flags, Modulo moduloX, Modulo moduloY, Alignment alignment, int nMinimumWidth) :
flags(flags), moduloX(moduloX), moduloY(moduloY), alignment(alignment), nMinimumWidth(nMinimumWidth) { }

Constraint::Constraint(CpuFlags flags, int nWidth, int nHeight, const uint8_t *pAddress, ptrdiff_t nPitch) :
    flags(flags), moduloX(toModulo(nWidth)), moduloY(toModulo(nHeight)), alignment(toAlignment(int64_t(pAddress) | nPitch)), nMinimumWidth(nWidth) { }

Constraint::Constraint(CpuFlags flags, int nWidth, int nHeight, int64_t nAddress, ptrdiff_t nPitch) :
flags(flags), moduloX(toModulo(nWidth)), moduloY(toModulo(nHeight)), alignment(toAlignment(nAddress | nPitch)), nMinimumWidth(nWidth) { }

Constraint::Constraint(CpuFlags flags, const Plane<const uint8_t> &plane) :
flags(flags), moduloX(toModulo(plane.width())), moduloY(toModulo(plane.height())), alignment(toAlignment(int64_t(static_cast<const uint8_t*>(plane.data())) | plane.pitch())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(CpuFlags flags, const Plane<uint8_t> &plane) :
flags(flags), moduloX(toModulo(plane.width())), moduloY(toModulo(plane.height())), alignment(toAlignment(int64_t(static_cast<const uint8_t*>(plane.data())) | plane.pitch())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(const Constraint &constraint, const Plane<uint8_t> &plane) :
flags(constraint.flags), moduloX(constraint.moduloX & toModulo(plane.width())), moduloY(constraint.moduloY & toModulo(plane.height())), alignment(constraint.alignment & toAlignment(int64_t(static_cast<const uint8_t*>(plane.data())) | plane.pitch())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(const Constraint &constraint, const Plane<const uint8_t> &plane) :
flags(constraint.flags), moduloX(constraint.moduloX & toModulo(plane.width())), moduloY(constraint.moduloY & toModulo(plane.height())), alignment(constraint.alignment & toAlignment(int64_t(static_cast<const uint8_t*>(plane.data())) | plane.pitch())), nMinimumWidth(plane.width()) {}

bool Constraint::respect(const Constraint &constraint) const
{
   return ((flags & constraint.flags) == flags) && (moduloX <= constraint.moduloX) && 
          (moduloY <= constraint.moduloY) && (alignment <= constraint.alignment) && (nMinimumWidth <= constraint.nMinimumWidth);
}

void Constraint::print() const
{
   Filtering::print(MAX_LOG_LEVEL, "flags : 0x%02x --- modulo : 0x%02x 0x%02x --- alignment : 0x%02x", flags, moduloX, moduloY, alignment);
}

}
