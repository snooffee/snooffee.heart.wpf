#include "pch.h"
#include "ShapeBooleanOperator.h"
#include "ViewHelper.h"

using namespace PotaOCC;

// Constructor
ShapeBooleanOperator::ShapeBooleanOperator(NativeViewerHandle* nativeHandle)
    : native(nativeHandle)
{
}

// ---------------------------------------------------------------------------
// 🧱 Union (A + B)
// ---------------------------------------------------------------------------
bool ShapeBooleanOperator::Fuse(const Handle(AIS_InteractiveContext)& context,
    const TopoDS_Shape& shapeA,
    const TopoDS_Shape& shapeB)
{
    std::cout << "Performing Boolean Fuse (Union)..." << std::endl;

    BRepAlgoAPI_Fuse fuseOp(shapeA, shapeB);
    fuseOp.Build();
    if (!fuseOp.IsDone())
    {
        std::cout << "❌ Boolean Fuse failed!" << std::endl;
        return false;
    }

    TopoDS_Shape result = fuseOp.Shape();

    Handle(AIS_Shape) aisResult = new AIS_Shape(result);
    aisResult->SetColor(Quantity_NOC_GREEN);  // 🎨 Green for Union
    aisResult->SetDisplayMode(AIS_Shaded);

    context->Display(aisResult, Standard_True);
    native->persistedExtrusions.push_back(aisResult);

    std::cout << "✅ Fuse completed successfully.\n";
    return true;
}

// ---------------------------------------------------------------------------
// ✂️ Subtraction (A - B)
// ---------------------------------------------------------------------------
bool ShapeBooleanOperator::Cut(const Handle(AIS_InteractiveContext)& context,
    const TopoDS_Shape& shapeA,
    const TopoDS_Shape& shapeB)
{
    std::cout << "Performing Boolean Cut (Subtraction)..." << std::endl;

    BRepAlgoAPI_Cut cutOp(shapeA, shapeB);
    cutOp.Build();
    if (!cutOp.IsDone())
    {
        std::cout << "❌ Boolean Cut failed!" << std::endl;
        return false;
    }

    TopoDS_Shape result = cutOp.Shape();

    Handle(AIS_Shape) aisResult = new AIS_Shape(result);
    aisResult->SetColor(Quantity_NOC_RED);  // 🎨 Red for Subtraction
    aisResult->SetDisplayMode(AIS_Shaded);

    context->Display(aisResult, Standard_True);
    native->persistedExtrusions.push_back(aisResult);

    std::cout << "✅ Cut completed successfully.\n";
    return true;
}

// ---------------------------------------------------------------------------
// ⚙️ Intersection (A ∩ B)
// ---------------------------------------------------------------------------
bool ShapeBooleanOperator::Common(const Handle(AIS_InteractiveContext)& context,
    const TopoDS_Shape& shapeA,
    const TopoDS_Shape& shapeB)
{
    std::cout << "Performing Boolean Common (Intersection)..." << std::endl;

    BRepAlgoAPI_Common commonOp(shapeA, shapeB);
    commonOp.Build();
    if (!commonOp.IsDone())
    {
        std::cout << "❌ Boolean Common failed!" << std::endl;
        return false;
    }

    TopoDS_Shape result = commonOp.Shape();

    Handle(AIS_Shape) aisResult = new AIS_Shape(result);
    aisResult->SetColor(Quantity_NOC_YELLOW);  // 🎨 Yellow for intersection
    aisResult->SetDisplayMode(AIS_Shaded);

    context->Display(aisResult, Standard_True);
    native->persistedExtrusions.push_back(aisResult);

    std::cout << "✅ Common completed successfully.\n";
    return true;
}

void ShapeBooleanOperator::FinalizeBooleanResult(
    NativeViewerHandle* native,
    const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view,
    const TopoDS_Shape& resultShape,
    const char* successMsg,
    const char* failMsg,
    bool success)
{
    if (success) {
        Handle(AIS_Shape) aisResult = new AIS_Shape(resultShape);

        // Remove original selected shapes from context
        for (auto& selectedObj : native->selectedShapes) {
            context->Remove(selectedObj, Standard_False);
        }
        native->selectedShapes.clear();

        // Remove overlays/helper objects
        if (!native->lineOverlay.IsNull()) {
            context->Remove(native->lineOverlay, Standard_False);
            native->lineOverlay.Nullify();
        }
        if (!native->circleOverlay.IsNull()) {
            context->Remove(native->circleOverlay, Standard_False);
            native->circleOverlay.Nullify();
        }

        ViewHelper::ClearCenterMarker(native);

        aisResult->SetColor(Quantity_NOC_ORANGE);
        aisResult->SetDisplayMode(AIS_Shaded);
        context->Display(aisResult, Standard_True);

        std::cout << successMsg << std::endl;
    }
    else {
        std::cout << failMsg << std::endl;
    }
}

TopoDS_Shape ShapeBooleanOperator::PerformBooleanOperation(
    const TopoDS_Shape& shape1,
    const TopoDS_Shape& shape2,
    NativeViewerHandle* native,
    const Handle(AIS_InteractiveContext)& context,
    const Handle(V3d_View)& view,
    bool& success)
{
    success = false;
    TopoDS_Shape result;

    if (native->isBooleanCutMode) {
        BRepAlgoAPI_Cut booleanCut(shape1, shape2);
        booleanCut.Build();
        success = booleanCut.IsDone();
        if (success)
            result = booleanCut.Shape();
    }
    else if (native->isBooleanUnionMode) {
        BRepAlgoAPI_Fuse booleanFuse(shape1, shape2);
        booleanFuse.Build();
        success = booleanFuse.IsDone();
        if (success)
            result = booleanFuse.Shape();
    }
    else if (native->isBooleanIntersectMode) {
        BRepAlgoAPI_Common booleanCommon(shape1, shape2);
        booleanCommon.Build();
        success = booleanCommon.IsDone();
        if (success)
            result = booleanCommon.Shape();
    }

    return result;
}