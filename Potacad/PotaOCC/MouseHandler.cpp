#include "pch.h"
#include "NativeViewerHandle.h"
#include "MouseHandler.h"
#include "MouseHelper.h"
#include "MouseCursor.h"
#include "ViewHelper.h"
#include "ShapeDrawer.h"
#include "ShapeExtruder.h"
#include "ShapeBooleanOperator.h"
#include "MateHelper.h"
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#include <gp_Dir.hxx>
#include <Quantity_Color.hxx>
#include <vector>
#include <TCollection_ExtendedString.hxx>
#include <TopoDS_Shape.hxx>
#include <string>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <iostream>
#include <Prs3d_TypeOfHighlight.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopoDS_Vertex.hxx>
#include <cmath> 
#include "LineDrawer.h"
#include "CircleDrawer.h"
#include "Print.h"
#include <BRepAlgoAPI_Section.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopExp.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <NCollection_TListIterator.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <AIS_InteractiveObject.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include "ShapeRevolver.h"
#include "GeometryHelper.h"
#include "RevolveHelper.h"
#using <System.Windows.Forms.dll>
using namespace System;
using namespace System::Windows::Forms;
using namespace PotaOCC;
NativeViewerHandle* native;
std::vector<Handle(AIS_InteractiveObject)> lastHilightedObjects;
std::vector<TopoDS_Edge> SortEdgesByConnectivityRobust(const std::vector<TopoDS_Edge>& inputEdges, double tolerance)
{
    if (inputEdges.empty()) return {};

    for (size_t startIndex = 0; startIndex < inputEdges.size(); ++startIndex)
    {
        std::vector<TopoDS_Edge> remaining = inputEdges;
        std::vector<TopoDS_Edge> sorted;

        // Take starting edge
        TopoDS_Edge startEdge = remaining[startIndex];
        remaining.erase(remaining.begin() + startIndex);
        sorted.push_back(startEdge);

        for (size_t step = 0; step < inputEdges.size(); ++step)
        {
            gp_Pnt endP = BRep_Tool::Pnt(TopExp::LastVertex(sorted.back()));
            bool found = false;

            for (auto it = remaining.begin(); it != remaining.end(); ++it)
            {
                gp_Pnt startP = BRep_Tool::Pnt(TopExp::FirstVertex(*it));
                gp_Pnt endP2 = BRep_Tool::Pnt(TopExp::LastVertex(*it));

                if (endP.Distance(startP) < tolerance) {
                    sorted.push_back(*it);
                    remaining.erase(it);
                    found = true;
                    break;
                }
                else if (endP.Distance(endP2) < tolerance) {
                    TopoDS_Edge reversed = TopoDS::Edge((*it).Reversed());
                    sorted.push_back(reversed);
                    remaining.erase(it);
                    found = true;
                    break;
                }
            }

            if (!found) {
                break;
            }
        }

        if (remaining.empty()) {
            // Check closure
            gp_Pnt firstStart = BRep_Tool::Pnt(TopExp::FirstVertex(sorted.front()));
            gp_Pnt lastEnd = BRep_Tool::Pnt(TopExp::LastVertex(sorted.back()));
            if (firstStart.Distance(lastEnd) < tolerance) {
                std::cout << "✅ Connectivity sorted successfully starting at edge " << startIndex << std::endl;
                return sorted;
            }
        }
    }

    std::cout << "❌ Could not find a valid ordering of edges." << std::endl;
    return {};
}
struct Rectangle {
    int x, y, width, height;

    // Constructor
    Rectangle(int x, int y, int width, int height)
        : x(x), y(y), width(width), height(height) {
    }

