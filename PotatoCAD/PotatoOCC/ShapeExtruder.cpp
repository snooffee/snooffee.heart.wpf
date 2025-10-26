#include "pch.h"                    // Required for Visual Studio precompiled headers
#include "ShapeExtruder.h"
#include "NativeViewerHandle.h"

#include <Quantity_Color.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <iostream>

using namespace PotatoOCC;

ShapeExtruder::ShapeExtruder(NativeViewerHandle* nativeHandle)
    : native(nativeHandle)
{
}

bool ShapeExtruder::ExtrudeWireAndDisplayPreview(const Handle(AIS_InteractiveContext)& context)
{
    BRepBuilderAPI_MakeFace makeFace(native->activeWire);
    TopoDS_Shape previewSolid;

    if (makeFace.IsDone())
    {
        TopoDS_Face face = makeFace.Face();
        previewSolid = BRepPrimAPI_MakePrism(face, gp_Vec(0, 0, native->currentExtrudeHeight));
    }
    else
    {
        previewSolid = BRepPrimAPI_MakePrism(native->activeWire, gp_Vec(0, 0, native->currentExtrudeHeight));
    }

    if (previewSolid.IsNull())
    {
        std::cout << "Extrusion failed!" << std::endl;
        return false;
    }

    native->extrudePreviewShape = new AIS_Shape(previewSolid);
    native->extrudePreviewShape->SetColor(Quantity_NOC_PINK);
    native->extrudePreviewShape->SetTransparency(0.0);
    native->extrudePreviewShape->SetDisplayMode(AIS_Shaded);
    context->Display(native->extrudePreviewShape, Standard_False);

    return true;
}

bool ShapeExtruder::ExtrudeWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, int finalHeight)
{
    BRepBuilderAPI_MakeFace makeFace(native->activeWire);
    TopoDS_Shape solid;

    if (makeFace.IsDone())
        solid = BRepPrimAPI_MakePrism(makeFace.Face(), gp_Vec(0, 0, finalHeight));
    else
        solid = BRepPrimAPI_MakePrism(native->activeWire, gp_Vec(0, 0, finalHeight));

    Handle(AIS_Shape) aisSolid = new AIS_Shape(solid);
    aisSolid->SetColor(Quantity_NOC_ORANGE);
    aisSolid->SetDisplayMode(AIS_Shaded);
    context->Display(aisSolid, Standard_True);

    std::cout << "✅ Extrusion finalized. Height: " << finalHeight << std::endl;

    native->extrudePreviewShape.Nullify();
    native->activeWire.Nullify();

    return true;
}