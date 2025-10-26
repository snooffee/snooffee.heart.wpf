#include "pch.h"
#include "NativeViewerHandle.h"
#include "Utils.h"
#include "ShapeDrawer.h"
#include <WNT_Window.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <map>
#include <mutex>
#include <TopoDS_Shape.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <iostream>
#include <BRepAlgoAPI_Section.hxx>
#include <TopExp_Explorer.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <ShapeFix_Wire.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

using namespace PotatoOCC;

// Thread-safe map for 3D boxes per viewer
static std::map<NativeViewerHandle*, Handle(AIS_Shape)> boxMap;
static std::mutex boxMapMutex;

// --------------------- 3D Box ---------------------
void ShapeDrawer::DrawBox(
    IntPtr viewerHandlePtr,
    double dx, double dy, double dz,
    int fr, int fg, int fb,              // face color
    double transparency,
    double edgeWidth,
    int er, int eg, int eb,             // edge color
    System::String^ edgeType)
{
    if (viewerHandlePtr == IntPtr::Zero) return;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    // -----------------------------
    // 1️⃣ Remove previous box if exists
    // -----------------------------
    if (!native->box3D.IsNull())
    {
        context->Remove(native->box3D, Standard_True);
        native->box3D.Nullify();
    }

    // -----------------------------
    // 2️⃣ Create new box shape
    // -----------------------------
    TopoDS_Shape boxShape = BRepPrimAPI_MakeBox(dx, dy, dz).Shape();
    native->box3D = new AIS_Shape(boxShape);

    // -----------------------------
    // 3️⃣ Set face color and transparency
    // -----------------------------
    native->box3D->SetColor(Quantity_Color(fr / 255.0, fg / 255.0, fb / 255.0, Quantity_TOC_RGB));
    native->box3D->SetTransparency(transparency);

    // -----------------------------
    // 4️⃣ Set edge color and width and Style
    // -----------------------------
    // --- Convert System::String^ to std::string ---
    using namespace Runtime::InteropServices;
    IntPtr ansiStr = Marshal::StringToHGlobalAnsi(edgeType);
    std::string edgeTypeStr = static_cast<const char*>(ansiStr.ToPointer());
    Marshal::FreeHGlobal(ansiStr);

    Aspect_TypeOfLine lineStyle = GetLineStyle(edgeTypeStr);


    Handle(Prs3d_LineAspect) edgeAspect = new Prs3d_LineAspect(
        Quantity_Color(er / 255.0, eg / 255.0, eb / 255.0, Quantity_TOC_RGB),
        lineStyle,
        edgeWidth
    );

    native->box3D->Attributes()->SetFaceBoundaryDraw(Standard_True);
    native->box3D->Attributes()->SetFaceBoundaryAspect(edgeAspect);


    // -----------------------------
    // 6️⃣ Display the box
    // -----------------------------
    context->Display(native->box3D, Standard_True);
    context->UpdateCurrentViewer();
    view->Redraw();

    // -----------------------------
    // 7️⃣ Keep track in map for thread safety
    // -----------------------------
    std::lock_guard<std::mutex> lock(boxMapMutex);
    boxMap[native] = native->box3D;
}

// ----------------------- Wireframe/Shaded -----------------------
void ShapeDrawer::SetWireframe(IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull()) return;

    Handle(AIS_Shape) box;
    { std::lock_guard<std::mutex> lock(boxMapMutex); auto it = boxMap.find(native); if (it == boxMap.end()) return; box = it->second; }

    native->context->SetDisplayMode(box, AIS_WireFrame, Standard_True);
    native->context->Update(box, Standard_True);

    native->context->Activate(box);  // Enable interaction/selection

    native->view->Redraw();
}

void ShapeDrawer::SetShaded(IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull()) return;

    Handle(AIS_Shape) box;
    { std::lock_guard<std::mutex> lock(boxMapMutex); auto it = boxMap.find(native); if (it == boxMap.end()) return; box = it->second; }

    native->context->SetMaterial(box, Graphic3d_NOM_PLASTIC, Standard_False);
    native->context->SetDisplayMode(box, AIS_Shaded, Standard_False);
    native->context->Update(box, Standard_False);

    native->context->Activate(box);  // Enable interaction/selection

    native->view->Redraw();
}

