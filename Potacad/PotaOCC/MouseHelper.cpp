#include "pch.h"
#include "MouseHelper.h"
#include "GeometryHelper.h"
#include "LineDrawer.h"
#include "CircleDrawer.h"
#include "EllipseDrawer.h"
#include "ViewHelper.h"
#include "ShapeDrawer.h"
#include "ShapeExtruder.h"
#include "ShapeBooleanOperator.h"
#include "MateHelper.h"
#include "NativeViewerHandle.h"
#include "TextDrawer.h"
#include "DimensionHelper.h"
#include <TopoDS.hxx>
#include "MouseCursor.h"
#include "Print.h"
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBndLib.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include "RectangleDrawer.h"
#include <Geom_Plane.hxx>
#include <Geom_Surface.hxx>
#include <BRep_Tool.hxx>
#include <gp_Ax1.hxx>

using namespace PotaOCC;

using namespace GeometryHelper;

using namespace ViewHelper;

using namespace MateHelper;

namespace PotaOCC
{
    namespace MouseHelper
    {
        gp_Pnt Get3DPntOnPlane(const Handle(V3d_View)& view, const gp_Pnt& planeOrigin, const gp_Dir& planeNormal, int xPixel, int yPixel)
        {
            // Screen → eye ray
            Standard_Real xEye, yEye, zEye;
            Standard_Real xDir, yDir, zDir;

            view->Convert(xPixel, yPixel, xEye, yEye, zEye);
            view->Proj(xDir, yDir, zDir);

            gp_Pnt eye(xEye, yEye, zEye);
            gp_Dir dir(gp_Vec(xDir, yDir, zDir));
            gp_Vec l(eye, eye.Translated(dir));

            // Line-plane intersection
            gp_Vec n(planeNormal.X(), planeNormal.Y(), planeNormal.Z());
            gp_Vec eyeToPlane(planeOrigin, eye);

            double denom = n.Dot(l);
            if (fabs(denom) < Precision::Confusion()) return gp_Pnt(); // parallel

            double t = -n.Dot(eyeToPlane) / denom;
            return eye.Translated(l * t);
        }
        gp_Pnt Get3DPntFromScreen(Handle(V3d_View) view, int x, int y)
        {
            Standard_Real X, Y, Z;
            view->Convert(x, y, X, Y, Z);
            return gp_Pnt(X, Y, 0);
        }
        Handle(AIS_Shape) DrawLine(NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w)
        {
            return LineDrawer::DrawLine(view, viewerHandlePtr, native->dragStartX, native->dragStartY, native->dragEndX, native->dragEndY, h, w);
        }
        Handle(AIS_Shape) DrawCircle(NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            return new AIS_Shape(CircleDrawer::DrawCircle(native, view, viewerHandlePtr, native->dragStartX, native->dragStartY, native->dragEndX, native->dragEndY, h, w, x, y));
        }
        Handle(AIS_Shape) DrawRectangle(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            return RectangleDrawer::DrawRectangle(context, native, view);
        }
        Handle(AIS_Shape) DrawEllipse(NativeViewerHandle* native, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            return new AIS_Shape(EllipseDrawer::DrawEllipse(native, view, viewerHandlePtr, native->dragStartX, native->dragStartY, native->dragEndX, native->dragEndY, h, w, x, y));
        }
        void ResetDragState(NativeViewerHandle* native)
        {
            if (!native) return; // 🛡️ Safety check

            native->isDragging = false;
            native->dragEndX = 0;
        }
        void ClearRubberBand(NativeViewerHandle* native)
        {
            if (!native) return; // 🛡️ Safety check

            if (!native->rubberBandOverlay.IsNull()) {
                native->rubberBandOverlay->Erase();
                native->rubberBandOverlay.Nullify();
                native->rubberBandGroup.Nullify();
            }
        }
        void HandleMoveModeMouseDown(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
        {
            context->MoveTo(x, y, view, Standard_True);
            if (!context->HasDetected()) return;

            native->movingObject = context->DetectedInteractive();
            if (native->movingObject.IsNull()) return;

            Standard_Real X, Y, Z;
            view->Convert(x, y, X, Y, Z);
            native->lastMousePoint = gp_Pnt(X, Y, Z);
            native->isMoving = true;
        }
        void HandleRotateModeMouseDown(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
        {
            context->MoveTo(x, y, view, Standard_True);
            if (!context->HasDetected()) return;

            native->rotatingObject = context->DetectedInteractive();
            if (native->rotatingObject.IsNull()) return;

            native->lastMouseX = x;
            native->lastMouseY = y;
            native->isRotating = true;
        }
        void HandleDefaultMouseDown(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y, bool multipleselect, std::vector<Handle(AIS_InteractiveObject)>& lastHilightedObjects)
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
        void HandleMateModeMouseDown(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
        {
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

            TopoDS_Shape subShape = context->DetectedShape();
            if (subShape.IsNull())
            {
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

            if (!native->hasFirstMateSelected)
            {
                native->firstMateShape = detected;
                native->firstMateFace = new TopoDS_Face(clickedFace);
                native->hasFirstMateSelected = true;

                // Remove old highlight if exists
                if (!native->highlightedFace.IsNull())
                {
                    context->Remove(native->highlightedFace, Standard_False);
                    native->highlightedFace.Nullify();
                }

                // Create & show highlight
                native->highlightedFace = CreateHighlightedFace(clickedFace);
                context->Display(native->highlightedFace, Standard_False);

                view->Redraw();
                //std::cout << "✅ First mate face selected & highlighted." << std::endl;
                return;
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

                HandleFaceAlignment(context, view, native->firstMateShape, face1, detected, face2);

                delete native->firstMateFace;
                native->firstMateFace = nullptr;
                native->hasFirstMateSelected = false;

                // Remove highlight
                if (!native->highlightedFace.IsNull())
                {
                    context->Remove(native->highlightedFace, Standard_False);
                    native->highlightedFace.Nullify();
                }
            }

            view->Redraw();
        }
        void HandleLineMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            Handle(AIS_Shape) aisLine = DrawLine(native, view, viewerHandlePtr, h, w);
            context->Display(aisLine, Standard_True);
            native->persistedLines.push_back(aisLine);
            native->dragStartX = x;
            native->dragStartY = y;
        }
        void HandleCircleMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            Handle(AIS_Shape) aisCircle = DrawCircle(native, view, viewerHandlePtr, h, w, x, y);
            aisCircle->SetColor(Quantity_NOC_BLACK);  // Or any color
            aisCircle->SetWidth(2.0);                 // Line thickness
            context->Display(aisCircle, Standard_True);
            native->persistedCircles.push_back(aisCircle);
            ClearCreateEntity(native);
        }
        void HandleEllipseMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            Handle(AIS_Shape) aisEllipse = DrawEllipse(native, view, viewerHandlePtr, h, w, x, y);
            context->Display(aisEllipse, Standard_True); // Show the ellipse
            native->persistedEllipses.push_back(aisEllipse); // Save the ellipse for later use
            ClearCreateEntity(native); // Reset state if needed (based on your existing methods)
        }
        void HandleRectangleMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, IntPtr viewerHandlePtr, int h, int w, int x, int y)
        {
            Handle(AIS_Shape) aisRect = DrawRectangle(native, context, view);
            if (aisRect.IsNull())
                return;

            context->Display(aisRect, Standard_True);
            native->persistedRectangles.push_back(aisRect);

            ClearCreateEntity(native);
        }
        void HandleTrimMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context)
        {
        }
        void HandleRadiusMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context)
        {
        }
        void HandleExtrudingMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            native->isExtrudingActive = false;
            if (!native->extrudePreviewShape.IsNull())
                context->Remove(native->extrudePreviewShape, Standard_False);

            double finalHeight = native->currentExtrudeHeight;
            ShapeExtruder extruder(native);
            extruder.ExtrudeWireAndDisplayFinal(context, finalHeight);

            view->Redraw();
        }
        void HandleBooleanMode(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
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

            // 🧩 1️⃣ Bake any existing local transforms into geometry
            ApplyLocalTransformationToAISShape(aisShape1, context);
            ApplyLocalTransformationToAISShape(aisShape2, context);

            TopoDS_Shape shape1 = aisShape1->Shape();
            TopoDS_Shape shape2 = aisShape2->Shape();
            if (shape1.IsNull() || shape2.IsNull()) return;

            bool success;
            TopoDS_Shape resultShape = ShapeBooleanOperator::PerformBooleanOperation(shape1, shape2, native, context, view, success);

            if (native->isBooleanCutMode)
                ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape, "BCut operation succeeded.", "BCut operation failed.", success);
            else if (native->isBooleanUnionMode)
                ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape, "BUnion operation succeeded.", "BUnion operation failed.", success);
            else if (native->isBooleanIntersectMode)
                ShapeBooleanOperator::FinalizeBooleanResult(native, context, view, resultShape, "BIntersect operation succeeded.", "BIntersect operation failed.", success);
        }
        void HandleObjectDetection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
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
        void DrawLineOverlay(NativeViewerHandle* native, int height)
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
        void DrawCircleOverlay(NativeViewerHandle* native, Handle(V3d_View) view, int height, int mouseX, int mouseY)
        {
            if (native->circleOverlay.IsNull())
                native->circleOverlay = new AIS_OverlayCircle();

            if (native->isPlaneMode) {
                native->circleOverlay->SetTransformPersistence(nullptr);

                // Step 1: Get 3D point on the plane under current mouse
                gp_Pnt cursor3D = MouseHelper::Get3DPntOnPlane(
                    view,
                    native->planeOrigin,
                    native->planeNormal,
                    mouseX,
                    mouseY
                );

                // Step 2: Compute radius from drag start point
                gp_Vec radiusVec(native->dragStartPoint, cursor3D);
                double radius = radiusVec.Magnitude();

                // Step 3: Set the circle overlay on the correct plane
                gp_Ax2 circleAxis(native->dragStartPoint, native->planeNormal);
                native->circleOverlay->SetCircle(circleAxis, radius);
            }
            else {
                if (!native->circleOverlay->TransformPersistence())
                {
                    Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
                    native->circleOverlay->SetTransformPersistence(trpers);
                }
                double dx = native->dragEndX - native->dragStartX;
                double dy = native->dragEndY - native->dragStartY;
                double radius = std::sqrt(dx * dx + dy * dy);

                int centerYFlipped = height - native->dragStartY;

                native->circleOverlay->SetCircle(
                    gp_Pnt(native->dragStartX, centerYFlipped, 0.0),
                    radius);
            }
            native->context->Display(native->circleOverlay, Standard_True);
        }
        void DrawEllipseOverlay(NativeViewerHandle* native, Handle(V3d_View) view, int height, int mouseX, int mouseY)
        {
            if (native->ellipseOverlay.IsNull())
                native->ellipseOverlay = new AIS_OverlayEllipse();

            if (native->isPlaneMode)
            {
                // --- Plane-based ellipse drawing ---
                gp_Pnt cursor3D = MouseHelper::Get3DPntOnPlane(
                    view,
                    native->planeOrigin,
                    native->planeNormal,
                    mouseX,
                    mouseY
                );

                gp_Pnt startPnt = native->dragStartPoint;

                gp_Ax2 planeAxis(native->planeOrigin, native->planeNormal);
                gp_Dir uDir = planeAxis.XDirection();
                gp_Dir vDir = planeAxis.YDirection();

                gp_Vec dragVec(startPnt, cursor3D);
                double u = dragVec.Dot(gp_Vec(uDir));
                double v = dragVec.Dot(gp_Vec(vDir));

                double radiusX = std::abs(u) * 0.5;
                double radiusY = std::abs(v) * 0.5;

                gp_Pnt center = startPnt.Translated(
                    gp_Vec(uDir) * (u * 0.5) + gp_Vec(vDir) * (v * 0.5)
                );

                native->ellipseOverlay->SetEllipseOnPlane(center, uDir, vDir, radiusX, radiusY);
            }
            else
            {
                // --- 2D screen fallback ---
                double dx = mouseX - native->dragStartX;
                double dy = mouseY - native->dragStartY;
                double radiusX = std::abs(dx) * 0.5;
                double radiusY = std::abs(dy) * 0.5;

                int centerX = native->dragStartX + dx * 0.5;
                int centerYFlipped = height - (native->dragStartY + dy * 0.5);

                native->ellipseOverlay->SetEllipse(
                    gp_Pnt(centerX, centerYFlipped, 0.0),
                    radiusX,
                    radiusY
                );
            }

            native->context->Display(native->ellipseOverlay, Standard_True);
        }
        void DrawRectangleOverlay(NativeViewerHandle* native, Handle(V3d_View) view, int height, int mouseX, int mouseY)
        {
            if (native->rectangleOverlay.IsNull())
                native->rectangleOverlay = new AIS_OverlayRectangle();

            if (native->isPlaneMode)
            {
                native->rectangleOverlay->SetTransformPersistence(nullptr);

                // 3D point on plane
                gp_Pnt cursor3D = MouseHelper::Get3DPntOnPlane(
                    view,
                    native->planeOrigin,
                    native->planeNormal,
                    mouseX,
                    mouseY
                );

                gp_Pnt endPnt = cursor3D; // <-- use local variable

                gp_Pnt startPnt = native->dragStartPoint;
                gp_Ax2 planeAxis(native->planeOrigin, native->planeNormal);
                gp_Dir uDir = planeAxis.XDirection();
                gp_Dir vDir = planeAxis.YDirection();

                gp_Vec dragVec(startPnt, cursor3D);
                double u = dragVec.Dot(gp_Vec(uDir));
                double v = dragVec.Dot(gp_Vec(vDir));

                gp_Pnt p1 = startPnt;
                gp_Pnt p2 = startPnt.Translated(gp_Vec(uDir) * u);
                gp_Pnt p3 = startPnt.Translated(gp_Vec(uDir) * u + gp_Vec(vDir) * v);
                gp_Pnt p4 = startPnt.Translated(gp_Vec(vDir) * v);

                native->rectangleOverlay->SetRectangle(p1, p2, p3, p4);
            }
            else
            {
                // 2D screen-space fallback
                int startYFlipped = height - native->dragStartY;
                int endYFlipped = height - native->dragEndY;

                gp_Pnt p1(native->dragStartX, startYFlipped, 0.0);
                gp_Pnt p2(native->dragEndX, endYFlipped, 0.0);

                native->rectangleOverlay->SetRectangle(p1, p2);
            }

            native->context->Display(native->rectangleOverlay, Standard_True);
        }
        void DrawDimensionOverlay(NativeViewerHandle* native, Handle(V3d_View) view, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& cursor, bool isFinal)
        {
            if (!native || native->context.IsNull() || view.IsNull()) return;
            Handle(AIS_InteractiveContext) context = native->context;

            DimensionHelper::ClearPreviousOverlays(native, context, isFinal);

            gp_Vec edgeVec(p1, p2);
            if (edgeVec.Magnitude() < 1e-6) return; // avoid zero-length edge
            edgeVec.Normalize();

            gp_Vec perpVec(-edgeVec.Y(), edgeVec.X(), 0.0);
            perpVec.Normalize();

            gp_Vec cursorVec(p1, cursor);
            double offset = cursorVec.Dot(perpVec);

            gp_Pnt dimLineP1 = p1.Translated(perpVec * offset);
            gp_Pnt dimLineP2 = p2.Translated(perpVec * offset);

            DimensionHelper::DrawExtensionLine(native, context, p1, dimLineP1, isFinal);
            DimensionHelper::DrawExtensionLine(native, context, p2, dimLineP2, isFinal);
            DimensionHelper::DrawDimensionLine(native, context, dimLineP1, dimLineP2, isFinal);
            DimensionHelper::DrawArrowheads(native, context, dimLineP1, dimLineP2, edgeVec, isFinal);
            DimensionHelper::DrawLabel(native, context, dimLineP1, dimLineP2, p1, p2, edgeVec, isFinal);

            view->Redraw();
        }
        void DrawSelectionRectangle(NativeViewerHandle* native)
        {
            int xMin = std::min(native->dragStartX, native->dragEndX);
            int yMin = std::min(native->dragStartY, native->dragEndY);
            int xMax = std::max(native->dragStartX, native->dragEndX);
            int yMax = std::max(native->dragStartY, native->dragEndY);

            Quantity_Color red(1.0, 0.0, 0.0, Quantity_TOC_RGB);
            MouseCursor::DrawRubberBandRectangle(native, xMin, yMin, xMax, yMax);
        }
        void HighlightHoveredShape(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
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
                    MouseCursor::SetCustomCursor(native, PotaOCC::CursorType::Crosshair);
                }
            }
            else
            {
                context->Deactivate();
                context->Activate(TopAbs_SHAPE);
                MouseCursor::SetCustomCursor(native, PotaOCC::CursorType::Default);
            }
        }
        void HandleMove(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
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

            // ✅ Use LocalTransformation instead of replacing the shape
            /*gp_Trsf currentTrsf = aisShape->LocalTransformation();
            gp_Trsf moveTrsf;
            moveTrsf.SetTranslation(moveVec);
            gp_Trsf newTrsf = currentTrsf.Multiplied(moveTrsf);
            aisShape->SetLocalTransformation(newTrsf);*/

            context->MoveTo(x, y, view, Standard_True);
            context->Redisplay(aisShape, Standard_False);
            view->Redraw();

            native->lastMousePoint = currentPoint;
        }
        void HandleRotate(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
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
            gp_Trsf currentTrsf = aisShape->LocalTransformation();
            gp_Trsf newTrsf = currentTrsf.Multiplied(combinedTrsf);

            TopoDS_Shape oldShape = aisShape->Shape();
            BRepBuilderAPI_Transform trsfBuilder(oldShape, newTrsf, true);
            TopoDS_Shape newShape = trsfBuilder.Shape();
            aisShape->Set(newShape);
            aisShape->SetLocalTransformation(gp_Trsf());

            context->MoveTo(x, y, view, Standard_True);
            context->Redisplay(aisShape, Standard_False);
            view->Redraw();

            native->lastMouseX = x;
            native->lastMouseY = y;
        }
        void HandleExtrude(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int y)
        {
            int dy = y - native->lastMouseY;

            // Determine extrusion height based on plane type
            // World-aligned planes (XY, XZ, YZ)
            auto isWorldAlignedPlane = [](const gp_Dir& n)
                {
                    double dx = Abs(n.Dot(gp::DX()));
                    double dy = Abs(n.Dot(gp::DY()));
                    double dz = Abs(n.Dot(gp::DZ()));
                    return (dx > 0.99 || dy > 0.99 || dz > 0.99);
                };

            if (isWorldAlignedPlane(native->planeNormal))
            {
                // Simple up/down extrude for world planes
                native->currentExtrudeHeight = -dy * 1.0;
            }
            else
            {
                // Arbitrary face: extrusion follows mouse Y direction
                native->currentExtrudeHeight = dy * 1.0;
            }

            // Stop if no movement
            if (native->currentExtrudeHeight == 0)
            {
                view->Redraw();
                return;
            }

            // Remove previous preview
            if (!native->extrudePreviewShape.IsNull())
                context->Remove(native->extrudePreviewShape, Standard_False);

            // Rebuild preview
            ShapeExtruder extruder(native);
            extruder.ExtrudeWireAndDisplayPreview(context);

            view->Redraw();
        }
        void HandleZoomWindow(NativeViewerHandle* native, Handle(V3d_View) view)
        {
            if (!native || view.IsNull()) return;

            int x1 = native->dragStartX;
            int y1 = native->dragStartY;
            int x2 = native->dragEndX;
            int y2 = native->dragEndY;
            if (std::abs(x2 - x1) > 5 && std::abs(y2 - y1) > 5)
            {
                int xMin = std::min(x1, x2);
                int yMin = std::min(y1, y2);
                int xMax = std::max(x1, x2);
                int yMax = std::max(y1, y2);

                // Auto-center or add zoom margin
                const int margin = 10;
                xMin = std::max(0, xMin - margin);
                yMin = std::max(0, yMin - margin);
                xMax += margin;
                yMax += margin;

                // Perform zoom window
                //view->WindowFitAll(xMin, yMin, xMax, yMax);
                AnimateZoomWindow(native, view, xMin, yMin, xMax, yMax, 30);

                // First clear overlays before redrawing
                MouseHelper::ClearRubberBand(native);

                // Redraw view after clearing overlays
                view->Redraw();

                // Finally reset drag state
                MouseHelper::ResetDragState(native);
            }
        }
        void UpdateHoverDetection(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            if (native == nullptr || context.IsNull() || view.IsNull())
                return;

            if (native->isMateAlignmentMode)
            {
                HandleFaceHover(native, x, y, context, view);
            }
            else
            {
                HandleEdgeHover(native, x, y, context, view);
            }
        }
        void PrepareFaceDetection(Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
        {
            context->Deactivate();
            context->Activate(TopAbs_FACE); // Only detect faces
            context->MoveTo(x, y, view, Standard_True);
        }
        TopoDS_Shape GetDetectedShapeOrOwner(Handle(AIS_InteractiveContext) context)
        {
            TopoDS_Shape subShape = context->DetectedShape();
            if (subShape.IsNull())
            {
                Handle(SelectMgr_EntityOwner) owner = context->DetectedOwner();
                if (!owner.IsNull() && owner->HasSelectable())
                {
                    subShape = Handle(AIS_Shape)::DownCast(owner->Selectable())->Shape();
                }
            }
            return subShape;
        }
        void SetupPlaneForFace(NativeViewerHandle* native, Handle(V3d_View) view, const TopoDS_Face& face, int x, int y)
        {
            Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
            Handle(Geom_Plane) plane = Handle(Geom_Plane)::DownCast(surf);

            if (plane.IsNull())
            {
                std::cout << "⚠️ Selected face is not planar!" << std::endl;
                return;
            }
            native->planeOrigin = plane->Location();
            native->planeNormal = plane->Axis().Direction();
            native->dragStartPoint = MouseHelper::Get3DPntOnPlane(view, native->planeOrigin, native->planeNormal, x, y);
        }
        void HandleDetectedShape(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int x, int y)
        {
            Handle(AIS_Shape) detected = Handle(AIS_Shape)::DownCast(context->DetectedInteractive());
            if (detected.IsNull())
            {
                std::cout << "⚠️ No valid shape detected!" << std::endl;
                return;
            }

            TopoDS_Shape subShape = GetDetectedShapeOrOwner(context);
            if (subShape.IsNull() || subShape.ShapeType() != TopAbs_FACE)
            {
                std::cout << "⚠️ No face detected (maybe edge or vertex selected)!" << std::endl;
                return;
            }

            TopoDS_Face clickedFace = TopoDS::Face(subShape);
            SetupPlaneForFace(native, view, clickedFace, x, y);
        }
        void HandleShapeSelection(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view, int mouseX, int mouseY)
        {
            PrepareFaceDetection(context, view, mouseX, mouseY);

            if (!context->HasDetected())
            {
                native->isPlaneMode = false;
            }
            else
            {
                native->isPlaneMode = true;
                HandleDetectedShape(native, context, view, mouseX, mouseY);
            }

            native->dragStartX = mouseX;
            native->dragStartY = mouseY;
            native->isDragging = true;
        }
    }
}