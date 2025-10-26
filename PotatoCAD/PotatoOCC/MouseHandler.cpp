#include "pch.h"
#include "NativeViewerHandle.h"
#include "ViewerManager.h"
#include "Utils.h"
#include "MouseHandler.h"
#include "MouseCursor.h"
#include "ViewHelper.h"
#include "ShapeDrawer.h"
#include "ShapeExtruder.h"
#include "ShapeBooleanOperator.h"
#include "MateHelper.h"
#include <WNT_Window.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
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
#include <Prs3d_Drawer.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Quantity_Color.hxx>
#include <vcclr.h>
#include <map>
#include <mutex>
#include <vector>
#include <TCollection_ExtendedString.hxx>
#include <TopoDS_Shape.hxx>
#include <BRepTools.hxx>
#include <Standard_Type.hxx>
#include <gcroot.h>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include <TopoDS.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Circle.hxx>
#include <BRep_Tool.hxx>
#include <iostream>
#include <Prs3d_TypeOfHighlight.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TopExp_Explorer.hxx>
#include <TopAbs.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <gp_Ax2.hxx>
#include <TopoDS_Vertex.hxx>
#include <cmath> 
#include "LineDrawer.h"
#include "CircleDrawer.h"
#include "Print.h"
#include <BRepAlgoAPI_Section.hxx>
#include <GC_MakeSegment.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <WinSock2.h>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <TopExp.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <NCollection_TListIterator.hxx>
#include <gp_Circ.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <ShapeFix_Wire.hxx>


#using <System.Windows.Forms.dll>
using namespace System;
using namespace System::Windows::Forms;
using namespace PotatoOCC;

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

gp_Pnt CalculateFilletCenter(const gp_Pnt& jointPoint, const gp_Dir& dir1, const gp_Dir& dir2, double filletRadius)
{
    // Compute bisector direction (normalize the sum of two vectors)
    gp_Vec vec1 = dir1.XYZ();  // Convert direction to vector
    gp_Vec vec2 = dir2.XYZ();  // Convert direction to vector

    // Normalize the vectors first
    vec1.Normalize();
    vec2.Normalize();

    // The bisector vector points inward between the two direction vectors
    gp_Vec bisectorVec = vec1 + vec2;

    // Normalize the bisector to ensure consistent magnitude
    bisectorVec.Normalize();

    // Compute the center of the fillet along the bisector at a distance equal to the radius
    gp_Pnt center = jointPoint.Translated(bisectorVec * filletRadius);

    // Ensure the center lies in the XY-plane (Z = 0)
    center.SetZ(0);  // Explicitly set Z to 0

    std::cout << "Center Point: X=" << center.X() << ", Y=" << center.Y() << ", Z=" << center.Z() << std::endl;

    return center;
}

TopoDS_Edge CreateFilletArc(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& center)
{
    // Create an arc of a circle (fillet)
    Handle(Geom_TrimmedCurve) arc = GC_MakeArcOfCircle(p1, center, p2);
    return BRepBuilderAPI_MakeEdge(arc);
}

Handle(AIS_Shape) MouseHandler::DrawLine(Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w)
{
    return LineDrawer::DrawLine(view, viewerHandlePtr, native->dragStartX, native->dragStartY, native->dragEndX, native->dragEndY, h, w);
}