void ShapeDrawer::RotateBoxAroundCenter(IntPtr viewerHandlePtr, double angleX, double angleY)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->box3D.IsNull() || native->view.IsNull() || native->context.IsNull()) return;

    gp_Pnt center = GetShapeCenter(native->box3D->Shape());
    gp_Trsf trX; trX.SetRotation(gp_Ax1(center, gp_Dir(0, 1, 0)), angleX); native->box3D->SetLocalTransformation(trX);
    gp_Trsf trY; trY.SetRotation(gp_Ax1(center, gp_Dir(1, 0, 0)), angleY); native->box3D->SetLocalTransformation(trY);

    native->context->Update(native->box3D, Standard_True); native->view->Redraw();
}

void ShapeDrawer::ActivateFaceSelection(System::IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull()) return;

    // Activate face selection mode
    native->context->Deactivate(); // clear any previous modes
    native->context->Activate(native->box3D, AIS_Shape::SelectionMode(TopAbs_FACE));

    // Make sure shape is displayed
    if (!native->context->IsDisplayed(native->box3D))
    {
        native->context->Display(native->box3D, Standard_True);
    }

    // Redraw so the selection is visually enabled
    if (!native->view.IsNull())
        native->view->Redraw();
}
void ShapeDrawer::UpdateSelection(IntPtr viewerHandlePtr, int x, int y)
{
    UpdateSelection(viewerHandlePtr, x, y, false); // delegate to full method
}
void ShapeDrawer::UpdateSelection(System::IntPtr viewerHandlePtr, int x, int y, bool isClick)
{
    if (viewerHandlePtr == IntPtr::Zero) return;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    // Flag to track if a redraw is required
    bool redrawRequired = false;

    // -----------------------------
    // 1️⃣ Move cursor for detection/highlight
    // -----------------------------
    // Move cursor to the new position, without immediate redraw
    context->MoveTo(x, y, view, Standard_False);

    // -----------------------------
    // 2️⃣ Debug: check detected object
    // -----------------------------
    if (context->HasDetected())
    {
        Handle(AIS_InteractiveObject) detIo = context->DetectedInteractive();
        if (detIo.IsNull()) return;

        TopoDS_Shape detShape = context->DetectedShape();

        std::cout << "[PotatoOCC] UpdateSelection: HasDetected = true" << std::endl;
        std::cout << "[PotatoOCC]   DetectedInteractive is null? " << (detIo.IsNull() ? 1 : 0) << std::endl;
        if (!detShape.IsNull())
        {
            std::cout << "[PotatoOCC]   DetectedShape type = " << (int)detShape.ShapeType() << std::endl;
        }
        else
        {
            std::cout << "[PotatoOCC]   DetectedShape is null" << std::endl;
        }
    }
    else
    {
        std::cout << "[PotatoOCC] UpdateSelection: HasDetected = false" << std::endl;
    }

    // -----------------------------
    // 3️⃣ Commit selection if mouse click
    // -----------------------------
    if (isClick)
    {
        // Only select if it's a click, not on mouse move
        context->Select(Standard_True);
        redrawRequired = true;  // Mark that a redraw is needed
    }

    // -----------------------------
    // 4️⃣ Redraw the view (only if needed)
    // -----------------------------
    if (redrawRequired)
    {
        // Redraw the view only after selection or relevant changes
        view->Redraw();
    }
}


