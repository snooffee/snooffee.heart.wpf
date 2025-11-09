#pragma once

#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Trsf.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <AIS_ListOfInteractive.hxx>
#include <TopoDS_Wire.hxx>
#include <AIS_Shape.hxx>

namespace PotaOCC
{
    struct NativeViewerHandle;
    namespace GeometryHelper
    {
        gp_Vec ComputeNormal(const TopoDS_Face& face);
        gp_Trsf ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1, const gp_Pnt& p2, const gp_Vec& n2);
        void ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform);
        void ApplyLocalTransformationToShape(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context);
        void ApplyLocalTransformationToAISShape(Handle(AIS_Shape) aisShape, Handle(AIS_InteractiveContext) context);
        void AnimateZoomWindow(PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view, int xMin, int yMin, int xMax, int yMax, int steps);
        bool PerformRectangleSelection(PotaOCC::NativeViewerHandle* native, const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int h, int w, int mouseY);
        Handle(AIS_Shape) CreateHighlightedFace(const TopoDS_Face& face);
        TopoDS_Shape GetSafeDetectedShape(const Handle(AIS_InteractiveContext)& context);
        void ClearHoverHighlightIfAny(PotaOCC::NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void ShowHoverHighlightForFace(PotaOCC::NativeViewerHandle* native, const TopoDS_Face& face, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleFaceHover(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleEdgeHover(PotaOCC::NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view);
        void HandleKeyMode(PotaOCC::NativeViewerHandle* native, char key);
    }
}