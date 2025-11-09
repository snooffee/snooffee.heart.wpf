#pragma once
#include "NativeViewerHandle.h"
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <vector>

using namespace System;

namespace PotaOCC
{
    public ref class HatchDrawer
    {
    public:
        static array<int>^ DrawHatchBatch(
            IntPtr viewerHandlePtr,
            array<array<array<double>^>^>^ allBoundariesX, // outer + inner boundaries
            array<array<array<double>^>^>^ allBoundariesY,
            array<array<array<double>^>^>^ allBoundariesZ,
            array<int>^ r, array<int>^ g, array<int>^ b,
            array<double>^ transparency,
            array<String^>^ pattern,
            array<bool>^ solid);
    };
}