void ShapeDrawer::AlignViewToSelectedFace(System::IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    // First try selected list
    native->context->InitSelected();
    if (native->context->MoreSelected())
    {
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: using SelectedShape()" << std::endl;

        TopoDS_Shape selShape = native->context->SelectedShape();
        if (!selShape.IsNull() && selShape.ShapeType() == TopAbs_FACE)
        {
            TopoDS_Face face = TopoDS::Face(selShape);

            // compute normal
            double u1, u2, v1, v2;
            BRepTools::UVBounds(face, u1, u2, v1, v2);
            double um = 0.5 * (u1 + u2);
            double vm = 0.5 * (v1 + v2);

            BRepAdaptor_Surface surf(face);
            gp_Pnt c;
            gp_Vec dU, dV;
            surf.D1(um, vm, c, dU, dV);
            gp_Vec nvec = dU.Crossed(dV);
            if (nvec.Magnitude() > Precision::Confusion())
            {
                gp_Dir normal(nvec);
                native->view->SetProj(normal.X(), normal.Y(), normal.Z());
                native->view->FitAll();
                native->view->Redraw();
                std::cout << "[PotatoOCC] AlignViewToSelectedFace: aligned to selected face." << std::endl;
                return;
            }
        }
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: selected item not a face or normal compute failed." << std::endl;
    }
    else
    {
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: no selected items." << std::endl;
    }

    // Fallback: use the currently detected shape under cursor (if any)
    if (native->context->HasDetected())
    {
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: falling back to DetectedShape()" << std::endl;
        TopoDS_Shape detShape = native->context->DetectedShape();
        if (!detShape.IsNull() && detShape.ShapeType() == TopAbs_FACE)
        {
            TopoDS_Face face = TopoDS::Face(detShape);

            double u1, u2, v1, v2;
            BRepTools::UVBounds(face, u1, u2, v1, v2);
            double um = 0.5 * (u1 + u2);
            double vm = 0.5 * (v1 + v2);

            BRepAdaptor_Surface surf(face);
            gp_Pnt c;
            gp_Vec dU, dV;
            surf.D1(um, vm, c, dU, dV);
            gp_Vec nvec = dU.Crossed(dV);
            if (nvec.Magnitude() > Precision::Confusion())
            {
                gp_Dir normal(nvec);
                native->view->SetProj(normal.X(), normal.Y(), normal.Z());
                native->view->FitAll();
                native->view->Redraw();
                std::cout << "[PotatoOCC] AlignViewToSelectedFace: aligned to detected face." << std::endl;
                return;
            }
        }
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: detected item not a face or normal compute failed." << std::endl;
    }
    else
    {
        std::cout << "[PotatoOCC] AlignViewToSelectedFace: no detected shape to fallback on." << std::endl;
    }
}

// ----------------------- Clear -----------------------
void ShapeDrawer::ClearAllShapes(IntPtr viewerHandlePtr)
{
    if (viewerHandlePtr == IntPtr::Zero) return;
    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    for (auto& shape : native->ais2DShapes) if (!shape.IsNull()) native->context->Remove(shape, Standard_False);
    native->ais2DShapes.clear();

    for (auto& label : native->aisLabels) if (!label.IsNull()) native->context->Remove(label, Standard_False);
    native->aisLabels.clear();

    if (!native->box3D.IsNull()) native->context->Remove(native->box3D, Standard_False);

    native->context->EraseAll(Standard_True);
    native->context->UpdateCurrentViewer(); native->view->Redraw();

    std::lock_guard<std::mutex> lock(boxMapMutex);
    boxMap.erase(native);
}

