#pragma once
#include <AIS_InteractiveObject.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include "NativeViewerHandle.h"
#include <SelectMgr_EntityOwner.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <vector>
#include <iostream>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <gcroot.h>
using namespace System;
namespace PotaOCC {
    struct NativeViewerHandle;
}
class ShapeExtruder;
class ShapeBooleanOperator;
class MateHelper;

// 🧠 Helper class for reusable mouse-related operations
class MouseHelper
{
public:
    static Handle(AIS_Shape) DrawLine(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static Handle(AIS_Shape) DrawCircle(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static Handle(AIS_Shape) DrawEllipse(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static Handle(AIS_Shape) DrawRectangle(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static void ResetDragState(PotaOCC::NativeViewerHandle* native);
    static void ClearRubberBand(PotaOCC::NativeViewerHandle* native);
    static void HandleMoveModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
    static void HandleRotateModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
    static void HandleDefaultMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect, std::vector<Handle(AIS_InteractiveObject)>& lastHilightedObjects);
    static void HandleMateModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
    static void HandleLineMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y);
    static void HandleCircleMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static void HandleEllipseMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static void HandleRectangleMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
    static void HandleTrimMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context);
    static void HandleRadiusMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context);
    static void HandleExtrudingMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
    static void HandleBooleanMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
    static void HandleObjectDetection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
    static void DrawLineOverlay(PotaOCC::NativeViewerHandle* native, int height);
    static void DrawCircleOverlay(PotaOCC::NativeViewerHandle* native, int height);
    static void DrawRectangleOverlay(PotaOCC::NativeViewerHandle* native, int height);
    static void DrawEllipseOverlay(PotaOCC::NativeViewerHandle* native, int height);
    static void DrawSelectionRectangle(PotaOCC::NativeViewerHandle* native);
    static void HighlightHoveredShape(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
    static void HandleMove(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
    static void HandleRotate(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
    static void HandleExtrude(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int y);
    static void UpdateHoverDetection(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
    static void DrawDimensionOverlay(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& cursor, bool isFinal = false);
    static gp_Pnt Get3DPntFromScreen(Handle(V3d_View) view, int x, int y);
};