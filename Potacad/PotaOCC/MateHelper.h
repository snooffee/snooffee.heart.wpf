#pragma once
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <V3d_View.hxx>
namespace PotaOCC
{
    struct NativeViewerHandle;
    namespace MateHelper
    {
        template <typename T>T ClampValue(T v, T lo, T hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }
        void HandleFaceAlignment(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, Handle(AIS_Shape) aisShape1, const TopoDS_Face& face1, Handle(AIS_Shape) aisShape2, const TopoDS_Face& face2);
    }
}