// ----------------------- 2D Box with Label -----------------------
void ShapeDrawer::Draw2DBoxWithLabel(
    System::IntPtr viewerHandlePtr,
    double x, double y, double width, double height,
    int fr, int fg, int fb, double fillAlpha,
    double edgeWidth,
    int er, int eg, int eb, double edgeAlpha,
    System::String^ labelText,
    int lr, int lg, int lb, double textAlpha,
    double fontHeight,
    System::String^ fontFamily,
    System::String^ edgeType)
{
    if (viewerHandlePtr == IntPtr::Zero) return;

    NativeViewerHandle* native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull() || native->view.IsNull()) return;

    Handle(AIS_InteractiveContext) context = native->context;
    Handle(V3d_View) view = native->view;

    if (width <= 0 || height <= 0) return;

    // Clamp alpha manually
    fillAlpha = (fillAlpha < 0.0) ? 0.0 : (fillAlpha > 1.0) ? 1.0 : fillAlpha;
    edgeAlpha = (edgeAlpha < 0.0) ? 0.0 : (edgeAlpha > 1.0) ? 1.0 : edgeAlpha;
    textAlpha = (textAlpha < 0.0) ? 0.0 : (textAlpha > 1.0) ? 1.0 : textAlpha;

    // 1️⃣ Rectangle geometry
    gp_Pnt p1(x, y, 0), p2(x + width, y, 0),
        p3(x + width, y + height, 0), p4(x, y + height, 0);

    BRepBuilderAPI_MakePolygon poly;
    poly.Add(p1); poly.Add(p2); poly.Add(p3); poly.Add(p4);
    poly.Close();
    TopoDS_Wire wire = poly.Wire();
    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire);

    // 2️⃣ Filled face
    Handle(AIS_Shape) aisFace = new AIS_Shape(face);
    aisFace->SetColor(Quantity_Color(fr / 255.0, fg / 255.0, fb / 255.0, Quantity_TOC_RGB));
    aisFace->SetTransparency(fillAlpha);
    aisFace->SetDisplayMode(AIS_Shaded);
    aisFace->Attributes()->SetFaceBoundaryDraw(Standard_False);



    // Convert System::String^ to std::string
    using namespace Runtime::InteropServices;
    IntPtr ansiStr = Marshal::StringToHGlobalAnsi(edgeType);
    std::string edgeTypeStr = static_cast<const char*>(ansiStr.ToPointer());
    Marshal::FreeHGlobal(ansiStr);

    // Apply line style
    Aspect_TypeOfLine lineStyle = GetLineStyle(edgeTypeStr);


    // 3️⃣ Edge wire
    Handle(AIS_Shape) aisWire = new AIS_Shape(wire);
    aisWire->SetColor(Quantity_Color(er / 255.0, eg / 255.0, eb / 255.0, Quantity_TOC_RGB));
    aisWire->SetTransparency(edgeAlpha);
    aisWire->SetWidth(edgeWidth);
    aisWire->SetDisplayMode(AIS_WireFrame);

    // Apply edge type properly
    Handle(Prs3d_Drawer) drawer = aisWire->Attributes();
    Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(
        Quantity_Color(er / 255.0, eg / 255.0, eb / 255.0, Quantity_TOC_RGB),
        lineStyle,
        edgeWidth
    );
    drawer->SetWireAspect(lineAspect);

    // 4️⃣ Label
    double charFactor = 0.15;
    double textWidth = fontHeight * labelText->Length * charFactor;
    double textHeight = fontHeight;
    double verticalOffset = fontHeight * 0.3;

    gp_Pnt labelPos(
        x + width * 0.5 - textWidth * 0.5,
        y + height * 0.5 - textHeight * 0.5 + verticalOffset,
        0.01
    );

    Handle(AIS_TextLabel) textLabel = new AIS_TextLabel();
    textLabel->SetPosition(labelPos);

    pin_ptr<const wchar_t> wch = PtrToStringChars(labelText);
    textLabel->SetText(TCollection_ExtendedString(wch));
    textLabel->SetColor(Quantity_Color(lr / 255.0, lg / 255.0, lb / 255.0, Quantity_TOC_RGB));
    textLabel->SetTransparency(textAlpha);
    if (fontHeight > 0) textLabel->SetHeight(fontHeight);
    if (fontFamily != nullptr && fontFamily->Length > 0)
    {
        using namespace Runtime::InteropServices;
        IntPtr ansiStr = Marshal::StringToHGlobalAnsi(fontFamily);
        const char* fontCStr = static_cast<const char*>(ansiStr.ToPointer());
        textLabel->SetFont(fontCStr);
        Marshal::FreeHGlobal(ansiStr);
    }


    // 5️⃣ Display
    context->Display(aisFace, Standard_False);
    context->Display(aisWire, Standard_False);
    context->Display(textLabel, Standard_False);

    // Keep handles alive
    native->ais2DShapes.push_back(aisFace);
    native->ais2DShapes.push_back(aisWire);
    native->aisLabels.push_back(textLabel);

    context->UpdateCurrentViewer();
    view->Redraw();
}

