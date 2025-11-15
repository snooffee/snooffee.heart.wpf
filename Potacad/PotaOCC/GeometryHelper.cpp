#include "pch.h"
#include "GeometryHelper.h"
#include "NativeViewerHandle.h"
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <GeomLProp_SLProps.hxx>
#include <iostream>
#include <cmath>
#include "ShapeDrawer.h"
#include "ShapeExtruder.h"
#include "ShapeRevolver.h"
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <AIS_Shape.hxx>
#include <AIS_InteractiveObject.hxx>
#include <TopExp_Explorer.hxx>
#include <thread>
#include <chrono>
#include "ViewHelper.h"
#include <BRepAdaptor_Curve.hxx>
#include "MouseCursor.h"
using namespace PotaOCC::ViewHelper;
using namespace PotaOCC::ViewHelper;
using namespace PotaOCC;
namespace PotaOCC {
    namespace GeometryHelper
    {
        inline double ClampValue(double val, double minVal, double maxVal)
        {
            return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val;
        }
        gp_Vec ComputeNormal(const TopoDS_Face& face)
        {
            BRepAdaptor_Surface surface(face, Standard_True);

            Standard_Real uMid = (surface.FirstUParameter() + surface.LastUParameter()) / 2.0;
            Standard_Real vMid = (surface.FirstVParameter() + surface.LastVParameter()) / 2.0;

            GeomLProp_SLProps props(surface.Surface().Surface(), uMid, vMid, 1, 1e-6);
            if (!props.IsNormalDefined())
            {
                std::cerr << "⚠️ Normal not defined for this face!" << std::endl;
                return gp_Vec(0, 0, 1);
            }

            gp_Dir normal = props.Normal();
            if (face.Orientation() == TopAbs_REVERSED)
                normal.Reverse();

            return gp_Vec(normal);
        }
        gp_Trsf ComputeFaceToFaceMate(const gp_Pnt& p1, const gp_Vec& n1, const gp_Pnt& p2, const gp_Vec& n2)
        {
            gp_Vec normal1 = n1.Normalized();
            gp_Vec normal2 = n2.Normalized();
            gp_Vec target = -normal2;

            gp_Vec axis = normal1.Crossed(target);
            double dot = normal1.Dot(target);

            gp_Trsf rotation;

            if (axis.Magnitude() < 1e-9)
            {
                // Parallel or anti-parallel
                if (dot < 0.9999)
                {
                    gp_Vec ref(1, 0, 0);
                    if (Abs(normal1.X()) > 0.9)
                        ref = gp_Vec(0, 1, 0);

                    axis = normal1.Crossed(ref);
                    axis.Normalize();

                    gp_Ax1 ax(p1, gp_Dir(axis));
                    rotation.SetRotation(ax, M_PI);
                }
                else
                {
                    rotation.SetRotation(gp_Ax1(p1, gp::DZ()), 0.0);
                }
            }
            else
            {
                axis.Normalize();
                double angle = acos(ClampValue(dot, -1.0, 1.0));
                gp_Ax1 ax(p1, gp_Dir(axis));
                rotation.SetRotation(ax, angle);
            }

            gp_Pnt p1_rotated = p1.Transformed(rotation);
            gp_Vec translationVec(p1_rotated, p2);

            gp_Trsf translation;
            translation.SetTranslation(translationVec);

            return translation.Multiplied(rotation);
        }
        void ApplyTransformationToShape(TopoDS_Shape& shape, const gp_Trsf& transform)
        {
            BRepBuilderAPI_Transform transformer(shape, transform, Standard_True);
            if (!transformer.IsDone())
            {
                std::cerr << "❌ Transformation failed!" << std::endl;
                return;
            }
            shape = transformer.Shape();
        }
        bool PerformRectangleSelection(NativeViewerHandle* native, const Handle(AIS_InteractiveContext)& context, const Handle(V3d_View)& view, int h, int w, int mouseY)
        {
            if (native == nullptr || context.IsNull() || view.IsNull())
                return false;

            // compute rectangle coords
            int xMin = std::min(native->dragStartX, native->dragEndX);
            int yMin = std::min(native->dragStartY, native->dragEndY);
            int xMax = std::max(native->dragStartX, native->dragEndX);
            int yMax = std::max(native->dragStartY, native->dragEndY);

            // Perform selection with AIS
            context->Select(xMin, yMin, xMax, yMax, view, Standard_False);

            // Collect selected AIS objects
            bool isAnyObjectSelected = false;
            AIS_ListOfInteractive picked;
            for (context->InitSelected(); context->MoreSelected(); context->NextSelected())
            {
                Handle(AIS_InteractiveObject) obj = context->SelectedInteractive();
                if (!obj.IsNull())
                {
                    picked.Append(obj);
                    isAnyObjectSelected = true;
                }
            }

            // Highlight selected objects (existing helper)
            ShapeDrawer::HighlightSelectedObjects(context, picked);

            // --- Extrude handling (same logic as original) ---
            if (isAnyObjectSelected && native->isExtrudeMode)
            {
                for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next())
                {
                    Handle(AIS_InteractiveObject) obj = it.Value();
                    Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
                    if (aisShape.IsNull()) continue;

                    TopoDS_Shape shape = aisShape->Shape();
                    if (shape.IsNull()) continue;

                    if (shape.ShapeType() == TopAbs_EDGE)
                    {
                        TopoDS_Edge detectedEdge = TopoDS::Edge(shape);
                        if (detectedEdge.IsNull())
                        {
                            std::cout << "Edge is invalid!" << std::endl;
                            return true; // processed selection (caller will reset)
                        }

                        TopoDS_Wire wire = ShapeDrawer::BuildWireFromSelection(picked);
                        if (wire.IsNull())
                        {
                            wire = BRepBuilderAPI_MakeWire(detectedEdge);
                        }

                        native->activeWire = wire;
                        native->isExtrudingActive = true;
                        native->currentExtrudeHeight = 0.0;
                        native->lastMouseY = mouseY;

                        // Compute centroid as base
                        Bnd_Box bbox;
                        BRepBndLib::Add(native->activeWire, bbox);
                        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

                        gp_Pnt centroid((xmin + xmax) / 2.0,
                            (ymin + ymax) / 2.0,
                            (zmin + zmax) / 2.0);

                        native->extrudeBasePoint = centroid;

                        // only handle first edge-case like original (others ignored)
                        break;
                    }
                    else if (shape.ShapeType() == TopAbs_FACE)
                    {
                        TopoDS_Face detectedFace = TopoDS::Face(shape);

                        // Extract the wire from the face
                        TopoDS_Wire wire;
                        for (TopExp_Explorer exp(detectedFace, TopAbs_WIRE); exp.More(); exp.Next())
                        {
                            wire = TopoDS::Wire(exp.Current());
                            break; // only handle first wire
                        }

                        native->activeWire = wire;
                        native->isExtrudingActive = true;
                        native->currentExtrudeHeight = 0.0;
                        native->lastMouseY = mouseY;

                        // Compute centroid as base
                        Bnd_Box bbox;
                        BRepBndLib::Add(native->activeWire, bbox);
                        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
                        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

                        gp_Pnt centroid((xmin + xmax) / 2.0,
                            (ymin + ymax) / 2.0,
                            (zmin + zmax) / 2.0);

                        native->extrudeBasePoint = centroid;

                        //ShapeExtruder::ExtrudeWireAndDisplay(context, wire);
                    }
                    else if (shape.ShapeType() == TopAbs_WIRE)
                    {
                        TopoDS_Wire detectedWire = TopoDS::Wire(shape);
                        ShapeExtruder::ExtrudeWireAndDisplay(context, detectedWire);
                        break;
                    }
                    else
                    {
                        std::cout << "Detected shape is not a closed wire!" << std::endl;
                    }
                }

                return true; // selection handled -> caller should ResetDragState() and ClearRubberBand()
            }

