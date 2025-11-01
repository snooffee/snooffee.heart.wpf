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

namespace PotaOCC {
    struct NativeViewerHandle;
    class GeometryHelper
    {
    public:
        // ✅ Compute normal vector of a face in world coordinates
        static gp_Vec ComputeNormal(const TopoDS_Face& face);

        // ✅ Compute transformation to align two faces (p1+n1 -> p2+n2)
        static gp_Trsf ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1,
            const gp_Pnt& p2, const gp_Vec& n2);

        // ✅ Apply a transformation to a shape
        static void ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform);
        static bool PerformRectangleSelection(
            NativeViewerHandle* native,
            const Handle(AIS_InteractiveContext)& context,
            const Handle(V3d_View)& view,
            int h,
            int w,
            int mouseY);
    };
}