void ShapeDrawer::ResetView(IntPtr viewerHandlePtr)
{
    NativeViewerHandle* native = reinterpret_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native) return;
    native->context->UpdateCurrentViewer();
    native->view->Redraw();
}

static double ComputeModelTextHeight(const Handle(V3d_View)& view, double pixelHeight)
{
    if (view.IsNull() || pixelHeight <= 0.0) return 1.0;

    // Get view scale (1.0 default; >1 zoom out)
    double scale = view->Scale();

    // We want the text height in model units so that it appears constant
    // in model space, like a line does. That means inversely scale by view->Scale().
    return pixelHeight / scale;
}


gp_Pnt ShapeDrawer::ScreenToWorld(Handle(V3d_View) view, double screenX, double screenY)
{
    Standard_Real X, Y, Z;
    view->Convert(screenX, screenY, X, Y, Z);
    return gp_Pnt(X, Y, 0);
}

TopoDS_Edge ShapeDrawer::CreateEdgeSafe(const gp_Pnt& p1, const gp_Pnt& p2)
{
    BRepBuilderAPI_MakeEdge edgeMaker(p1, p2);
    if (!edgeMaker.IsDone())
    {
        std::cerr << "Failed to create edge." << std::endl;
        return TopoDS_Edge();
    }
    return edgeMaker.Edge();
}

TopoDS_Edge ShapeDrawer::CreateCircleSafe(const gp_Circ& circle)
{
    BRepBuilderAPI_MakeEdge edgeMaker(circle);
    if (!edgeMaker.IsDone())
    {
        std::cerr << "Failed to create circle edge." << std::endl;
        return TopoDS_Edge();
    }
    return edgeMaker.Edge();
}

void ShapeDrawer::SetupOverlay(Handle(AIS_InteractiveObject)& obj, const Quantity_NameOfColor color, double width)
{
    obj->SetColor(color);
    obj->SetWidth(width);
}

void ShapeDrawer::eraseLineByDetectedIO(Handle(AIS_InteractiveObject) detectedIO, std::vector<Handle(AIS_Shape)>& persistedLines) {
    int indexToRemove = -1;

    // Find and erase the line with the matching detectedIO
    for (int i = 0; i < (int)persistedLines.size(); i++) {
        if (persistedLines[i].get() == detectedIO.get()) {
            indexToRemove = i;
            break;
        }
    }

    if (indexToRemove != -1) {
        persistedLines.erase(persistedLines.begin() + indexToRemove);
    }
}



bool ShapeDrawer::findIntersectionPoints(const gp_Pnt& intersectionPoint, const Handle(Geom_Curve)& curve,
    Standard_Real f, Standard_Real l, Handle(AIS_InteractiveObject) detectedIO,
    Handle(AIS_InteractiveContext) context, std::vector<Handle(AIS_Shape)>& persistedLines) {
    GeomAPI_ProjectPointOnCurve projector(intersectionPoint, curve);
    if (projector.NbPoints() == 0) {
        return false;
    }

    try {
        double param = projector.LowerDistanceParameter();
        const double tol = 1e-12;

        // Check if parameters are too close to avoid zero-length trimmed curve
        if (fabs(f - param) < tol) {
            return false;
        }

        Handle(Geom_TrimmedCurve) trimmedCurve = new Geom_TrimmedCurve(curve, f, param);
        TopoDS_Edge trimmedEdge = BRepBuilderAPI_MakeEdge(trimmedCurve);

        Handle(AIS_Shape) trimmedAIS = new AIS_Shape(trimmedEdge);
        trimmedAIS->SetDisplayMode(AIS_WireFrame);

        // Erase the original line
        ShapeDrawer::eraseLineByDetectedIO(detectedIO, persistedLines);


        // Update the context with the trimmed shape
        context->Remove(detectedIO, Standard_True);
        context->Display(trimmedAIS, Standard_True);

        return true;
    }
    catch (Standard_Failure& e) {
        std::cerr << "Error during trimming: " << e.GetMessageString() << std::endl;
        return false;
    }
}

