#ifndef __Common_Avs2x_Filter_H__
#define __Common_Avs2x_Filter_H__

#include "params.h"
#include "../common/constraints/constraints.h"
#include <avs/cpuid.h>

namespace Filtering { namespace Avisynth2x {

static CpuFlags AvsToInternalCpuFlags(int avsCpuFlags) {
  int flags = CPU_NONE;
#ifdef INTEL_INTRINSICS
  // the CPUF_ values of x86 and ARM overlap (CPUF_ARM_NEON == CPUF_FORCE): x86 builds only
  if (avsCpuFlags & CPUF_AVX2) flags |= CPU_AVX2;
  if (avsCpuFlags & CPUF_AVX) flags |= CPU_AVX;
  if (avsCpuFlags & CPUF_SSE4_2) flags |= CPU_SSE4_2;
  if (avsCpuFlags & CPUF_SSE4_1) flags |= CPU_SSE4_1;
  if (avsCpuFlags & CPUF_SSSE3) flags |= CPU_SSSE3;
  if (avsCpuFlags & CPUF_SSE3) flags |= CPU_SSE3;
  if (avsCpuFlags & CPUF_SSE2) flags |= CPU_SSE2;
#elif defined(NEON_INTRINSICS)
  if (avsCpuFlags & CPUF_ARM_NEON) flags |= CPU_NEON;
#else
  UNUSED(avsCpuFlags);
#endif
  return flags;
}


template<class T>
class Filter : public GenericVideoFilter
{
    T _filter;
    Signature signature;
    int inputConfigSize; // to prevent MT static init problems with /Zc:threadSafeInit- settings for WinXP