            // --- Revolve handling (use native->isRevolveMode & axis) ---
            if (isAnyObjectSelected && native->isRevolveMode && native->isRevolveAxisSet)
            {
                // Build profile wire from selection (same intent as original code)
                TopoDS_Wire revolveWire = ShapeDrawer::BuildWireFromSelection(picked);

                // fallback: single edge -> make wire
                if (revolveWire.IsNull())
                {
                    for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next())
                    {
                        Handle(AIS_InteractiveObject) obj = it.Value();
                        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
                        if (aisShape.IsNull()) continue;

                        TopoDS_Shape shape = aisShape->Shape();
                        if (shape.IsNull()) continue;

                        if (shape.ShapeType() == TopAbs_EDGE)
                        {
                            revolveWire = BRepBuilderAPI_MakeWire(TopoDS::Edge(shape));
                            break;
                        }
                    }
                }

                if (revolveWire.IsNull())
                {
                    std::cout << "⚠️ No valid wire found for revolve." << std::endl;
                    return true;
                }

                // Ensure face can be made from the wire before revolve
                BRepBuilderAPI_MakeFace mkFace(revolveWire);
                if (!mkFace.IsDone())
                {
                    std::cout << "⚠️ Unable to make face from wire for revolve." << std::endl;
                    return true;
                }

                // Perform revolve using the axis stored in native (first selected line)
                ShapeRevolver revolver(native);
                revolver.RevolveWireAndDisplayFinal(context, revolveWire, 360.0);

                // Store and reset revolve state as original code did
                native->revolveWire = revolveWire;
                native->isRevolveMode = false;
                native->isRevolveAxisSet = false;

