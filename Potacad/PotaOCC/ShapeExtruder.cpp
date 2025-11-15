#include "pch.h"
#include "ShapeExtruder.h"
#include "NativeViewerHandle.h"
#include <Quantity_Color.hxx>
#include <AIS_Shape.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <iostream>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

using namespace PotaOCC;

ShapeExtruder::ShapeExtruder(NativeViewerHandle* nativeHandle)
    : native(nativeHandle)
{
}

bool ShapeExtruder::ExtrudeWireAndDisplayPreview(const Handle(AIS_InteractiveContext)& context)
{
    // Build planar face from wire
    BRepBuilderAPI_MakeFace makeFace(native->activeWire);
    if (!makeFace.IsDone()) {
        std::cout << "❌ Failed to build face from wire." << std::endl;
        return false;
    }

    TopoDS_Face face = makeFace.Face();

    // Extract real plane from the face
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    Handle(Geom_Plane) geomPlane = Handle(Geom_Plane)::DownCast(surf);
    if (geomPlane.IsNull()) {
        std::cout << "❌ The selected wire is not planar!" << std::endl;
        return false;
    }

    // Correct world-space normal
    gp_Pln plane = geomPlane->Pln();
    gp_Vec extrudeDir(plane.Axis().Direction());
    extrudeDir *= native->currentExtrudeHeight; // Apply height (positive or negative)

    // Extrude
    TopoDS_Shape previewSolid = BRepPrimAPI_MakePrism(face, extrudeDir);
    if (previewSolid.IsNull()) {
        std::cout << "❌ Extrusion failed." << std::endl;
        return false;
    }

    // Display preview shape
    if (!native->extrudePreviewShape.IsNull())
        context->Remove(native->extrudePreviewShape, Standard_False);

    native->extrudePreviewShape = new AIS_Shape(previewSolid);
    native->extrudePreviewShape->SetColor(Quantity_NOC_PINK);
    native->extrudePreviewShape->SetTransparency(0.0);
    native->extrudePreviewShape->SetDisplayMode(AIS_Shaded);
    context->Display(native->extrudePreviewShape, Standard_False);

    return true;
}


bool ShapeExtruder::ExtrudeWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, int finalHeight)
{
    // Build planar face from wire
    BRepBuilderAPI_MakeFace makeFace(native->activeWire);
    if (!makeFace.IsDone()) {
        std::cout << "❌ Failed to build face from wire." << std::endl;
        return false;
    }

    TopoDS_Face face = makeFace.Face();

    // Extract real plane from the face
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    Handle(Geom_Plane) geomPlane = Handle(Geom_Plane)::DownCast(surf);
    if (geomPlane.IsNull()) {
        std::cout << "❌ The selected wire is not planar!" << std::endl;
        return false;
    }

    // Correct world-space normal
    gp_Pln plane = geomPlane->Pln();
    gp_Vec extrudeDir(plane.Axis().Direction());
    extrudeDir *= native->currentExtrudeHeight; // Apply height (positive or negative)

    // Extrude
    TopoDS_Shape solid = BRepPrimAPI_MakePrism(face, extrudeDir);
    if (solid.IsNull()) {
        std::cout << "❌ Extrusion failed." << std::endl;
        return false;
    }

    Handle(AIS_Shape) aisSolid = new AIS_Shape(solid);
    aisSolid->SetColor(Quantity_NOC_ORANGE);
    aisSolid->SetDisplayMode(AIS_Shaded);
    context->Display(aisSolid, Standard_True);

    //std::cout << "✅ Extrusion finalized. Height: " << finalHeight << std::endl;

    native->extrudePreviewShape.Nullify();
    native->activeWire.Nullify();

    return true;
}