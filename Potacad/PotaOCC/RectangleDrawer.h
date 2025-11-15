#pragma once

#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <V3d_View.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Quantity_Color.hxx>
#include <iostream>

namespace PotaOCC {
    struct NativeViewerHandle;
}
class RectangleDrawer
{
public:
    static Handle(AIS_Shape) DrawRectangle(Handle(AIS_InteractiveContext) context, PotaOCC::NativeViewerHandle* native, Handle(V3d_View) view);
};