#ifndef NOMINMAX
#define NOMINMAX // no min & max macros
#endif
#ifdef _WIN32
#include <windows.h>
#pragma warning( push )
#pragma warning(disable:4100) // disable possible loss of data conversion
#include <avisynth.h>
#pragma warning( pop )
#else
#include <avisynth.h>
#endif
#include <avs/config.h>
#include "../../../common/utils/utils.h"
#include "../../helpers/forms/forms.h"
#include "../../helpers/parser/spirit.h"

using namespace Filtering;
using namespace Filtering::MaskTools::Helpers::Forms;
using namespace Filtering::MaskTools::Helpers::PolishConverter;

namespace Filtering { namespace MaskTools { namespace Avs2x { namespace Helpers {

template<Form rf>
AVSValue __cdecl CreateRadiusForm(AVSValue args, void *user_data, IScriptEnvironment *env)
{
   UNUSED(user_data); UNUSED(env);
   return AVSValue((new String(rf(-args[0].AsInt(1), -args[0].AsInt(1), args[0].AsInt(1), args[0].AsInt(1), args[1].AsBool(true))))->c_str()); /* grrrrr -> memory leak */
}

template<StringConverter sc>
AVSValue __cdecl CreateStringConverter(AVSValue args, void *user_data, IScriptEnvironment *env)
{
   UNUSED(user_data); UNUSED(env);
   return AVSValue((new String(sc(args[0].AsString("x"))))->c_str()); /* grrrrr -> memory leak */
}

template<Form bf>
AVSValue __cdecl CreateBiRadiusForm(AVSValue args, void *user_data, IScriptEnvironment *env)
{
   UNUSED(user_data); UNUSED(env);
   return AVSValue((new String(bf(-args[0].AsInt(1), -args[1].AsInt(1), args[0].AsInt(1), args[1].AsInt(1), args[2].AsBool(true))))->c_str()); /* grrrrr -> memory leak */
}

template<Form bf>
AVSValue __cdecl CreateGenericForm(AVSValue args, void *user_data, IScriptEnvironment *env)
{
   UNUSED(user_data); UNUSED(env);
   return AVSValue((new String(bf(args[0].AsInt(-1), args[1].AsInt(-1), args[2].AsInt(1), args[3].AsInt(1), args[4].AsBool(true))))->c_str()); /* grrrrr -> memory leak */
}

template<Form rf>
static void DeclareRadiusForm(const String &name, IScriptEnvironment *env)
{
   env->AddFunction(name.c_str(), "[radius]i[zero]b", CreateRadiusForm<rf>, NULL);
}

template<Form rf>
static void DeclareGenericForm(const String &name, IScriptEnvironment *env)
{
   env->AddFunction(name.c_str(), "iiii[zero]b", CreateGenericForm<rf>, NULL);
}

template<StringConverter sc>
static void DeclareStringConverter(const String &name, IScriptEnvironment *env)
{
   env->AddFunction(name.c_str(), "[infix]s", CreateStringConverter<sc>, NULL);
}

template<Form bf>
static void DeclareBiRadiusForm(const String &name, IScriptEnvironment *env)
{
   env->AddFunction(name.c_str(), "[hor_radius]i[ver_radius]i[zero]b", CreateBiRadiusForm<bf>, NULL);
}

void DeclareHelpers(IScriptEnvironment* env)
{
   DeclareBiRadiusForm<LosangeToString>("mt_losange", env);
   DeclareBiRadiusForm<RectangleToString>("mt_rectangle", env);
   DeclareBiRadiusForm<EllipseToString>("mt_ellipse", env);
   DeclareRadiusForm<RectangleToString>("mt_square", env);
   DeclareRadiusForm<EllipseToString>("mt_circle", env);
   DeclareRadiusForm<LosangeToString>("mt_diamond", env);
   DeclareGenericForm<RectangleToString>("mt_freerectangle", env);
   DeclareGenericForm<EllipseToString>("mt_freeellipse", env);
   DeclareGenericForm<LosangeToString>("mt_freelosange", env);
   DeclareStringConverter<Converter>("mt_polish", env);
   DeclareStringConverter<Infix>("mt_infix", env);
}

} } } }