    // Function to check if a point (x, y) is inside the rectangle
    bool Contains(int px, int py) const {
        return (px >= x && px <= (x + width) && py >= y && py <= (y + height));
    }
};
MouseHandler::MouseMode MouseHandler::GetCurrentMode()
{
    if (native->isMateAlignmentMode) return MouseMode::MateAlignment;
    if (native->isMoveMode) return MouseMode::Move;
    if (native->isRotateMode) return MouseMode::Rotate;
    return MouseMode::Default;
}
TopoDS_Face createForcedFace(const TopoDS_Wire& wire) {
    // Fallback method to create a face when wire is incomplete or invalid
    // You can force the face creation by making some assumptions about the wire, 
    // such as converting it into a surface or patch.

    // For simplicity, let's use a default method to create a face from a wire
    TopoDS_Face face;

    // Check if it's a valid wire, otherwise, assume you can create a simple face manually
    if (wire.IsNull()) {
        return face; // Return empty face if wire is not valid
    }

    // Otherwise, attempt to create a face from the wire (even if open)
    try {
        BRepBuilderAPI_MakeFace makeFace(wire);
        if (makeFace.IsDone()) {
            face = makeFace.Face();
        }
    }
    catch (...) {
        // Handle any exceptions or issues during face creation
        std::cout << "Exception occurred while forcing face creation." << std::endl;
    }

    return face;
}
void MouseHandler::StartRotation(IntPtr viewPtr, int x, int y) {
    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    if (view.IsNull()) return;
    view->StartRotation(x, y);
}
void MouseHandler::Rotate(IntPtr viewPtr, int x, int y) {
    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    if (view.IsNull()) return;
    view->Rotation(x, y);
    view->Redraw();
}
void MouseHandler::Pan(IntPtr viewPtr, int dx, int dy) { Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer()); view->Pan(dx, dy); view->Redraw(); }
void MouseHandler::Zoom(IntPtr viewPtr, double factor)
{
    if (!native) return;

    ViewHelper::ClearCreateEntity(native);

    Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer());
    view->SetZoom(factor);
    view->Redraw();
}
void MouseHandler::ZoomAt(IntPtr viewPtr, int x, int y, double factor) { Handle(V3d_View) view = static_cast<V3d_View*>(viewPtr.ToPointer()); view->Place(x, y, factor); view->Redraw(); }
void MouseHandler::SetMouseControlSettings(IntPtr viewerHandlePtr, MouseControlSettings^ settings)
{
    if (viewerHandlePtr == IntPtr::Zero || settings == nullptr) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    native->leftButtonAction = static_cast<int>(settings->LeftButtonAction);
    native->middleButtonAction = static_cast<int>(settings->MiddleButtonAction);
    native->rightButtonAction = static_cast<int>(settings->RightButtonAction);
    native->wheelScrollAction = static_cast<int>(settings->WheelScrollAction);
}
bool MouseHandler::CheckRayIntersectionWithAxis(const gp_Pnt& origin, const gp_Dir& direction, const std::string& axis)
{
    // Depending on the axis, we check if the ray intersects that axis.
    // This is a simplified version: you could improve this by using actual ray-geometry intersection tests.

    // Example logic: check if the ray direction aligns with the axis.
    if (axis == "X")
    {
        // Check for intersection with X axis (red axis in TriedronDisplay)
        // You would check the direction of the ray to see if it intersects the X axis region
        return true;  // Dummy check: assuming the ray always intersects X for simplicity
    }
    else if (axis == "Y")
    {
        // Check for intersection with Y axis (green axis in TriedronDisplay)
        return true;  // Dummy check: assuming the ray always intersects Y for simplicity
    }
    else if (axis == "Z")
    {
        // Check for intersection with Z axis (blue axis in TriedronDisplay)
        return true;  // Dummy check: assuming the ray always intersects Z for simplicity
    }

    return false;
}
void MouseHandler::ResetViewToAxis(Handle(V3d_View) view, const std::string& axis)
{
    //if (axis == "X")
    //{
    //    // Reset the view to show the X axis (looking down the X axis)
    //    view->SetProj(V3d_Xpos);  // Change this to whatever projection corresponds to the X axis
    //}
    //else if (axis == "Y")
    //{
    //    // Reset the view to show the Y axis (looking down the Y axis)
    //    view->SetProj(V3d_Ypos);  // Change this to whatever projection corresponds to the Y axis
    //}
    //else if (axis == "Z")
    //{
    //    // Reset the view to show the Z axis (looking down the Z axis)
    //    view->SetProj(V3d_Zpos);  // Change this to whatever projection corresponds to the Z axis
    //}

    view->SetProj(V3d_Zpos);

    // Redraw the view
    view->Redraw();
}
void MouseHandler::ResetView(IntPtr viewerHandlePtr, int x, int y)
{
    NativeViewerHandle* native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    Handle(V3d_View) view = native->view;

    // Redraw the view after the interaction
    native->view->SetProj(V3d_Zpos);
    native->context->UpdateCurrentViewer();
    native->view->FitAll();
    native->view->Redraw();
}
void MouseHandler::OnMouseDown(IntPtr viewerHandlePtr, int x, int y, bool multipleselect)
{
    native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || !native->view) {
        std::cout << "Invalid NativeViewerHandle or view!" << std::endl;
        return;
    }

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    if (context.IsNull() || view.IsNull()) {
        std::cout << "AIS_InteractiveContext or V3d_View is null!" << std::endl;
        return;
    }

    if (native->isRevolveMode)
    {
        if (!native->isRevolveAxisSet)
        {
            if (RevolveHelper::TrySelectRevolveAxis(native, context))
                return;
            else
                return; // Failed to set axis — abort action
        }
    }
    if (native->isEllipseMode)
    {
        // Record the start point of the ellipse
        native->dragStartX = x;
        native->dragStartY = y;
        native->isDragging = true;  // Enable dragging mode
        return;
    }
    if (native->isDrawDimensionMode)
    {
        gp_Pnt clickedPnt = MouseHelper::Get3DPntFromScreen(view, x, y);

        if (native->isPickingDimensionFirstPoint)
        {
            native->dimPoint1 = clickedPnt;
            native->isPickingDimensionFirstPoint = false;
        }
        else if (!native->isPlacingDimension)
        {
            native->dimPoint2 = clickedPnt;
            native->isPlacingDimension = true;
        }
        else
        {
            native->dimPosition = clickedPnt;

            // Final placement
            MouseHelper::DrawDimensionOverlay(native, view, native->dimPoint1, native->dimPoint2, native->dimPosition, true);

            // Clean up temporary overlay
            if (!native->dimLineTemp.IsNull()) context->Remove(native->dimLineTemp, Standard_False);
            if (!native->dimLabelTemp.IsNull()) context->Remove(native->dimLabelTemp, Standard_False);
            native->dimLineTemp.Nullify();
            native->dimLabelTemp.Nullify();

            native->isDrawDimensionMode = false;
            native->isPickingDimensionFirstPoint = true;
            native->isPlacingDimension = false;
        }

        view->Redraw();
        return;
    }
    HandleMouseDownAction(context, view, x, y, multipleselect);
}
void MouseHandler::OnMouseMove(IntPtr viewerHandlePtr, int x, int y, int h)
{
    native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->view.IsNull() || native->context.IsNull())
        return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    if (native->isDragging)
    {
        native->dragEndX = x;
        native->dragEndY = y;

        if (native->isLineMode)
        {
            MouseHelper::DrawLineOverlay(native, h);
        }
        else if (native->isCircleMode)
        {
            MouseHelper::DrawCircleOverlay(native, h);
        }
        else if (native->isRectangleMode)
        {
            MouseHelper::DrawRectangleOverlay(native, h);
        }
        else if (native->isTrimMode)
        {
            // TODO: Implement trim mode if needed
        }
        else if (native->isBooleanUnionMode || native->isBooleanCutMode || native->isBooleanIntersectMode)
        {
            MouseHelper::HighlightHoveredShape(native, x, y, context, view);
        }
        else if (native->isEllipseMode)  // If the user is in ellipse mode
        {
            MouseHelper::DrawEllipseOverlay(native, h);
        }
        else
        {
            MouseHelper::DrawSelectionRectangle(native);
            view->Redraw();
        }
    }
    else if (native->isDrawDimensionMode && native->isPlacingDimension)
    {
        gp_Pnt cursorPnt = MouseHelper::Get3DPntFromScreen(view, x, y);

        MouseHelper::DrawDimensionOverlay(
            native,
            view,
            native->dimPoint1,
            native->dimPoint2,
            cursorPnt, // cursor determines offset of temporary line & label
            false
        );

    }
    else if (native->isMoveMode)
    {
        MouseHelper::HandleMove(native, x, y, context, view);
    }
    else if (native->isRotateMode && native->isRotating)
    {
        MouseHelper::HandleRotate(native, x, y, context, view);
    }
    else if (native->isExtrudingActive)
    {
        MouseHelper::HandleExtrude(native, context, view, y);
    }
    else
    {
        MouseHelper::UpdateHoverDetection(native, x, y, context, view);
    }
}
void MouseHandler::OnMouseUp(IntPtr viewerHandlePtr, int x, int y, int h, int w)
{
    native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    native->dragEndX = x;
    native->dragEndY = y;

    if (native->isLineMode) {
        MouseHelper::HandleLineMode(native, context, view, viewerHandlePtr, h, w, x, y);
        view->Redraw();
        return;
    }
    else if (native->isCircleMode) {
        MouseHelper::HandleCircleMode(native, context, view, viewerHandlePtr, h, w);
        view->Redraw();
        return;
    }
    else if (native->isEllipseMode) {
        MouseHelper::HandleEllipseMode(native, context, view, viewerHandlePtr, h, w);
        view->Redraw();
        return;
    }
    else if (native->isRectangleMode) {
        MouseHelper::HandleRectangleMode(native, context, view, viewerHandlePtr, h, w);
        view->Redraw();
        return;
    }
    else if (native->isTrimMode) {
        MouseHelper::HandleTrimMode(native, context);
        view->Redraw();
        return;
    }
    else if (native->isRadiusMode) {
        MouseHelper::HandleRadiusMode(native, context);
    }
    else if (native->isExtrudingActive) {
        MouseHelper::HandleExtrudingMode(native, context, view);
    }
    else if (native->isBooleanUnionMode || native->isBooleanCutMode || native->isBooleanIntersectMode) {
        MouseHelper::HandleBooleanMode(native, context, view, x, y);
        view->Redraw();
        return;
    }
    else if (native->isMoveMode && native->isMoving) {
        native->isMoving = false;
        native->movingObject.Nullify();
    }
    else if (native->isRotateMode && native->isRotating) {
        native->isRotating = false;
        native->rotatingObject.Nullify();
    }
    else if (native->isDragging && native->dragEndX != 0) {
        Boolean handle = GeometryHelper::PerformRectangleSelection(native, context, view, h, w, y);
    }
    else {
        MouseHelper::HandleObjectDetection(context, view, x, y);
    }

    MouseHelper::ResetDragState(native);
    MouseHelper::ClearRubberBand(native);

    view->Redraw();
}
void MouseHandler::OnKeyDown(IntPtr viewerHandlePtr, char key)
{
}
void MouseHandler::OnKeyUp(IntPtr viewerHandlePtr, char key)
{
    native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;

    // Assuming context is passed or available globally in the class
    Handle(AIS_InteractiveContext) context = native->context;
    ViewHelper::ClearCreateEntity(native);
    if (key == 'L' || key == 'l') {
        native->isLineMode = true;
    }
    else if (key == 'B' || key == 'b') {
        native->isBooleanMode = true;
    }
    else if (key == 'C' || key == 'c') {
        native->isCircleMode = true;
    }
    else if (key == 'D' || key == 'd') {
        native->isBooleanCutMode = true;
    }
    else if (key == 'E' || key == 'e') {
        native->isEllipseMode = true;
    }
    else if (key == 'F' || key == 'f') {
        native->isRadiusMode = true;
    }
    else if (key == 'I' || key == 'i') {
        native->isBooleanIntersectMode = true;
    }
    else if (key == 'W' || key == 'w') {
        native->isRevolveMode = true;
    }
    else if (key == 'M' || key == 'm') {
        native->isMoveMode = true;
    }
    else if (key == 'O' || key == 'o') {
        native->isDrawDimensionMode = true;
        native->isPickingDimensionFirstPoint = true;
    }
    else if (key == 'P' || key == 'p') {
        native->isMateAlignmentMode = true;
    }
    else if (key == 'R' || key == 'r') {
        native->isRotateMode = true;
    }
    else if (key == 'S' || key == 's') {
        native->isExtrudeMode = true;
    }
    else if (key == 'T' || key == 't') {
        native->isTrimMode = true;
    }
    else if (key == 'U' || key == 'u') {
        native->isBooleanUnionMode = true;
    }
    else if (key == 'Q' || key == 'q') {
        native->isRectangleMode = true;
    }
    else if (key == 27)
    {
        ViewHelper::ClearCreateEntity(native);
    }
}
void MouseHandler::HandleMouseDownAction(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect)
{
    switch (MouseHandler::GetCurrentMode())
    {
    case MouseMode::MateAlignment:
        MouseHelper::HandleMateModeMouseDown(native, context, view, x, y);
        break;
    case MouseMode::Move:
        MouseHelper::HandleMoveModeMouseDown(native, context, view, x, y);
        break;
    case MouseMode::Rotate:
        MouseHelper::HandleRotateModeMouseDown(native, context, view, x, y);
        break;
    case MouseMode::Default:
    default:
        MouseHelper::HandleDefaultMouseDown(native, context, view, x, y, multipleselect, lastHilightedObjects);
        break;
    }
}
void MouseHandler::ShowAxisTriad(IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    if (native->axisX.IsNull())
    {
        double len = 20.0;
        native->axisX = new AIS_Shape(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(len, 0, 0)));
        native->axisY = new AIS_Shape(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(0, len, 0)));
        native->axisZ = new AIS_Shape(BRepBuilderAPI_MakeEdge(gp_Pnt(0, 0, 0), gp_Pnt(0, 0, len)));
        native->axisX->SetColor(Quantity_NOC_RED); native->axisY->SetColor(Quantity_NOC_GREEN); native->axisZ->SetColor(Quantity_NOC_BLUE);
        native->context->Display(native->axisX, Standard_False);
        native->context->Display(native->axisY, Standard_False);
        native->context->Display(native->axisZ, Standard_True);
    }

    Standard_Real width = 0, height = 0; native->view->Size(width, height);
    gp_Trsf trsf; trsf.SetTranslation(gp_Vec(width - 50, 50, 0));
    native->axisX->SetLocalTransformation(trsf); native->axisY->SetLocalTransformation(trsf); native->axisZ->SetLocalTransformation(trsf);

    native->context->UpdateCurrentViewer(); native->view->Redraw();
}
void MouseHandler::HideAxisTriad(IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    if (!native->axisX.IsNull()) native->context->Erase(native->axisX, Standard_False);
    if (!native->axisY.IsNull()) native->context->Erase(native->axisY, Standard_False);
    if (!native->axisZ.IsNull()) native->context->Erase(native->axisZ, Standard_True);

    native->context->UpdateCurrentViewer(); native->view->Redraw();
}
void MouseHandler::HandleRevolveMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    native->isRevolveMode = false;

    if (!native->revolvePreviewShape.IsNull())
        context->Remove(native->revolvePreviewShape, Standard_False);

    double angleDeg = native->currentRevolveAngle;  // User-defined revolve angle
    ShapeRevolver revolver(native);
    revolver.RevolveWireAndDisplayFinal(context, angleDeg);

    view->Redraw();
}
TopoDS_Shape MouseHandler::PickShapeAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y)
{
    if (context.IsNull())
        return TopoDS_Shape();

    // Step 1: Check if user has already selected shapes in the context
    if (context->HasSelectedShape())
    {
        // Return the first selected shape
        return context->SelectedShape();
    }

    // Step 2: Nothing is selected
    return TopoDS_Shape();
}
TopoDS_Edge MouseHandler::PickEdgeAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y)
{
    TopoDS_Shape shape = PickShapeAtCursor(context, view, x, y);
    if (shape.IsNull()) return TopoDS_Edge();

    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next())
    {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        if (!edge.IsNull()) return edge;
    }

    return TopoDS_Edge();
}
TopoDS_Wire MouseHandler::PickWireAtCursor(const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int x, int y)
{
    TopoDS_Shape shape = PickShapeAtCursor(context, view, x, y);
    if (shape.IsNull()) return TopoDS_Wire();

    if (shape.ShapeType() == TopAbs_WIRE) return TopoDS::Wire(shape);

    if (shape.ShapeType() == TopAbs_FACE)
    {
        TopoDS_Face face = TopoDS::Face(shape);
        TopExp_Explorer exp(face, TopAbs_WIRE);
        if (exp.More()) return TopoDS::Wire(exp.Current());
    }

    BRepBuilderAPI_MakeWire mkWire;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next())
        mkWire.Add(TopoDS::Edge(exp.Current()));

    if (mkWire.IsDone()) return mkWire.Wire();

    return TopoDS_Wire();
}