Handle(AIS_Shape) MouseHandler::DrawCircle(Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w)
{
    return new AIS_Shape(CircleDrawer::DrawCircle(view, viewerHandlePtr, native->dragStartX, native->dragStartY, native->dragEndX, native->dragEndY, h, w));
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

// ----------------------- Camera -----------------------
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
            DrawLineOverlay(native, h);
        }
        else if (native->isCircleMode)
        {
            DrawCircleOverlay(native, h);
        }
        else if (native->isTrimMode)
        {
            // TODO: Implement trim mode if needed
        }
        else if (native->isBooleanUnionMode || native->isBooleanCutMode || native->isBooleanIntersectMode)
        {
            HighlightHoveredShape(native, x, y, context, view);
        }
        else
        {
            DrawSelectionRectangle(native);
            view->Redraw();
        }
    }
    else if (native->isMoveMode)
    {
        HandleMove(native, x, y, context, view);
    }
    else if (native->isRotateMode && native->isRotating)
    {
        HandleRotate(native, x, y, context, view);
    }
    else if (native->isExtrudingActive)
    {
        HandleExtrude(native, context, view, y);
    }
    else
    {
        UpdateHoverDetection(native, x, y, context, view);
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
        HandleLineMode(context, view, viewerHandlePtr, h, w, x, y);
        view->Redraw();
        return;
    }

    if (native->isCircleMode) {
        HandleCircleMode(context, view, viewerHandlePtr, h, w);
        view->Redraw();
        return;
    }

    if (native->isTrimMode) {
        HandleTrimMode(context);
        view->Redraw();
        return;
    }

    if (native->isRadiusMode) {
        HandleRadiusMode(context);
        view->Redraw();
        return;
    }

    if (native->isExtrudingActive) {
        HandleExtrudingMode(context, view);
        return;
    }

    if (native->isBooleanUnionMode || native->isBooleanCutMode || native->isBooleanIntersectMode) {
        HandleBooleanMode(context, view, x, y);
        view->Redraw();
        return;
    }

    if (native->isMoveMode && native->isMoving) {
        native->isMoving = false;
        native->movingObject.Nullify();
        view->Redraw();
        return;
    }

    if (native->isRotateMode && native->isRotating) {
        native->isRotating = false;
        native->rotatingObject.Nullify();
        view->Redraw();
        return;
    }

    // Default behavior
    if (native->isDragging && native->dragEndX != 0) {
        PerformRectangleSelection(context, view, h, w, y);
    }
    else {
        HandleObjectDetection(context, view, x, y);
    }

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
    else if (key == 'E' || key == 'e') { //extrude
        if (native->persistedLines.size() >= 2) {

            Handle(TopTools_HSequenceOfShape) edgesSeq = new TopTools_HSequenceOfShape();

            for (auto& aisLine : native->persistedLines) {
                TopoDS_Shape shape = aisLine->Shape();
                if (!shape.IsNull() && shape.ShapeType() == TopAbs_EDGE) {
                    edgesSeq->Append(shape);
                }
            }

            if (edgesSeq->Length() < 3) {
                std::cout << "Not enough valid edges to create wire." << std::endl;
                return;
            }

            // Convert sequence to list
            NCollection_List<TopoDS_Shape> edgesList;
            for (int i = 1; i <= edgesSeq->Length(); ++i) {
                edgesList.Append(edgesSeq->Value(i));
            }

            // === DEBUG OUTPUT for selected edges ===
            std::cout << "======== Selected Lines Debug ========" << std::endl;
            for (NCollection_List<TopoDS_Shape>::Iterator it(edgesList); it.More(); it.Next()) {
                TopoDS_Edge edge = TopoDS::Edge(it.Value());
                gp_Pnt p1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
                gp_Pnt p2 = BRep_Tool::Pnt(TopExp::LastVertex(edge));
                std::cout << "Edge start: (" << p1.X() << ", " << p1.Y() << ", " << p1.Z() << ")"
                    << "  end: (" << p2.X() << ", " << p2.Y() << ", " << p2.Z() << ")" << std::endl;
            }
            std::cout << "=====================================" << std::endl;
            // === END DEBUG OUTPUT ===

            // Remove duplicates by building a new list skipping duplicates
            NCollection_List<TopoDS_Shape> uniqueEdges;
            for (NCollection_List<TopoDS_Shape>::Iterator it(edgesList); it.More(); it.Next()) {
                TopoDS_Edge currentEdge = TopoDS::Edge(it.Value());
                bool foundDuplicate = false;
                for (NCollection_List<TopoDS_Shape>::Iterator uniqueIt(uniqueEdges); uniqueIt.More(); uniqueIt.Next()) {
                    TopoDS_Edge uniqueEdge = TopoDS::Edge(uniqueIt.Value());
                    if (AreEdgesSame(currentEdge, uniqueEdge)) {
                        foundDuplicate = true;
                        break;
                    }
                }
                if (!foundDuplicate) {
                    uniqueEdges.Append(currentEdge);
                }
            }

            if (uniqueEdges.Size() < 3) {
                std::cout << "Not enough unique edges to create wire." << std::endl;
                return;
            }

            // Convert uniqueEdges list to vector for sorting
            std::vector<TopoDS_Edge> edgeVec;
            for (NCollection_List<TopoDS_Shape>::Iterator it(uniqueEdges); it.More(); it.Next()) {
                edgeVec.push_back(TopoDS::Edge(it.Value()));
            }

            // Sort them by connectivity
            auto sortedEdges = SortEdgesByConnectivityRobust(edgeVec, 5.0);

            std::cout << "Sorted edges count: " << sortedEdges.size() << std::endl;

            // Build wire
            BRepBuilderAPI_MakeWire wireMaker;
            for (auto& e : sortedEdges) {
                wireMaker.Add(e);
            }


            if (!wireMaker.IsDone()) {
                std::cout << "Failed to create wire from lines!" << std::endl;
                return;
            }

            TopoDS_Wire wire = wireMaker.Wire();

            if (!ShapeDrawer::IsClosedWire(wire)) {
                std::cout << "The wire is not closed, cannot extrude!" << std::endl;
                return;
            }

            native->persistedLines.clear();

            Handle(AIS_Shape) aisWire = new AIS_Shape(wire);
            native->context->Display(aisWire, Standard_True);
            native->persistedLines.push_back(aisWire);

            std::cout << "Successfully created closed wire from selected lines!" << std::endl;
        }
    }
    else if (key == 'F' || key == 'f') {
        native->isRadiusMode = true;
    }
    else if (key == 'I' || key == 'i') {
        native->isBooleanIntersectMode = true;
    }
    else if (key == 'M' || key == 'm') {
        native->isMoveMode = true;
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
    else if (key == 27)
    {
        ViewHelper::ClearCreateEntity(native);
    }
}

