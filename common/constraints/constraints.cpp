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

Constraint::Constraint() : flags(CPU_NONE), moduloX(MODULO_NONE), nMinimumWidth(0) { }

Constraint::Constraint(CpuFlags flags, int nMinimumWidth) :
flags(flags), moduloX(MODULO_NONE), nMinimumWidth(nMinimumWidth) { }

Constraint::Constraint(CpuFlags flags, Modulo moduloX, int nMinimumWidth) :
flags(flags), moduloX(moduloX), nMinimumWidth(nMinimumWidth) { }

Constraint::Constraint(CpuFlags flags, const Plane<const uint8_t> &plane) :
flags(flags), moduloX(toModulo(plane.width())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(CpuFlags flags, const Plane<uint8_t> &plane) :
flags(flags), moduloX(toModulo(plane.width())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(const Constraint &constraint, const Plane<uint8_t> &plane) :
flags(constraint.flags), moduloX(constraint.moduloX & toModulo(plane.width())), nMinimumWidth(plane.width()) {}

Constraint::Constraint(const Constraint &constraint, const Plane<const uint8_t> &plane) :
flags(constraint.flags), moduloX(constraint.moduloX & toModulo(plane.width())), nMinimumWidth(plane.width()) {}

bool Constraint::respect(const Constraint &constraint) const
{
   return ((flags & constraint.flags) == flags) && (moduloX <= constraint.moduloX) &&
          (nMinimumWidth <= constraint.nMinimumWidth);
}

void Constraint::print() const
{
   Filtering::print(MAX_LOG_LEVEL, "flags : 0x%02x --- modulo : 0x%02x --- minimum width : %d", flags, moduloX, nMinimumWidth);
}

}
