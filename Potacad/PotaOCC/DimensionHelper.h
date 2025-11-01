#pragma once
#include <AIS_InteractiveContext.hxx>
#include <gp_Pnt.hxx>
using namespace System;
namespace PotaOCC {
    struct NativeViewerHandle;
}
class DimensionHelper {
public:
    static void ClearPreviousOverlays(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, bool isFinal);
    static void DrawExtensionLine(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& start, const gp_Pnt& end, bool isFinal);
    static void DrawDimensionLine(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& p1, const gp_Pnt& p2, bool isFinal);
    static void DrawArrowheads(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Vec& edgeVec, bool isFinal);
    static void DrawLabel(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, const gp_Pnt& dimLineP1, const gp_Pnt& dimLineP2, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Vec& edgeVec, bool isFinal);
};