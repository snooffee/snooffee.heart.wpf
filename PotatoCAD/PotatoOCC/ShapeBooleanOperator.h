#pragma once

// --- OpenCascade Includes ---
#include <TopoDS_Shape.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveContext.hxx>
#include <BRepAlgoAPI_Fuse.hxx>       // Union
#include <BRepAlgoAPI_Cut.hxx>        // Subtraction
#include <BRepAlgoAPI_Common.hxx>     // Intersection
#include <iostream>
#include "NativeViewerHandle.h"


// ---------------------------------------------------------------------------
// 🧠 CLASS: ShapeBooleanOperator
// ---------------------------------------------------------------------------
// Encapsulates Boolean operations (Union, Cut, Intersect)
// ---------------------------------------------------------------------------
class ShapeBooleanOperator
{
public:
    // Constructor
    explicit ShapeBooleanOperator(PotatoOCC::NativeViewerHandle* nativeHandle);

    // Perform Union (A + B)
    bool Fuse(const Handle(AIS_InteractiveContext)& context,
        const TopoDS_Shape& shapeA,
        const TopoDS_Shape& shapeB);

    // Perform Subtraction (A - B)
    bool Cut(const Handle(AIS_InteractiveContext)& context,
        const TopoDS_Shape& shapeA,
        const TopoDS_Shape& shapeB);

    // Perform Intersection (A ∩ B)
    bool Common(const Handle(AIS_InteractiveContext)& context,
        const TopoDS_Shape& shapeA,
        const TopoDS_Shape& shapeB);

    static void FinalizeBooleanResult(
        PotatoOCC::NativeViewerHandle* native,
        const Handle(AIS_InteractiveContext)& context,
        const Handle(V3d_View)& view,
        const TopoDS_Shape& resultShape,
        const char* successMsg,
        const char* failMsg,
        bool success);

    static TopoDS_Shape PerformBooleanOperation(
        const TopoDS_Shape& shape1,
        const TopoDS_Shape& shape2,
        PotatoOCC::NativeViewerHandle* native,
        const Handle(AIS_InteractiveContext)& context,
        const Handle(V3d_View)& view,
        bool& success);

private:
    PotatoOCC::NativeViewerHandle* native;
};