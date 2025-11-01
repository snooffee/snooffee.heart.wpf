#include "pch.h"
#include "ShapeRevolver.h"
#include "NativeViewerHandle.h"
#include <BRepBuilderAPI_MakeFace.hxx>

using namespace PotaOCC;

ShapeRevolver::ShapeRevolver(NativeViewerHandle* nativeHandle) : native(nativeHandle) {}
bool ShapeRevolver::RevolveWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, double angleDeg)
{
    if (native->activeWire.IsNull() || !native->isRevolveAxisSet)
    {
        std::cout << "⚠️ No wire or axis selected for revolve." << std::endl;
        return false;
    }
    double angleRad = angleDeg * M_PI / 180.0;
    BRepBuilderAPI_MakeFace makeFace(native->activeWire);
    TopoDS_Shape solid;
    if (makeFace.IsDone())
        solid = BRepPrimAPI_MakeRevol(makeFace.Face(), native->revolveAxis, angleRad);
    else
        solid = BRepPrimAPI_MakeRevol(native->activeWire, native->revolveAxis, angleRad);
    Handle(AIS_Shape) aisSolid = new AIS_Shape(solid);
    aisSolid->SetColor(Quantity_NOC_ORANGE);
    aisSolid->SetDisplayMode(AIS_Shaded);
    context->Display(aisSolid, Standard_True);
    //std::cout << "✅ Revolve finalized. Angle: " << angleDeg << "°" << std::endl;
    native->revolvePreviewShape.Nullify();
    native->activeWire.Nullify();
    native->isRevolveAxisSet = false;

    return true;
}
bool ShapeRevolver::RevolveWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, const TopoDS_Wire& wire, double angle)
{
    if (wire.IsNull())
    {
        std::cout << "⚠️ No wire provided for revolve." << std::endl;
        return false;
    }
    if (native->revolveAxisObj.IsNull())
    {
        std::cout << "⚠️ No revolve axis selected.\n";
        return false;
    }
    BRepBuilderAPI_MakeFace mkFace(wire);
    if (!mkFace.IsDone())
    {
        std::cout << "⚠️ Unable to create face from wire for revolve." << std::endl;
        return false;
    }
    TopoDS_Face face = mkFace.Face();
    BRepPrimAPI_MakeRevol makeRevol(face, native->revolveAxis, angle * (M_PI / 180.0));
    if (!makeRevol.IsDone())
    {
        std::cout << "⚠️ Revolve operation failed." << std::endl;
        return false;
    }
    TopoDS_Shape revolved = makeRevol.Shape();
    Handle(AIS_Shape) aisRevolved = new AIS_Shape(revolved);
    aisRevolved->SetColor(Quantity_NOC_YELLOW);
    aisRevolved->SetDisplayMode(AIS_Shaded);
    context->Display(aisRevolved, Standard_True);
    //std::cout << "✅ Revolve completed successfully around the first selected line axis." << std::endl;
    return true;
}