                std::cout << "🌀 Revolve complete around first mouse-selected line." << std::endl;

                return true; // processed selection
            }

            // nothing to do
            return false;
        }
        void AnimateZoomWindow(NativeViewerHandle* native, Handle(V3d_View) view, int xMin, int yMin, int xMax, int yMax, int steps)
        {
            if (view.IsNull()) return;
            Standard_Real startScale = view->Camera()->Scale();
            Handle(Graphic3d_Camera) camTarget = new Graphic3d_Camera(*view->Camera());
            view->WindowFitAll(xMin, yMin, xMax, yMax);
            Standard_Real targetScale = view->Camera()->Scale();
            view->Camera()->SetScale(startScale);
            view->Redraw();
            for (int i = 1; i <= steps; ++i)
            {
                double t = (double)i / (double)steps;
                t = t * t * (3.0 - 2.0 * t);
                Standard_Real currentScale = startScale + (targetScale - startScale) * t;
                view->Camera()->SetScale(currentScale);
                view->Redraw();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            view->Camera()->SetScale(targetScale);
            view->Redraw();
        }
        void ApplyLocalTransformationToAISShape(Handle(AIS_Shape) aisShape, Handle(AIS_InteractiveContext) context)
        {
            if (aisShape.IsNull()) return;

            TopoDS_Shape originalShape = aisShape->Shape();
            gp_Trsf localTrsf = aisShape->LocalTransformation();

            if (localTrsf.Form() != gp_Identity)
            {
                TopoDS_Shape transformedShape = BRepBuilderAPI_Transform(originalShape, localTrsf, true).Shape();
                aisShape->Set(transformedShape);
                aisShape->SetLocalTransformation(gp_Trsf());
                context->Redisplay(aisShape, Standard_False);
            }
        }
        void ApplyLocalTransformationToShape(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context)
        {
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(native->rotatingObject);
            if (aisShape.IsNull())
                return;
            TopoDS_Shape originalShape = aisShape->Shape();
            gp_Trsf localTrsf = aisShape->LocalTransformation();
            TopoDS_Shape transformedShape = BRepBuilderAPI_Transform(originalShape, localTrsf, true).Shape();
            aisShape->Set(transformedShape);
            aisShape->SetLocalTransformation(gp_Trsf());
            context->Redisplay(aisShape, Standard_False);
        }
        Handle(AIS_Shape) CreateHighlightedFace(const TopoDS_Face& face)
        {
            if (face.IsNull())
                return Handle(AIS_Shape)(); // return null, extremely important

            Handle(AIS_Shape) aisFace = new AIS_Shape(face);

            // Always create a new Drawer to avoid nullptr
            Handle(Prs3d_Drawer) drawer = new Prs3d_Drawer();

            // Set shading (visible face color)
            drawer->SetShadingAspect(new Prs3d_ShadingAspect());
            drawer->ShadingAspect()->SetColor(Quantity_Color(Quantity_NOC_RED));

            // Semi-transparency
            drawer->SetTransparency(0.3);

            // Ensure shaded display
            drawer->SetDisplayMode(AIS_Shaded);

            // Explicitly assign attributes to AIS object
            aisFace->SetAttributes(drawer);

            return aisFace;
        }
        TopoDS_Shape GetSafeDetectedShape(const Handle(AIS_InteractiveContext)& context)
        {
            TopoDS_Shape empty;
            if (!context->HasDetected())
                return empty;

            Handle(SelectMgr_EntityOwner) owner = context->DetectedOwner();
            if (owner.IsNull())
                return empty;

            // Convert selectable object to AIS
            Handle(AIS_InteractiveObject) aisObj =
                Handle(AIS_InteractiveObject)::DownCast(owner->Selectable());
            if (aisObj.IsNull())
                return empty;

            // Only proceed if object is AIS_Shape
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(aisObj);
            if (aisShape.IsNull())
                return empty;

            // ✅ Safe: retrieve shape directly from AIS_Shape
            return aisShape->Shape();
        }
        void HandleFaceHover(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            // Activate face detection and move cursor
            context->Deactivate();
            context->Activate(TopAbs_FACE);
            context->MoveTo(x, y, view, Standard_True);

            // Get a safe detected shape (only if selectable is AIS_Shape)
            TopoDS_Shape shape = GetSafeDetectedShape(context);

            if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE)
            {
                // Nothing to highlight: remove previous highlight if any
                ClearHoverHighlightIfAny(native, context, view);
                return;
            }

            // Mouse on a face -> set cursor and display highlight
            MouseCursor::SetCustomCursor(native, PotaOCC::CursorType::Crosshair);

            TopoDS_Face face = TopoDS::Face(shape);
            ShowHoverHighlightForFace(native, face, context, view);
        }
        void HandleEdgeHover(NativeViewerHandle* native, int x, int y, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            // Activate generic shape detection and move cursor
            context->Deactivate();
            context->Activate(TopAbs_SHAPE);
            context->MoveTo(x, y, view, Standard_True);

            // Default cursor (may be changed below)
            MouseCursor::SetCustomCursor(native, PotaOCC::CursorType::Default);

            // Get safe shape only when selectable is AIS_Shape
            TopoDS_Shape shape = GetSafeDetectedShape(context);

            if (shape.IsNull())
            {
                ClearCenterMarker(native);
                return;
            }

            // Set crosshair if something detected
            MouseCursor::SetCustomCursor(native, PotaOCC::CursorType::Crosshair);

            // If not line-mode, nothing to do for edges
            if (!native->isLineMode)
            {
                ClearCenterMarker(native);
                return;
            }

            if (shape.ShapeType() != TopAbs_EDGE)
            {
                ClearCenterMarker(native);
                return;
            }

            TopoDS_Edge edge = TopoDS::Edge(shape);
            BRepAdaptor_Curve curveAdaptor(edge);

            // Only handle straight lines
            if (curveAdaptor.GetType() != GeomAbs_Line)
            {
                ClearCenterMarker(native);
                return;
            }

            // Compute center point of visible param range and draw marker
            Standard_Real u1 = curveAdaptor.FirstParameter();
            Standard_Real u2 = curveAdaptor.LastParameter();

            gp_Pnt p1 = curveAdaptor.Value(u1);
            gp_Pnt p2 = curveAdaptor.Value(u2);

            gp_Pnt mid((p1.X() + p2.X()) / 2.0,
                (p1.Y() + p2.Y()) / 2.0,
                (p1.Z() + p2.Z()) / 2.0);

            // Ensure any face highlight is removed (edge has priority only when not in mate mode)
            ClearHoverHighlightIfAny(native, context, view);

            DrawCenterMarker(native, mid.X(), mid.Y(), mid.Z());
        }
        void ClearHoverHighlightIfAny(NativeViewerHandle* native, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            if (!native->hoverHighlightedFace.IsNull())
            {
                // Use Erase (safer for temporary overlays)
                context->Erase(native->hoverHighlightedFace, Standard_False);
                native->hoverHighlightedFace.Nullify();
                if (!view.IsNull())
                    view->Redraw();
            }
        }
        void ShowHoverHighlightForFace(NativeViewerHandle* native, const TopoDS_Face& face, Handle(AIS_InteractiveContext) context, Handle(V3d_View) view)
        {
            if (face.IsNull())
                return;

            // Avoid recreating if same face already highlighted
            if (!native->hoverHighlightedFace.IsNull())
            {
                if (native->hoverHighlightedFace->Shape().IsSame(face))
                    return; // already highlighted
                // Different face -> clear previous
                context->Erase(native->hoverHighlightedFace, Standard_False);
                native->hoverHighlightedFace.Nullify();
            }

            // Create highlight
            Handle(AIS_Shape) newHighlight = CreateHighlightedFace(face);
            if (newHighlight.IsNull())
                return;

            native->hoverHighlightedFace = newHighlight;
            context->Display(native->hoverHighlightedFace, Standard_False);

            if (!view.IsNull())
                view->Redraw();
        }
        void HandleKeyMode(NativeViewerHandle* native, char key)
        {
            if (!native) return;
            ClearCreateEntity(native);
            switch (key)
            {
            case 'L': case 'l': native->isLineMode = true; break;
            case 'B': case 'b': native->isBooleanMode = true; break;
            case 'C': case 'c': native->isCircleMode = true; break;
            case 'D': case 'd': native->isBooleanCutMode = true; break;
            case 'E': case 'e': native->isEllipseMode = true; break;
            case 'F': case 'f': native->isRadiusMode = true; break;
            case 'I': case 'i': native->isBooleanIntersectMode = true; break;
            case 'W': case 'w': native->isRevolveMode = true; break;
            case 'M': case 'm': native->isMoveMode = true; break;
            case 'O': case 'o': native->isDrawDimensionMode = true; native->isPickingDimensionFirstPoint = true; break;
            case 'P': case 'p': native->isMateAlignmentMode = true; break;
            case 'R': case 'r': native->isRotateMode = true; break;
            case 'S': case 's': native->isExtrudeMode = true; break;
            case 'T': case 't': native->isTrimMode = true; break;
            case 'U': case 'u': native->isBooleanUnionMode = true; break;
            case 'Q': case 'q': native->isRectangleMode = true; break;
            case 'Z': case 'z': native->isZoomWindowMode = true; break;
            case 27: ClearCreateEntity(native); break;
            }
        }
    }
}