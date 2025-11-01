#pragma once

#include "pch.h"
#include "NativeViewerHandle.h"

#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <Quantity_Color.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GC_MakeSegment.hxx>
#include <TopoDS_Edge.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <Standard_Integer.hxx>
#include <AIS_TextLabel.hxx>
#include <TCollection_ExtendedString.hxx>

using namespace System;

namespace PotaOCC {

    public ref class DimensionDrawer
    {
    public:
        /// <summary>
        /// Draws a batch of dimension entities (linear, radial, diameter, angular)
        /// </summary>
        static array<int>^ DrawDimensionBatch(
            System::IntPtr ctxPtr,
            array<double>^ startX, array<double>^ startY, array<double>^ startZ,
            array<double>^ endX, array<double>^ endY, array<double>^ endZ,
            array<double>^ leaderX, array<double>^ leaderY, array<double>^ leaderZ,
            array<System::String^>^ textValues,
            array<System::String^>^ dimTypes,
            array<System::String^>^ lineTypes,
            array<int>^ r, array<int>^ g, array<int>^ b);
    };
}