#ifndef __Common_Avs2x_Params_H__
#define __Common_Avs2x_Params_H__

#include "clip.h"
#include "../common/params/params.h"

namespace Filtering { namespace Avisynth2x {

// The host has frame properties (interface V8). Set once, in AvisynthPluginInit3.
// C++17 inline variable: a single variable in the whole plugin, even if more source files include this header.
// Not about code inlining, it stays a normal global, changeable at run time.
// C++ interface only. On a move to the C interface: frame property access crashes there in V8. Reading properties needs
// V9 and a non-NULL entry (Win32 3.7.1 is V9 with NULL entries) or V10; avs_new_video_frame_p with a non-NULL source and
// avs_prop_get_data work from V10 (3.7.3).
inline bool has_at_least_v8 = false;

static Value AVSValueToValue(const AVSValue& value, const Parameter &param)
{
    switch (param.getType())
    {
    case TYPE_INT: return Value(value.AsInt());
    case TYPE_STRING: return Value(String(value.AsString()));
    case TYPE_BOOL: return Value(value.AsBool());
    case TYPE_CLIP: return std::shared_ptr<Filtering::Clip>(new Clip(value.AsClip()));
    case TYPE_FLOAT: return Value(value.AsFloat());
    default: return Value();
    }
}

static String ParameterToString(const Parameter &parameter)
{
    String str = "";

    if (parameter.isNamed())
        str.append("[").append(parameter.getName()).append("]");

    switch (parameter.getType())
    {
    case TYPE_INT: return str.append("i");
    case TYPE_FLOAT: return str.append("f");
    case TYPE_STRING: return str.append("s");
    case TYPE_CLIP: return str.append("c");
    case TYPE_BOOL: return str.append("b");
    default: assert(0); return "";
    }
}

String SignatureToString(const Signature &signature)
{
    String str;

    for (int i = 0; i < signature.count(); i++)
      str.append(ParameterToString(signature[i]));

    return str;
}

Parameter GetParameter(const AVSValue &value, const Parameter &default_param)
{
    if (value.Defined()) {
        return Parameter(AVSValueToValue(value, default_param), default_param.getName(), false); // false: n/a
    }

    Parameter parameter = default_param;
    parameter.set_defined(false);

    return parameter;
}

// mt_merge cplace "auto": the kernel for the _ChromaLocation frame property of the first clip's frame 0, as AviSynth+
// Overlay and Layer choose it (chromaLocationToMaskPlacement). nullptr: no valid property, the format default applies.
static const char *CplaceFromFrameProps(const AVSValue &first_clip, IScriptEnvironment *env)
{
    if (!has_at_least_v8)
        return nullptr;
    ::PClip clip = first_clip.AsClip();
    const VideoInfo &vi = clip->GetVideoInfo();
    if (vi.num_frames <= 0 || !vi.IsPlanar() || !vi.IsYUV() || vi.IsY())
        return nullptr;
    const bool is440 = vi.GetPlaneWidthSubsampling(PLANAR_U) == 0 && vi.GetPlaneHeightSubsampling(PLANAR_U) == 1;
    PVideoFrame frame0 = clip->GetFrame(0, env);
    const AVSMap *props = env->getFramePropsRO(frame0);
    if (env->propNumElements(props, "_ChromaLocation") <= 0)
        return nullptr;
    int error = 0;
    const int64_t location = env->propGetInt(props, "_ChromaLocation", 0, &error);
    if (error)
        return nullptr;
    enum { LEFT, CENTER, TOP_LEFT, TOP, BOTTOM_LEFT, BOTTOM, DV };
    switch (location) {
    case LEFT: case BOTTOM_LEFT: case DV: return is440 ? "mpeg1" : "mpeg2";
    case CENTER: case BOTTOM: return "mpeg1";
    case TOP: return is440 ? "top_left" : "mpeg1";
    case TOP_LEFT: return "top_left";
    default: return nullptr;
    }
}

Parameters GetParameters(const AVSValue &args, const Signature &signature, bool has_avs_expr_support, IScriptEnvironment *env)
{
    Parameters parameters;

    for (int i = 0; i < signature.count(); i++) {
      if (signature[i].getName() == "cplace") {
        const String given = args[i].Defined() ? String(args[i].AsString()) : String("auto");
        const char *from_props = (given == "" || given == "auto") ? CplaceFromFrameProps(args[0], env) : nullptr;
        if (from_props) {
          parameters.push_back(Parameter(Value(String(from_props)), signature[i].getName(), false));
          continue;
        }
      }
      if (signature[i].getName() == "use_expr" && !has_avs_expr_support) {
        // v2.26 when no "Expr" support in avisynth then set use_expr to undefined (default 0)
        parameters.push_back(GetParameter(AVSValue(), signature[i]));
      }
      else
        parameters.push_back(GetParameter(args[i], signature[i]));
    }

    return parameters;
}

} }

#endif