// Function to check if the wire is closed
bool ShapeDrawer::IsClosedWire(const TopoDS_Wire& wire) {
    TopExp_Explorer exp(wire, TopAbs_VERTEX);

    if (exp.More()) {
        // Get the first vertex
        TopoDS_Vertex firstVertex = TopoDS::Vertex(exp.Current());
        gp_Pnt firstPoint = BRep_Tool::Pnt(firstVertex);

        exp.Next();

        // Get the last vertex
        TopoDS_Vertex lastVertex = TopoDS::Vertex(exp.Current());
        gp_Pnt lastPoint = BRep_Tool::Pnt(lastVertex);

        // Compare first and last vertices
        return firstPoint.IsEqual(lastPoint, 1e-6);  // Use a tolerance to compare the points
    }

    return false;
}


bool ShapeDrawer::checkAndTrimEdge(
    const TopoDS_Edge& selectedEdge,
    const TopoDS_Edge& otherEdge,
    const Handle(Geom_Curve)& curve,
    Standard_Real f, Standard_Real l,
    Handle(AIS_InteractiveObject) detectedIO,
    bool& trimmed,
    Handle(AIS_InteractiveContext) context,
    std::vector<Handle(AIS_Shape)>& persistedLines)
{
    int indexToRemove = -1;

    // Use an iterator-based approach to find and remove the detectedIO
    for (auto it = persistedLines.begin(); it != persistedLines.end(); ++it) {
        if (*it == detectedIO) {
            indexToRemove = std::distance(persistedLines.begin(), it); // Find the index of the detected IO
            break;
        }
    }

    if (indexToRemove != -1) {
        persistedLines.erase(persistedLines.begin() + indexToRemove);  // Erase using the iterator index
    }

    return true;
}

TopoDS_Wire ShapeDrawer::BuildWireFromSelection(const AIS_ListOfInteractive& picked)
{
    std::vector<TopoDS_Edge> edges;
    std::vector<gp_Pnt> startPts;
    std::vector<gp_Pnt> endPts;

    // Step 1: Extract valid edges from selected shapes
    for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next()) {
        Handle(AIS_InteractiveObject) obj = it.Value();
        if (obj.IsNull()) continue;

        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
        if (aisShape.IsNull()) continue;

        TopoDS_Shape shape = aisShape->Shape();
        if (shape.IsNull()) continue;

        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            TopoDS_Edge edge = TopoDS::Edge(exp.Current());
            if (edge.IsNull()) continue;

            edges.push_back(edge);

            TopoDS_Vertex v1, v2;
            TopExp::Vertices(edge, v1, v2);
            gp_Pnt p1 = BRep_Tool::Pnt(v1);
            gp_Pnt p2 = BRep_Tool::Pnt(v2);
            startPts.push_back(p1);
            endPts.push_back(p2);
        }
    }

    if (edges.size() < 3) {
        std::cout << "Not enough edges to form a closed wire." << std::endl;
        return TopoDS_Wire();
    }

    // Step 2: Order edges so they connect properly
    std::vector<TopoDS_Edge> ordered;
    std::vector<bool> used(edges.size(), false);
    ordered.push_back(edges[0]);
    used[0] = true;

    gp_Pnt currentEnd = endPts[0];
    Standard_Real tol = 1.0e-4;

    for (;;) {
        bool found = false;
        for (size_t i = 1; i < edges.size(); ++i) {
            if (used[i]) continue;

            gp_Pnt s = startPts[i];
            gp_Pnt e = endPts[i];

            if (currentEnd.Distance(s) < tol) {
                ordered.push_back(edges[i]);
                currentEnd = e;
                used[i] = true;
                found = true;
                break;
            }
            else if (currentEnd.Distance(e) < tol) {
                // reverse edge if needed
                BRepBuilderAPI_MakeEdge revEdge(BRep_Tool::Pnt(TopExp::LastVertex(edges[i])),
                    BRep_Tool::Pnt(TopExp::FirstVertex(edges[i])));
                ordered.push_back(revEdge.Edge());
                currentEnd = s;
                used[i] = true;
                found = true;
                break;
            }
        }
        if (!found) break;
    }

    if (ordered.size() != edges.size()) {
        std::cout << "Warning: some edges could not be connected (tolerance issue)." << std::endl;
    }

    // Step 3: Build wire from ordered edges
    BRepBuilderAPI_MakeWire wireBuilder;
    for (auto& e : ordered) {
        wireBuilder.Add(e);
    }

    if (!wireBuilder.IsDone()) {
        std::cout << "Wire construction failed. Attempting ShapeFix..." << std::endl;
        BRepBuilderAPI_MakeWire fallback;
        for (auto& e : edges) fallback.Add(e);
        if (!fallback.IsDone()) {
            std::cout << "Even ShapeFix failed." << std::endl;
            return TopoDS_Wire();
        }
        return fallback.Wire();
    }

    TopoDS_Wire rawWire = wireBuilder.Wire();

    // Step 4: Auto-fix wire for closure and tolerance issues
    ShapeFix_Wire fixWire;
    fixWire.Load(rawWire);
    fixWire.FixClosed();
    fixWire.FixConnected();
    fixWire.SetPrecision(1.0e-4);
    fixWire.Perform();

    TopoDS_Wire fixedWire = fixWire.Wire();

    //std::cout << "✅ Wire successfully built, ordered, and closed." << std::endl;
    return fixedWire;
}

