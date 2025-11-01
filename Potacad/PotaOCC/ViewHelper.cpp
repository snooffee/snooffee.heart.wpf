#include "pch.h"
#include "ViewHelper.h"
#include <V3d_View.hxx>
#include "NativeViewerHandle.h"
#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Quantity_Color.hxx>
#include <gp_Pnt.hxx>

using namespace PotaOCC;

// Define the Marker class outside of the function to avoid the "local class definition" error
class Marker : public AIS_InteractiveObject
{
public:
    double mx, my, mz;
    Marker(double x, double y, double z) : mx(x), my(y), mz(z) {}

    virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& pm,
        const Handle(Prs3d_Presentation)& pres,
        const Standard_Integer mode) override
    {
        pres->Clear();

        const Standard_Real half = 3.0; // Length of the marker lines

        // Each pair of vertices defines a line segment (so 4 vertices = 2 segments)
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(4);

        // Horizontal line (along X axis)
        segs->AddVertex(gp_Pnt(mx - half, my, mz));
        segs->AddVertex(gp_Pnt(mx + half, my, mz));

        // Vertical line (along Y axis)
        segs->AddVertex(gp_Pnt(mx, my - half, mz));
        segs->AddVertex(gp_Pnt(mx, my + half, mz));

        Handle(Graphic3d_Group) group = pres->CurrentGroup();
        Handle(Prs3d_LineAspect) aspect = new Prs3d_LineAspect(Quantity_NOC_RED, Aspect_TOL_SOLID, 1.0);
        group->SetPrimitivesAspect(aspect->Aspect());  // Set line style for the marker
        group->AddPrimitiveArray(segs);  // Add the lines to the group
    }

    virtual void ComputeSelection(const Handle(SelectMgr_Selection)&,
        const Standard_Integer) override
    {
        // No selection needed
    }
};

void ViewHelper::ResetView(System::IntPtr viewPtr)
{
    if (viewPtr == System::IntPtr::Zero) return;
    Handle(V3d_View) view =
        reinterpret_cast<V3d_View*>(viewPtr.ToPointer());

    if (view.IsNull()) return;

    view->SetProj(V3d_Zpos);
    view->SetTwist(0.0);
    view->FitAll();
    view->Redraw();
}

// Clears any entities like lines, circles, and resets flags
void ViewHelper::ClearCreateEntity(NativeViewerHandle* native)
{
    // Dismiss the overlay line (if it exists)
    if (!native->lineOverlay.IsNull())
    {
        native->context->Erase(native->lineOverlay, Standard_False);  // Remove the overlay
        native->lineOverlay.Nullify();  // Nullify the handle to clear it
    }

    // Dismiss the overlay circle (if it exists)
    if (!native->circleOverlay.IsNull())
    {
        native->context->Erase(native->circleOverlay, Standard_False);  // Remove the overlay
        native->circleOverlay.Nullify();  // Nullify the handle to clear it
    }

    if (!native->ellipseOverlay.IsNull())
    {
        native->context->Erase(native->ellipseOverlay, Standard_False);  // Remove the overlay
        native->ellipseOverlay.Nullify();  // Nullify the handle to clear it
    }

    if (!native->rectangleOverlay.IsNull())
    {
        native->context->Erase(native->rectangleOverlay, Standard_False);  // Remove the overlay
        native->rectangleOverlay.Nullify();  // Nullify the handle to clear it
    }

    // Reset any other relevant flags or data if necessary
    native->isDragging = false;  // Stop dragging
    native->dragEndX = native->dragStartX = 0;  // Reset drag points
    native->dragEndY = native->dragStartY = 0;  // Reset drag points

    native->isLineMode = false;
    native->isCircleMode = false;
    native->isRectangleMode = false;
    native->isEllipseMode = false;
    native->isTrimMode = false;
    native->isExtrudeMode = false;
    native->isRadiusMode = false;
    native->isEnCloseMode = false;
    native->isSelecting = false;
    native->isBooleanMode = false;
    native->isBooleanUnionMode = false;
    native->isBooleanCutMode = false;
    native->isBooleanIntersectMode = false;
    native->isMoveMode = false;
    native->isMoving = false;
    native->isRotateMode = false;
    native->isRotating = false;
    native->isMateAlignmentMode = false;
    native->firstMateFace = nullptr;

    native->isRevolveMode = false;
    native->isRevolveAxisSet = false;
    native->revolveAxis;
    native->isCenterLineSet = false;
    native->revolveAxisObj.Nullify();
    native->revolveAxisShape.Nullify();

    native->isExtrudingActive = false;
    native->extrudePreviewShape.Nullify();
    native->activeWire.Nullify();

    native->isDrawDimensionMode = false;
    native->isPickingDimensionFirstPoint = false;
    native->isPlacingDimension = false;
}

// Draw a center marker at a given point (x, y, z) in the 3D view
void ViewHelper::DrawCenterMarker(NativeViewerHandle* native, double x, double y, double z)
{
    auto context = native->context;
    if (context.IsNull()) return;

    // Remove existing marker if any
    if (!native->centerOverlay.IsNull())
    {
        context->Remove(native->centerOverlay, Standard_False);
        native->centerOverlay.Nullify();
    }

    // Instantiate the marker with 3D coordinates and display it
    Handle(AIS_InteractiveObject) marker = new Marker(x, y, z);
    native->centerOverlay = marker;
    context->Display(marker, Standard_False);  // Display without refreshing
    context->Redisplay(marker, Standard_True); // Redisplay to show the marker immediately
}

// Clears the center marker
void ViewHelper::ClearCenterMarker(NativeViewerHandle* native)
{
    if (native && !native->centerOverlay.IsNull())
    {
        // Remove the existing marker from the context
        native->context->Remove(native->centerOverlay, Standard_False);

        // Nullify the centerOverlay to avoid further references to the removed object
        native->centerOverlay.Nullify();

        // Redisplay the context to update the view
        native->view->Redraw();
    }
}