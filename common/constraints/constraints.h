#ifndef __Common_Constraint_H__
#define __Common_Constraint_H__

#include "../utils/utils.h"
#include "../plane/plane.h"

namespace Filtering {

// internal CPU type list, for choosing best processor
enum {
  CPU_NONE   = 0,
  //CPU_MMX = 0x01,
  //CPU_ISSE = 0x02,
  CPU_SSE2   = 0x04,
  CPU_SSE3   = 0x08,
  //CPU_3DNOW = 0x10,
  //CPU_3DNOW2 = 0x20,
  CPU_SSSE3  = 0x40,
  CPU_SSE4_1 = 0x80,
  CPU_SSE4_2 = 0x100,
  CPU_AVX    = 0x200,
  CPU_AVX2   = 0x400,
  CPU_NEON   = 0x800, // aarch64
};

typedef int CpuFlags;

typedef enum {

   MODULO_NONE = 0,
   MODULO_2    = 1,
   MODULO_4    = 3,
   MODULO_8    = 7,
   MODULO_16   = 15,
   MODULO_32   = 31,

} Modulo;

Modulo toModulo(int64_t n);

// Registration: Constraint(flags), Constraint(flags, nMinimumWidth) or Constraint(flags, MODULO_x, nMinimumWidth);
// a processor is used for a plane when the flags are available, the plane width is a multiple of moduloX and at
// least nMinimumWidth (the kernel's prerequisite, commented there when non-trivial).
class Constraint {

   CpuFlags    flags;
   Modulo      moduloX;
   int         nMinimumWidth;

public:

   Constraint();
   explicit Constraint(CpuFlags flags, int nMinimumWidth = 1);
   Constraint(CpuFlags flags, Modulo moduloX, int nMinimumWidth);
   Constraint(CpuFlags flags, const Plane<const uint8_t> &plane);
   Constraint(CpuFlags flags, const Plane<uint8_t> &plane);
   Constraint(const Constraint &constraint, const Plane<const uint8_t> &plane);
   Constraint(const Constraint &constraint, const Plane<uint8_t> &plane);
   bool respect(const Constraint &constraint) const;
   void print() const;
};

template<class P>
class Processor {

    P           *processor;
    int         nSpeed;
    Constraint  constraint;

public:

    Processor() : processor(NULL), nSpeed(-1) { }
    Processor(P *processor) : processor(processor), nSpeed(0), constraint(CPU_NONE) { }
    Processor(P *processor, const Constraint &constraint, int nSpeed) : processor(processor), nSpeed(nSpeed), constraint(constraint) { }
    bool respect(const Constraint &_constraint) const { return this->constraint.respect(_constraint); }
    int speed() const { return nSpeed; }
    P* getFunction() const { return processor; }
    void print() const { constraint.print(); }

};

template<class P>
class ProcessorList : public std::list<Processor<P>>
{

public:

   P *best_processor(const Constraint &constraint) const 
   {
      int best_speed = -1;
      const Processor<P> *best_proc;

      for(const auto &processor: *this) {
          if (processor.speed() > best_speed && processor.respect(constraint)) {
              best_speed = processor.speed();
              best_proc = &processor;
          }
      }

      return best_proc->getFunction();
   }
};

} // namespace Filtering

#endif
