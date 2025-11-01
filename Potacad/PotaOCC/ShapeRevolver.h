#pragma once
#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
namespace PotaOCC {
    struct NativeViewerHandle;
}
class ShapeRevolver
{
public:
    explicit ShapeRevolver(PotaOCC::NativeViewerHandle* nativeHandle);
    bool RevolveWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, double angleDeg);
    bool RevolveWireAndDisplayFinal(const Handle(AIS_InteractiveContext)& context, const TopoDS_Wire& wire, double angle);
private:
    PotaOCC::NativeViewerHandle* native;
};