void MouseHandler::PerformRectangleSelection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int h, int w, int y)
{
    int xMin = std::min(native->dragStartX, native->dragEndX);
    int yMin = std::min(native->dragStartY, native->dragEndY);
    int xMax = std::max(native->dragStartX, native->dragEndX);
    int yMax = std::max(native->dragStartY, native->dragEndY);

    // Perform selection
    context->Select(xMin, yMin, xMax, yMax, view, Standard_False);

    bool isAnyObjectSelected = false;  // Flag to track if any object was selected
    AIS_ListOfInteractive picked;
    for (context->InitSelected(); context->MoreSelected(); context->NextSelected()) {
        Handle(AIS_InteractiveObject) obj = context->SelectedInteractive();
        if (!obj.IsNull()) {
            picked.Append(obj);
            isAnyObjectSelected = true;  // Set flag if an object is selected
        }
    }

    // 🟨 Highlight selected objects visually, but only if there are selected objects
    ShapeDrawer::HighlightSelectedObjects(context, picked);

    if (isAnyObjectSelected && native->isExtrudeMode) {
        for (auto& obj : picked) {
            // Check if the object is of type AIS_Shape
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
            if (!aisShape.IsNull()) {
                TopoDS_Shape shape = aisShape->Shape();
                if (!shape.IsNull()) {
                    if (shape.ShapeType() == TopAbs_EDGE) {
                        // 🧱 Process the edge and extrude using helper
                        TopoDS_Edge detectedEdge = TopoDS::Edge(shape);
                        if (detectedEdge.IsNull()) {
                            std::cout << "Edge is invalid!" << std::endl;
                            return;
                        }
                        TopoDS_Wire wire = ShapeDrawer::BuildWireFromSelection(picked);
                        if (wire.IsNull())
                        {
                            // Wire is invalid so use  detectedEdge for extrusion
                            wire = BRepBuilderAPI_MakeWire(detectedEdge);
                        }

                        native->activeWire = wire;
                        native->isExtrudingActive = true;
                        native->currentExtrudeHeight = 0.0;
                        native->lastMouseY = y;

                        // Compute centroid as base
                        Bnd_Box bbox;
                        BRepBndLib::Add(native->activeWire, bbox);

                        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

                        gp_Pnt pMin(xmin, ymin, zmin);
                        gp_Pnt pMax(xmax, ymax, zmax);

                        gp_Pnt centroid((xmin + xmax) / 2.0,
                            (ymin + ymax) / 2.0,
                            (zmin + zmax) / 2.0);

                        native->extrudeBasePoint = centroid;
                        /*std::cout << "Extrusion base point: "
                            << centroid.X() << ", "
                            << centroid.Y() << ", "
                            << centroid.Z() << std::endl;*/


                            //std::cout << "🟩 Extrude mode started. Move mouse up/down to set height.\n";
                    }
                    else if (shape.ShapeType() == TopAbs_WIRE)
                    {
                        // 🧱 Process the wire and extrude using helper
                        TopoDS_Wire detectedWire = TopoDS::Wire(shape);
                        ShapeExtruder::ExtrudeWireAndDisplay(context, detectedWire);
                    }
                    else
                    {
                        std::cout << "Detected shape is not a closed wire!" << std::endl;
                    }
                }
            }
        }
    }

    // Reset drag state and clear rubber band after selection
    ResetDragState();
    ClearRubberBand();
}
void MouseHandler::ResetDragState()
{
    native->isDragging = false;
    native->dragEndX = 0;
}
void MouseHandler::ClearRubberBand()
{
    if (!native->rubberBandOverlay.IsNull()) {
        native->rubberBandOverlay->Erase();
        native->rubberBandOverlay.Nullify();
        native->rubberBandGroup.Nullify();
    }
}
void MouseHandler::HandleObjectDetection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
{
    if (context->HasDetected())
    {
        Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
        if (!detectedIO.IsNull()) {
            Handle(AIS_TextLabel) textLabel = Handle(AIS_TextLabel)::DownCast(detectedIO);
            if (!textLabel.IsNull()) {
                TCollection_ExtendedString text = textLabel->Text();
                std::wcout << L"Text Selected: " << text.ToExtString() << std::endl;
                return;
            }

            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(detectedIO);
            if (!aisShape.IsNull()) {
                Print::ShapeDetails(aisShape);
            }
            else {
                std::cout << "Detected object is not AIS_Shape or AIS_TextLabel." << std::endl;
            }
        }
    }
    else {
        //std::cout << "No object detected at the cursor." << std::endl;
        //std::cout << "Mouse coordinates: (" << x << ", " << y << ")" << std::endl;
    }
}

