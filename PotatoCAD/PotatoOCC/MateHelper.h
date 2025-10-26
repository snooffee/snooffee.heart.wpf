#pragma once

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Geom_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <V3d_View.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>
#include <Standard_Type.hxx>
#include <iostream>

class MateHelper
{
public:
    // ? Utility template
    template <typename T>
    static T ClampValue(T v, T lo, T hi)
    {
        return (v < lo) ? lo : (v > hi) ? hi : v;
    }

    // ? Compute normal of a face
    static gp_Vec ComputeNormal(const TopoDS_Face& face);

    // ? Compute transformation to align face1 Å® face2
    static gp_Trsf ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1,
        const gp_Pnt& p2, const gp_Vec& n2);

    // ? Apply transformation to shape
    static void ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform);

    // ? Handle face alignment (first overload)
    static void HandleFaceAlignment(Handle(AIS_InteractiveContext) context,
        Handle(V3d_View) view,
        int x, int y,
        Handle(AIS_InteractiveObject) movingObject,
        bool isMoveMode,
        bool isRotateMode);

    // ? Handle face alignment (second overload)
    static void HandleFaceAlignment(Handle(AIS_InteractiveContext) context,
        Handle(V3d_View) view,
        Handle(AIS_Shape) aisShape1,
        const TopoDS_Face& face1,
        Handle(AIS_Shape) aisShape2,
        const TopoDS_Face& face2);
};