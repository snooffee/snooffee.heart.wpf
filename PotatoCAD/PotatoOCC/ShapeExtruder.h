#pragma once
#include <TopoDS_Wire.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <iostream>
#include <gp_Vec.hxx>

// Forward declare the handle inside PotatoOCC namespace to match its real definition
namespace PotatoOCC {
    struct NativeViewerHandle;
}

class ShapeExtruder
{
public:
    explicit ShapeExtruder(PotatoOCC::NativeViewerHandle* nativeHandle);

    bool ExtrudeWireAndDisplayPreview(const Handle(AIS_InteractiveContext)& context);
    bool ExtrudeWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, int finalHeight);

    /// <summary>
    /// Extrudes a wire or edge to a solid and displays it in the 3D viewer.
    /// </summary>
    /// <param name="context">AIS interactive context</param>
    /// <param name="wire">Wire or wire-built shape to extrude</param>
    /// <param name="height">Extrusion height (default 300.0)</param>
    /// <returns>true if successful</returns>
    static bool ExtrudeWireAndDisplay(const Handle(AIS_InteractiveContext)& context,
        const TopoDS_Wire& wire,
        Standard_Real height = 50.0)
    {
        if (wire.IsNull()) {
            std::cout << "? Wire is null. Cannot extrude." << std::endl;
            return false;
        }

        // Try to make a face from the wire
        BRepBuilderAPI_MakeFace makeFace(wire);
        TopoDS_Shape solid;

        if (makeFace.IsDone()) {
            TopoDS_Face face = makeFace.Face();
            solid = BRepPrimAPI_MakePrism(face, gp_Vec(0, 0, height));
        }
        else {
            std::cout << "?? Failed to create face from wire. Extruding the wire directly." << std::endl;
            solid = BRepPrimAPI_MakePrism(wire, gp_Vec(0, 0, height));
        }

        if (solid.IsNull()) {
            std::cout << "? Extrusion failed!" << std::endl;
            return false;
        }

        // Display in shaded mode
        Handle(AIS_Shape) aisSolid = new AIS_Shape(solid);
        aisSolid->SetDisplayMode(AIS_Shaded);
        context->Display(aisSolid, Standard_True);

        std::cout << "?? Extrusion successful." << std::endl;
        return true;
    }


private:
    PotatoOCC::NativeViewerHandle* native;
};