Handle(AIS_InteractiveObject) ShapeDrawer::BuildCompoundFromSelection(const AIS_ListOfInteractive& picked)
{
    std::vector<TopoDS_Edge> edges;

    // Step 1: Extract valid edges from the selected AIS_InteractiveObjects
    for (AIS_ListOfInteractive::Iterator it(picked); it.More(); it.Next()) {
        Handle(AIS_InteractiveObject) obj = it.Value();
        if (obj.IsNull()) continue;

        // 🔽 Downcast
        Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
        if (aisShape.IsNull()) continue;

        TopoDS_Shape shape = aisShape->Shape();
        if (shape.IsNull()) continue;

        if (shape.ShapeType() == TopAbs_EDGE) {
            edges.push_back(TopoDS::Edge(shape));
        }
        else if (shape.ShapeType() == TopAbs_WIRE) {
            for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
                edges.push_back(TopoDS::Edge(exp.Current()));
            }
        }
        else if (shape.ShapeType() == TopAbs_COMPOUND) {
            for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
                edges.push_back(TopoDS::Edge(exp.Current()));
            }
        }
    }

    if (edges.size() < 1) {
        std::cout << "No edges selected." << std::endl;
        return Handle(AIS_InteractiveObject)();  // Null object
    }

    // Step 2: Create a compound to hold all selected edges
    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);

    for (const auto& edge : edges) {
        builder.Add(compound, edge);
    }

    // Step 3: Create an AIS_Shape to display the compound
    Handle(AIS_Shape) aisCompound = new AIS_Shape(compound);

    // Step 4: You can now add this AIS_InteractiveObject to your viewer or selection context
    // For example, in a document or viewer:
    // viewer->Add(aisCompound);

    std::cout << "✅ Compound created with " << edges.size() << " edges." << std::endl;

    return aisCompound;
}


void ShapeDrawer::HighlightSelectedObjects(Handle(AIS_InteractiveContext) context, AIS_ListOfInteractive& picked)
{
    Quantity_Color mySelectColor(Quantity_NOC_GREEN);
    Handle(Prs3d_Drawer) selStyle = context->HighlightStyle(Prs3d_TypeOfHighlight_Selected);
    selStyle->SetColor(mySelectColor);

    for (AIS_ListIteratorOfListOfInteractive it(picked); it.More(); it.Next()) {
        context->AddSelect(it.Value());
    }
}