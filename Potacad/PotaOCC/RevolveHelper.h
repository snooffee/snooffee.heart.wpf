#pragma once

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Ax1.hxx>
#include <iostream> 
#include "NativeViewerHandle.h" 
namespace PotaOCC {
    struct NativeViewerHandle;
}
class RevolveHelper
{
public:
    static bool TrySelectRevolveAxis(PotaOCC::NativeViewerHandle* native, const Handle(AIS_InteractiveContext)& context);
};