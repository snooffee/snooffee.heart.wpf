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
    namespace MouseHelper
    {
        Handle(AIS_Shape) DrawLine(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        Handle(AIS_Shape) DrawCircle(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        Handle(AIS_Shape) DrawEllipse(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        Handle(AIS_Shape) DrawRectangle(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        void ResetDragState(PotaOCC::NativeViewerHandle* native);
        void ClearRubberBand(PotaOCC::NativeViewerHandle* native);
        void HandleMoveModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
        void HandleRotateModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
        void HandleDefaultMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect, std::vector<Handle(AIS_InteractiveObject)>& lastHilightedObjects);
        void HandleMateModeMouseDown(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
        void HandleLineMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y);
        void HandleCircleMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        void HandleEllipseMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        void HandleRectangleMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w);
        void HandleTrimMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context);
        void HandleRadiusMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context);
        void HandleExtrudingMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleBooleanMode(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
        void HandleObjectDetection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y);
        void DrawLineOverlay(PotaOCC::NativeViewerHandle* native, int height);
        void DrawCircleOverlay(PotaOCC::NativeViewerHandle* native, int height);
        void DrawRectangleOverlay(PotaOCC::NativeViewerHandle* native, int height);
        void DrawEllipseOverlay(PotaOCC::NativeViewerHandle* native, int height);
        void DrawSelectionRectangle(PotaOCC::NativeViewerHandle* native);
        void HighlightHoveredShape(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleMove(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleRotate(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleExtrude(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int y);
        void UpdateHoverDetection(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void DrawDimensionOverlay(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& cursor, bool isFinal = false);
        gp_Pnt Get3DPntFromScreen(Handle(V3d_View) view, int x, int y);
        void HandleZoomWindow(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view);
    }
}