void MouseHandler::DrawLineOverlay(NativeViewerHandle* native, int height)
{
    if (native->lineOverlay.IsNull())
        native->lineOverlay = new AIS_OverlayLine();

    int startYFlipped = height - native->dragStartY;
    int endYFlipped = height - native->dragEndY;

    native->lineOverlay->SetEndpoints(
        native->dragStartX,
        startYFlipped,
        native->dragEndX,
        endYFlipped);

    native->context->Display(native->lineOverlay, Standard_True);
}
void MouseHandler::DrawCircleOverlay(NativeViewerHandle* native, int height)
{
    if (native->circleOverlay.IsNull())
        native->circleOverlay = new AIS_OverlayCircle();

    double dx = native->dragEndX - native->dragStartX;
    double dy = native->dragEndY - native->dragStartY;
    double radius = std::sqrt(dx * dx + dy * dy);

    int centerYFlipped = height - native->dragStartY;

    native->circleOverlay->SetCircle(
        gp_Pnt(native->dragStartX, centerYFlipped, 0.0),
        radius);

    native->context->Display(native->circleOverlay, Standard_True);
}
void MouseHandler::DrawSelectionRectangle(NativeViewerHandle* native)
{
    int xMin = std::min(native->dragStartX, native->dragEndX);
    int yMin = std::min(native->dragStartY, native->dragEndY);
    int xMax = std::max(native->dragStartX, native->dragEndX);
    int yMax = std::max(native->dragStartY, native->dragEndY);

    Quantity_Color red(1.0, 0.0, 0.0, Quantity_TOC_RGB);
    MouseCursor::DrawRubberBandRectangle(native, xMin, yMin, xMax, yMax);
}
void MouseHandler::HighlightHoveredShape(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    context->Deactivate();
    context->Activate(TopAbs_SHAPE);
    context->MoveTo(x, y, view, Standard_True);

    if (context->HasDetected())
    {
        Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
        if (!detectedIO.IsNull())
        {
            Quantity_Color hoverColor(Quantity_NOC_YELLOW);
            context->HighlightStyle(Prs3d_TypeOfHighlight_Dynamic)->SetColor(hoverColor);
            MouseCursor::SetCustomCursor(native, PotatoOCC::CursorType::Crosshair);
        }
    }
    else
    {
        context->Deactivate();
        context->Activate(TopAbs_SHAPE);
        MouseCursor::SetCustomCursor(native, PotatoOCC::CursorType::Default);
    }
}
void MouseHandler::HandleMove(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    Standard_Real X, Y, Z;
    view->Convert(x, y, X, Y, Z);
    gp_Pnt currentPoint(X, Y, Z);

    gp_Vec moveVec(native->lastMousePoint, currentPoint);
    if (moveVec.Magnitude() < 1e-6)
        return;

    Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(native->movingObject);
    if (aisShape.IsNull())
        return;

    TopoDS_Shape originalShape = aisShape->Shape();

    gp_Trsf trsf;
    trsf.SetTranslation(moveVec);

    TopoDS_Shape movedShape = BRepBuilderAPI_Transform(originalShape, trsf).Shape();

    aisShape->Set(movedShape);

    context->MoveTo(x, y, view, Standard_True);
    context->Redisplay(aisShape, Standard_False);
    view->Redraw();

    native->lastMousePoint = currentPoint;
}
void MouseHandler::HandleRotate(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    int deltaX = x - native->lastMouseX;
    int deltaY = y - native->lastMouseY;
    double sensitivity = 0.5; // degrees per pixel

    double angleX = deltaY * sensitivity * (M_PI / 180.0);  // rotate around X-axis
    double angleZ = deltaX * sensitivity * (M_PI / 180.0);  // rotate around Z-axis

    Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(native->rotatingObject);
    if (aisShape.IsNull())
        return;

    TopoDS_Shape originalShape = aisShape->Shape();

    // Compute bounding box center for rotation pivot
    Bnd_Box bbox;
    BRepBndLib::Add(originalShape, bbox);
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

    gp_Ax1 axisX(center, gp::DX());
    gp_Ax1 axisZ(center, gp::DZ());

    gp_Trsf trsfX, trsfZ;
    trsfX.SetRotation(axisX, angleX);
    trsfZ.SetRotation(axisZ, angleZ);

    gp_Trsf combinedTrsf = trsfX.Multiplied(trsfZ);

    TopoDS_Shape rotatedShape = BRepBuilderAPI_Transform(originalShape, combinedTrsf).Shape();
    aisShape->Set(rotatedShape);

    context->MoveTo(x, y, view, Standard_True);
    context->Redisplay(aisShape, Standard_False);
    view->Redraw();

    native->lastMouseX = x;
    native->lastMouseY = y;
}
void MouseHandler::HandleExtrude(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int y)
{
    int dy = y - native->lastMouseY;
    native->currentExtrudeHeight = -dy * 1.0;  // scale factor

    if (native->currentExtrudeHeight == 0)
    {
        view->Redraw();
        return;
    }

    if (!native->extrudePreviewShape.IsNull())
        context->Remove(native->extrudePreviewShape, Standard_False);

    ShapeExtruder extruder(native);
    extruder.ExtrudeWireAndDisplayPreview(context);

    view->Redraw();
}
void MouseHandler::UpdateHoverDetection(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    context->Deactivate();
    context->Activate(TopAbs_SHAPE);
    context->MoveTo(x, y, view, Standard_True);

    if (context->HasDetected())
    {
        Quantity_Color hoverColor(Quantity_NOC_RED);
        context->HighlightStyle(Prs3d_TypeOfHighlight_Dynamic)->SetColor(hoverColor);

        MouseCursor::SetCustomCursor(native, PotatoOCC::CursorType::Crosshair);

        if (!native->isLineMode) return;

        Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
        if (detectedIO.IsNull()) return;

        TopoDS_Shape shape = context->DetectedShape();
        if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) return;

        TopoDS_Edge edge = TopoDS::Edge(shape);
        BRepAdaptor_Curve curveAdaptor(edge);
        if (curveAdaptor.GetType() != GeomAbs_Line) return;

        gp_Lin line = curveAdaptor.Line();

        Standard_Real u1 = curveAdaptor.FirstParameter();
        Standard_Real u2 = curveAdaptor.LastParameter();

        gp_Pnt p1 = curveAdaptor.Value(u1);
        gp_Pnt p2 = curveAdaptor.Value(u2);

        gp_Pnt lineCenter(
            (p1.X() + p2.X()) / 2.0,
            (p1.Y() + p2.Y()) / 2.0,
            (p1.Z() + p2.Z()) / 2.0
        );

        ViewHelper::DrawCenterMarker(native, lineCenter.X(), lineCenter.Y(), lineCenter.Z());
    }
    else
    {
        MouseCursor::SetCustomCursor(native, PotatoOCC::CursorType::Default);
        ViewHelper::ClearCenterMarker(native);
    }
}
void MouseHandler::HandleLineMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
{
    Handle(AIS_Shape) aisLine = DrawLine(view, viewerHandlePtr, h, w);
    context->Display(aisLine, Standard_True);
    native->persistedLines.push_back(aisLine);
    native->dragStartX = x;
    native->dragStartY = y;
}
void MouseHandler::HandleCircleMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w)
{
    Handle(AIS_Shape) aisCircle = DrawCircle(view, viewerHandlePtr, h, w);
    context->Display(aisCircle, Standard_True);
    native->persistedCircles.push_back(aisCircle);
    ViewHelper::ClearCreateEntity(native);
}
void MouseHandler::HandleTrimMode(Handle(AIS_InteractiveContext) context)
{
    if (!context->HasDetected()) return;

    Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
    if (detectedIO.IsNull()) return;

    TopoDS_Shape shape = context->DetectedShape();
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) return;

    TopoDS_Edge selectedEdge = TopoDS::Edge(shape);

    Standard_Real f, l;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(selectedEdge, f, l);

    bool trimmed = false;

    for (auto& otherLineAIS : native->persistedLines) {
        if (otherLineAIS == detectedIO) continue;

        TopoDS_Shape otherShape = otherLineAIS->Shape();
        if (otherShape.ShapeType() != TopAbs_EDGE) continue;

        TopoDS_Edge otherEdge = TopoDS::Edge(otherShape);

        BRepAlgoAPI_Section section(selectedEdge, otherEdge, Standard_False);
        section.ComputePCurveOn1(Standard_True);
        section.Build();

        if (!section.IsDone() || section.Shape().IsNull()) continue;

        TopExp_Explorer exp(section.Shape(), TopAbs_VERTEX);
        while (exp.More()) {
            TopoDS_Vertex vertex = TopoDS::Vertex(exp.Current());
            gp_Pnt intersectionPoint = BRep_Tool::Pnt(vertex);

            GeomAPI_ProjectPointOnCurve projector(intersectionPoint, curve);
            if (projector.NbPoints() == 0) {
                exp.Next();
                continue;
            }

            try {
                double param = projector.LowerDistanceParameter();
                const double tol = 1e-12;
                if (fabs(f - param) < tol) {
                    exp.Next();
                    continue;
                }

                Handle(Geom_TrimmedCurve) trimmedCurve = new Geom_TrimmedCurve(curve, f, param);
                TopoDS_Edge trimmedEdge = BRepBuilderAPI_MakeEdge(trimmedCurve);
                Handle(AIS_Shape) trimmedAIS = new AIS_Shape(trimmedEdge);
                trimmedAIS->SetDisplayMode(AIS_WireFrame);

                auto& lines = native->persistedLines;
                int indexToRemove = -1;
                for (int i = 0; i < (int)lines.size(); i++) {
                    if (lines[i].get() == detectedIO.get()) {
                        indexToRemove = i;
                        break;
                    }
                }

                if (indexToRemove != -1)
                    lines.erase(lines.begin() + indexToRemove);

                context->Remove(detectedIO, Standard_True);

                trimmed = true;
                break;
            }
            catch (Standard_Failure& e) {
                throw gcnew System::Exception(gcnew System::String(e.GetMessageString()));
            }
        }
        if (trimmed) break;
    }

    std::cout << "persistedLines After Remove : " << native->persistedLines.size() << std::endl;
}
void MouseHandler::HandleRadiusMode(Handle(AIS_InteractiveContext) context)
{
    if (!context->HasDetected()) return;

    Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
    if (detectedIO.IsNull()) return;

    TopoDS_Shape shape = context->DetectedShape();
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) return;

    TopoDS_Edge selectedEdge = TopoDS::Edge(shape);

    // Store selected edges (max 2)
    if (native->selectedEdges.size() < 2)
        native->selectedEdges.push_back(selectedEdge);

    if (native->selectedEdges.size() != 2) return;

    // Extract edges
    TopoDS_Edge edge1 = native->selectedEdges[0];
    TopoDS_Edge edge2 = native->selectedEdges[1];

    // Get endpoints of edges
    gp_Pnt e1v1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge1));
    gp_Pnt e1v2 = BRep_Tool::Pnt(TopExp::LastVertex(edge1));
    gp_Pnt e2v1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge2));
    gp_Pnt e2v2 = BRep_Tool::Pnt(TopExp::LastVertex(edge2));

    const double tol = 1e-4;
    gp_Pnt jointPoint;
    bool foundShared = false;

    // Detect shared vertex
    if (e1v1.IsEqual(e2v1, tol)) { jointPoint = e1v1; foundShared = true; }
    else if (e1v1.IsEqual(e2v2, tol)) { jointPoint = e1v1; foundShared = true; }
    else if (e1v2.IsEqual(e2v1, tol)) { jointPoint = e1v2; foundShared = true; }
    else if (e1v2.IsEqual(e2v2, tol)) { jointPoint = e1v2; foundShared = true; }

    // If no shared vertex, compute 2D intersection manually (safe for C++/CLI)
    if (!foundShared)
    {
        double x1 = e1v1.X(), y1 = e1v1.Y();
        double x2 = e1v2.X(), y2 = e1v2.Y();
        double x3 = e2v1.X(), y3 = e2v1.Y();
        double x4 = e2v2.X(), y4 = e2v2.Y();

        double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (fabs(denom) < 1e-9)
        {
            std::cout << "Edges are parallel, cannot fillet.\n";
            native->selectedEdges.clear();
            return;
        }

        double det1 = x1 * y2 - y1 * x2;
        double det2 = x3 * y4 - y3 * x4;
        double px = (det1 * (x3 - x4) - (x1 - x2) * det2) / denom;
        double py = (det1 * (y3 - y4) - (y1 - y2) * det2) / denom;

        jointPoint.SetX(px);
        jointPoint.SetY(py);
        jointPoint.SetZ(0.0);
        foundShared = true;
    }

    // Build direction vectors from joint point to other vertices
    gp_Pnt other1 = (jointPoint.IsEqual(e1v1, tol) ? e1v2 : e1v1);
    gp_Pnt other2 = (jointPoint.IsEqual(e2v1, tol) ? e2v2 : e2v1);

    gp_Vec v1(jointPoint, other1);
    gp_Vec v2(jointPoint, other2);

    if (v1.Magnitude() < 1e-9 || v2.Magnitude() < 1e-9)
    {
        std::cout << "Zero-length vector detected.\n";
        native->selectedEdges.clear();
        return;
    }

    v1.Normalize();
    v2.Normalize();

    // Compute interior angle between edges
    double angle = v1.Angle(v2);
    if (angle < 1e-6 || angle > M_PI - 1e-6)
    {
        std::cout << "Angle invalid / collinear: angle = " << angle << "\n";
        native->selectedEdges.clear();
        return;
    }

    // Fillet geometry parameters
    double radius = 20.0;
    double halfAngle = angle / 2.0;
    double offset = radius / tan(halfAngle);
    double distToCenter = radius / sin(halfAngle);

    gp_Vec bisector = v1 + v2;
    if (bisector.Magnitude() < 1e-9)
    {
        std::cout << "Bisector degenerate.\n";
        native->selectedEdges.clear();
        return;
    }
    bisector.Normalize();

    // Flip bisector if needed to ensure arc lies inside the angle
    gp_Vec cross = v1.Crossed(v2);
    if (cross.Z() < 0) bisector.Reverse();

    // Compute tangent points and center of fillet arc
    gp_Pnt tangent1 = jointPoint.Translated(v1 * offset);
    gp_Pnt tangent2 = jointPoint.Translated(v2 * offset);
    gp_Pnt center = jointPoint.Translated(bisector * distToCenter);

    std::cout << std::fixed << std::setprecision(6)
        << "joint: " << jointPoint.X() << "," << jointPoint.Y() << "\n"
        << "angle: " << angle << "  offset: " << offset
        << "  distToCenter: " << distToCenter << "\n";

    // Create fillet arc
    Handle(Geom_TrimmedCurve) arc;
    try {
        arc = GC_MakeArcOfCircle(tangent1, center, tangent2);
    }
    catch (...) {
        try { arc = GC_MakeArcOfCircle(tangent2, center, tangent1); }
        catch (...) {
            std::cout << "GC_MakeArcOfCircle failed\n";
            native->selectedEdges.clear();
            return;
        }
    }

    TopoDS_Edge filletArc = BRepBuilderAPI_MakeEdge(arc);

    // Create trimmed edges from original edges to tangent points
    TopoDS_Edge edgeTrim1 = BRepBuilderAPI_MakeEdge(other1, tangent1);
    TopoDS_Edge edgeTrim2 = BRepBuilderAPI_MakeEdge(other2, tangent2);

    // Combine trimmed edges and fillet arc into a wire
    BRepBuilderAPI_MakeWire wireBuilder;
    wireBuilder.Add(edgeTrim1);
    wireBuilder.Add(filletArc);
    wireBuilder.Add(edgeTrim2);

    if (!wireBuilder.IsDone())
    {
        std::cout << "Wire builder failed.\n";
        native->selectedEdges.clear();
        return;
    }

    // Display the new wire as AIS shape
    Handle(AIS_Shape) aisWire = new AIS_Shape(wireBuilder.Wire());
    aisWire->SetDisplayMode(AIS_WireFrame);
    native->context->Display(aisWire, Standard_True);

    // Clear previous lines and selected edges
    native->persistedLines.clear();
    native->persistedLines.push_back(aisWire);
    native->selectedEdges.clear();
}
void MouseHandler::HandleExtrudingMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
{
    native->isExtrudingActive = false;

    if (!native->extrudePreviewShape.IsNull())
        context->Remove(native->extrudePreviewShape, Standard_False);

    double finalHeight = native->currentExtrudeHeight;
    ShapeExtruder extruder(native);
    extruder.ExtrudeWireAndDisplayFinal(context, finalHeight);

    view->Redraw();
}
void MouseHandler::HandleBooleanMode(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
{
    native->isDragging = false;

    context->Activate(TopAbs_SHAPE);
    context->MoveTo(x, y, view, Standard_True);

    if (!context->HasDetected()) return;

    Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
    if (detectedIO.IsNull()) return;

    auto it = std::find(native->selectedShapes.begin(), native->selectedShapes.end(), detectedIO);
    if (it != native->selectedShapes.end()) {
        context->AddOrRemoveSelected(detectedIO, Standard_True);
        native->selectedShapes.erase(it);
    }
    else {
        context->AddOrRemoveSelected(detectedIO, Standard_True);
        native->selectedShapes.push_back(detectedIO);
    }

    view->Redraw();

    if (native->selectedShapes.size() != 2) return;

    Handle(AIS_Shape) aisShape1 = Handle(AIS_Shape)::DownCast(native->selectedShapes[0]);
    Handle(AIS_Shape) aisShape2 = Handle(AIS_Shape)::DownCast(native->selectedShapes[1]);

    if (aisShape1.IsNull() || aisShape2.IsNull()) return;

    TopoDS_Shape shape1 = aisShape1->Shape();
    TopoDS_Shape shape2 = aisShape2->Shape();

    if (shape1.IsNull() || shape2.IsNull()) return;

    bool success;
    TopoDS_Shape resultShape = ShapeBooleanOperator::PerformBooleanOperation(shape1, shape2, native, context, view, success);

    if (native->isBooleanCutMode)
        ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape,
            "Boolean Cut operation succeeded.",
            "Boolean Cut operation failed.", success);
    else if (native->isBooleanUnionMode)
        ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape,
            "Boolean Union operation succeeded.",
            "Boolean Union operation failed.", success);
    else if (native->isBooleanIntersectMode)
        ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape,
            "Boolean Intersect operation succeeded.",
            "Boolean Intersect operation failed.", success);
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
void MouseHandler::HandleMouseDownAction(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect)
{
    switch (MouseHandler::GetCurrentMode())
    {
    case MouseMode::MateAlignment:
        HandleMateModeMouseDown(context, view, x, y);
        break;
    case MouseMode::Move:
        HandleMoveModeMouseDown(context, view, x, y);
        break;
    case MouseMode::Rotate:
        HandleRotateModeMouseDown(context, view, x, y);
        break;
    case MouseMode::Default:
    default:
        HandleDefaultMouseDown(context, view, x, y, multipleselect);
        break;
    }
}
void MouseHandler::HandleMoveModeMouseDown(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
{
    context->MoveTo(x, y, view, Standard_True);
    if (!context->HasDetected()) return;

    native->movingObject = context->DetectedInteractive();
    if (native->movingObject.IsNull()) return;

    Standard_Real X, Y, Z;
    view->Convert(x, y, X, Y, Z);
    native->lastMousePoint = gp_Pnt(X, Y, Z);
    native->isMoving = true;

    //std::cout << "🎯 Selected object to move.\n";
}
void MouseHandler::HandleRotateModeMouseDown(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
{
    context->MoveTo(x, y, view, Standard_True);
    if (!context->HasDetected()) return;

    native->rotatingObject = context->DetectedInteractive();
    if (native->rotatingObject.IsNull()) return;

    native->lastMouseX = x;
    native->lastMouseY = y;
    native->isRotating = true;

    //std::cout << "🎯 Selected object to rotate.\n";
}
void MouseHandler::HandleDefaultMouseDown(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect)
{
    context->Activate(TopAbs_SHAPE);
    context->MoveTo(x, y, view, Standard_True);

    if (native->isCircleMode || native->dragStartX == 0) {
        native->dragStartX = x;
        native->dragStartY = y;
    }

    if (context->HasDetected()) {
        Handle(AIS_InteractiveObject) detectedIO = context->DetectedInteractive();
        if (detectedIO.IsNull()) {
            std::cout << "No interactive object detected!" << std::endl;
            return;
        }

        Quantity_Color mySelectColor(Quantity_NOC_GREEN);
        context->HighlightStyle(Prs3d_TypeOfHighlight_Selected)->SetColor(mySelectColor);

        auto it = std::find(lastHilightedObjects.begin(), lastHilightedObjects.end(), detectedIO);

        if (it != lastHilightedObjects.end()) {
            context->Unhilight(detectedIO, Standard_False);
            lastHilightedObjects.erase(it);
        }
        else {
            context->AddOrRemoveSelected(detectedIO, Standard_False);
            lastHilightedObjects.push_back(detectedIO);
        }
    }
    else {
        if (multipleselect) {
            native->isDragging = true;
            native->dragStartX = x;
            native->dragStartY = y;
        }
    }
}
void MouseHandler::HandleMateModeMouseDown(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y) {
    // ✅ Ensure we are detecting faces
    context->Deactivate();
    context->Activate(TopAbs_FACE);

    context->MoveTo(x, y, view, Standard_True);
    if (!context->HasDetected())
    {
        std::cout << "⚠️ No object detected under cursor!" << std::endl;
        return;
    }

    Handle(AIS_Shape) detected = Handle(AIS_Shape)::DownCast(context->DetectedInteractive());
    if (detected.IsNull())
    {
        std::cout << "⚠️ No valid shape detected!" << std::endl;
        return;
    }

    // ✅ Try to get the actual face that was clicked
    TopoDS_Shape subShape = context->DetectedShape();
    if (subShape.IsNull())
    {
        std::cout << "⚠️ DetectedShape() returned null, trying fallback method..." << std::endl;

        Handle(SelectMgr_EntityOwner) owner = context->DetectedOwner();
        if (!owner.IsNull() && owner->HasSelectable())
        {
            subShape = Handle(AIS_Shape)::DownCast(owner->Selectable())->Shape();
        }
    }

    if (subShape.IsNull() || subShape.ShapeType() != TopAbs_FACE)
    {
        std::cout << "⚠️ No face detected (maybe edge or vertex selected)!" << std::endl;
        return;
    }

    TopoDS_Face clickedFace = TopoDS::Face(subShape);

    // 🟢 First click
    if (!native->hasFirstMateSelected)
    {
        native->firstMateShape = detected;
        native->firstMateFace = new TopoDS_Face(clickedFace);
        native->hasFirstMateSelected = true;
        //std::cout << "✅ First face selected for mate alignment." << std::endl;
    }
    else
    {
        if (native->firstMateShape.IsNull() || native->firstMateFace == nullptr)
        {
            std::cout << "❌ Invalid first selection!" << std::endl;
            native->hasFirstMateSelected = false;
            return;
        }

        TopoDS_Face face1 = *(native->firstMateFace);
        TopoDS_Face face2 = clickedFace;

        //std::cout << "🔄 Aligning selected faces..." << std::endl;

        MateHelper::HandleFaceAlignment(context, view,
            native->firstMateShape,
            face1,
            detected,
            face2);

        delete native->firstMateFace;
        native->firstMateFace = nullptr;
        native->hasFirstMateSelected = false;

        //std::cout << "✅ Mate alignment completed." << std::endl;
    }

    view->Redraw();
};