    static AVSValue __cdecl _create(AVSValue args, void *user_data, IScriptEnvironment *env)
    {
      UNUSED(user_data);
      // pass "Expr" existance to override "use_expr" parameter value for old avisynth versions
      const bool has_avs_expr_support = env->FunctionExists("Expr");
      return new Filter<T>(args[0].AsClip(), GetParameters(args, T::filter_signature(), has_avs_expr_support, env), env);
    }
public:
    Filter(::PClip child, const Parameters &parameters, IScriptEnvironment *env) : GenericVideoFilter(child), _filter(parameters, AvsToInternalCpuFlags(env->GetCPUFlags())), signature(T::filter_signature())
    {
        inputConfigSize = _filter.input_configuration().size();
        // This is a warning left here intentionally, why multithreading and threadSafeInit- for winXp causes big troubles sometimes.
        // When the above line is missing, problems kick in _filter.get_frame(n, destination, env)
        // Why: "input_configuration" has static initializer that has problems in multithreaded environment
        // when the XP compatible /Zc:threadSafeInit- switch is used for compiling in Visual Studio
        // https://docs.microsoft.com/en-us/cpp/build/reference/zc-threadsafeinit-thread-safe-local-static-initialization
        // In non-threadSafeInit mode (XP) when get_frame is called in multithreaded environment,
        // the initialization is started in thread#1 and at specific timing conditions (e.g. debug mode is not OK) this initialization is not finished yet when
        // thread#2 also calls into input_configuration().size().
        // Real life problems: the proper size value is "2", but for thread#2 still "0" is reported!
        // This resulted in zero sized local PVideoFrame array to be allocated, but later, when the initialization
        // is finished in an other thread, size() turnes into "2". It needs only some 1/10000th seconds, but the problem is there by then.
        // This zero sized array is then indexed with the proper size of "2" from 0..1 -> Access Violation
        // Debuglog: Masktools2 Getframe #1
        //  Masktools2 Getframe #0
        //  Masktools2 Getframe 0, clipcount = 0 // should be 2!!
        //  Masktools2 Getframe 0, clipcount2 = 2 // meanwhile the init was done in the background, we get 2 which is correct
        //  Masktools2 Getframe 1, clipcount = 2
        if (_filter.is_error())
        {
            env->ThrowError((signature.getName() + " : " + _filter.get_error()).c_str());
        }
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override
    {

      // v2.2.15-
      // lut, lutxy, lutxyz, lutxyza: 'use_expr' parameters option to pass expression to the Expr filter of avs+
      // which is much faster than masktools2 realtime interpreted pixel-by-pixel calculation
      // expr_need_process[4] and expr_list[4] are all filled for avs+ requirements
      // copy-plane or fill operators are also converted to valid expression strings (single "x" or like "128")
      const bool effective_expr_need_process = _filter.expr_need_process[0] || _filter.expr_need_process[1] || _filter.expr_need_process[2] || _filter.expr_need_process[3];
      if (effective_expr_need_process) {
        const int planecount = _filter.plane_count();

        // lut, lutxy, lutxyz, lutxyza: InPlaceTwoFrame: size is less by one, first clip is explicit
        int realInputConfigSize = inputConfigSize + 1;

        // Expr in Avs+3.4  : c+s+[format]s[optAvx2]b[optSingleMode]b[optSSE2]b[scale_inputs]s[clamp_float]b
        // Expr in Avs+3.4.1: c+s+[format]s[optAvx2]b[optSingleMode]b[optSSE2]b[scale_inputs]s[clamp_float]b[clamp_float_UV]b
        // Incompatibility note: Avisynth+ 3.4 and masktools <= 2.2.19 has no clamp_float_UV

        int extra_param_count;
        if (_filter.expr_clamp_float_UV)
          extra_param_count = 3; // scale_inputs, clamp_float, clamp_float_UV
        else
          extra_param_count = 2; // scale_inputs, clamp_float

        int param_length = realInputConfigSize + planecount + extra_param_count;

        const char *arg_names[4 + 4 + 3]; // worst case
        for (int i = 0; i < param_length - extra_param_count; i++)
          arg_names[i] = nullptr;
        arg_names[param_length - extra_param_count + 0] = (const char *)"scale_inputs";
        arg_names[param_length - extra_param_count + 1] = (const char *)"clamp_float";
        if (_filter.expr_clamp_float_UV) // only pass when not default false
          arg_names[param_length - extra_param_count + 2] = (const char*)"clamp_float_UV";

        std::vector<AVSValue> new_args(param_length, AVSValue());

        for (int i = 0; i < realInputConfigSize; i++)
          new_args[i] = _filter.get_childs()[i]->get_avs_clip();
        // Expr takes a compact plane list: YA's second expression comes from our internal alpha slot 3.
        for (int i = 0; i < planecount; i++)
          new_args[i + realInputConfigSize] = _filter.expr_list[_filter.plane_index(i)].c_str();

        new_args[param_length - extra_param_count + 0] = _filter.expr_scale_inputs.c_str();
        new_args[param_length - extra_param_count + 1] = _filter.expr_clamp_float;
        if (_filter.expr_clamp_float_UV) // only pass when not default false
          new_args[param_length - extra_param_count + 2] = _filter.expr_clamp_float_UV;

        try {
          AVSValue clip_out = env->Invoke("Expr", AVSValue(new_args.data(), param_length), arg_names);
          PVideoFrame dst = clip_out.AsClip()->GetFrame(n, env);
          return dst;
        }
        catch (const IScriptEnvironment::NotFound&) {
          env->ThrowError("masktools2 error on invoking \"Expr\" call with use_expr: not found!");
        }
        catch (const AvisynthError& ae) {
          env->ThrowError("masktools2 error on invoking \"Expr\" call with use_expr: %s\n", ae.msg);
        }
        catch (...) {
          env->ThrowError("masktools2 error on invoking \"Expr\" call with use_expr: C++ exception!");
        }
      }

      // filter is not a lut or lut w/o "Expr" call
      PVideoFrame dst = _filter.is_in_place() ? 
        child->GetFrame(n, env) : 
        env->NewVideoFrame(vi);

      if (_filter.is_in_place()) {
        env->MakeWritable(&dst);
      }

      const bool need_copy_frame_properties = has_at_least_v8 && !_filter.is_in_place();

      Frame<uint8_t> destination = dynamic_cast<Clip *>((Filtering::Clip *)_filter.get_childs()[0].get())->ConvertTo<uint8_t>(dst);

      // see in common/base/filter.h
      // I have to pass dst as ref, this whole thing is very nice but was made so abstract that I'm unable to access dst from Frame<uint8_t>
      _filter.get_frame(n, destination, need_copy_frame_properties, dst, env);

      return dst;
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range) override {
        return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }

    static void create(IScriptEnvironment *env)
    {
      // float parameters are registered with an integer alternative as well: with only the float one, an older
      // masktools2 (integer-only signature) loaded earlier would be the better match for integer arguments.
      // The integer one first, the order is important.
      String signature1 = SignatureToString(T::filter_signature(), true); // integer
      env->AddFunction(T::filter_signature().getName().c_str(), signature1.c_str(), _create, NULL);
      String signature2 = SignatureToString(T::filter_signature(), false); // float
      if (signature1 != signature2)
        env->AddFunction(T::filter_signature().getName().c_str(), signature2.c_str(), _create, NULL);
    }
};

} } // namespace Avisynth2x, Filtering

#endif
