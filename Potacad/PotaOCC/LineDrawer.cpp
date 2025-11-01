#include "pch.h"
#include "NativeViewerHandle.h"
#include "LineDrawer.h"
#include "ShapeDrawer.h"
#include "ViewHelper.h"
#include <AIS_InteractiveContext.hxx>
#include <GC_MakeSegment.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <AIS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <Font_BRepTextBuilder.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>

using namespace PotaOCC;

Handle(AIS_Shape) LineDrawer::DrawLineWithoutSnapping(Handle(V3d_View) view, System::IntPtr viewerHandlePtr, double dragStartX, double dragStartY, double dragEndX, double dragEndY, int h, int w)
{
    if (viewerHandlePtr == System::IntPtr::Zero)
        return nullptr;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull())
        return nullptr;

    Handle(AIS_InteractiveContext) ctx = native->context;

    // Convert screen to world coordinates
    gp_Pnt p1 = ShapeDrawer::ScreenToWorld(view, dragStartX, dragStartY);
    gp_Pnt p2 = ShapeDrawer::ScreenToWorld(view, dragEndX, dragEndY);

    if (p1.IsEqual(p2, 1e-9))
        return nullptr;

    // Create edge
    TopoDS_Edge edge = ShapeDrawer::CreateEdgeSafe(p1, p2);
    if (edge.IsNull())
        return nullptr;

    // Wrap in AIS_Shape
    Handle(AIS_Shape) aisLine = new AIS_Shape(edge);
    aisLine->SetDisplayMode(AIS_WireFrame);

    // Setup appearance
    Quantity_Color color(Quantity_NOC_WHITE);
    Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(color, Aspect_TOL_SOLID, 2.0);
    Handle(Prs3d_Drawer) drawer = aisLine->Attributes();
    drawer->SetWireAspect(lineAspect);
    aisLine->SetAttributes(drawer);

    // Display in viewer
    ctx->Display(aisLine, Standard_False);
    ctx->SetColor(aisLine, color, Standard_False);
    ctx->Redisplay(aisLine, Standard_False);
    ctx->UpdateCurrentViewer();

    // Persist
    if (!aisLine.IsNull())  // -> to prevent NullReferenceException during trimming.
        native->persistedLines.push_back(aisLine);

    return aisLine;
}

Handle(AIS_Shape) LineDrawer::DrawLine(
    Handle(V3d_View) view,
    System::IntPtr viewerHandlePtr,
    double dragStartX, double dragStartY,
    double dragEndX, double dragEndY,
    int h, int w)
{
    if (viewerHandlePtr == System::IntPtr::Zero)
        return nullptr;

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());
    if (!native || native->context.IsNull())
        return nullptr;

    Handle(AIS_InteractiveContext) ctx = native->context;

    // Convert screen to world coordinates
    gp_Pnt p1 = ShapeDrawer::ScreenToWorld(view, dragStartX, dragStartY);
    gp_Pnt p2 = ShapeDrawer::ScreenToWorld(view, dragEndX, dragEndY);

    if (p1.IsEqual(p2, 1e-9))
        return nullptr;

    // ========= SNAP CONFIG =========
    double snapTol = 10.0;  // tolerance (in model units)
    gp_Pnt firstStartPnt;
    bool foundLoopStart = false;
    double sX = 0, sY = 0;
    Standard_Integer sx2 = 0, sy2 = 0;
    // ========= CAPTURE LOOP START POINT (FIRST DRAWN LINE START) =========
    if (!native->persistedLines.empty()) {
        TopoDS_Shape firstShape = native->persistedLines.front()->Shape();
        if (!firstShape.IsNull() && firstShape.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge firstEdge = TopoDS::Edge(firstShape);
            firstStartPnt = BRep_Tool::Pnt(TopExp::FirstVertex(firstEdge));
            sX = dragStartX;
            sY = dragStartY;

            view->Convert(sX, sY, 0, sx2, sy2);
            foundLoopStart = true;
        }
    }

    bool snappedToStart = false;
    // ========= SNAP TO EXISTING ENDPOINTS =========
    for (auto& existingLine : native->persistedLines) {
        if (existingLine.IsNull()) continue;

        TopoDS_Shape s = existingLine->Shape();
        if (s.IsNull() || s.ShapeType() != TopAbs_EDGE) continue;

        TopoDS_Edge e = TopoDS::Edge(s);
        gp_Pnt eStart = BRep_Tool::Pnt(TopExp::FirstVertex(e));
        gp_Pnt eEnd = BRep_Tool::Pnt(TopExp::LastVertex(e));

        if (p1.Distance(eStart) <= snapTol) p1 = eStart;
        else if (p1.Distance(eEnd) <= snapTol) p1 = eEnd;

        /*if (p2.Distance(eStart) <= snapTol) p2 = eStart;
        else if (p2.Distance(eEnd) <= snapTol) p2 = eEnd;*/
        if (p2.Distance(eStart) <= snapTol) {
            p2 = eStart;
            // ✅ trigger when snap happens to loop start
            //if (eStart.IsEqual(firstStart, snapTol)) {
            snappedToStart = true;
            //}
        }
        else if (p2.Distance(eEnd) <= snapTol) {
            p2 = eEnd;
            // ✅ trigger when snap happens to loop start
            //if (eEnd.IsEqual(firstStart, snapTol)) {
            snappedToStart = true;
            //}
        }

    }

    // ========= TRIGGER END IF SNAPPED =========
    if (snappedToStart) {
        //std::cout << "[Snap] End point reached loop start — closing polyline." << std::endl;

        // ✅ Build a wire from only the current loop edges
        BRepBuilderAPI_MakeWire wireBuilder;
        std::vector<TopoDS_Edge> orderedEdges;

        gp_Pnt lastEnd = firstStartPnt; // Start with the first point
        const double tolerance = 1e-6; // Small tolerance for endpoint matching

        // Try to order edges based on proximity of the last endpoint
        for (auto& lineShape : native->currentLoopEdges) {
            if (lineShape.IsNull()) continue;

            TopoDS_Shape shape = lineShape->Shape();
            if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) continue;

            TopoDS_Edge edge = TopoDS::Edge(shape);
            gp_Pnt p1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
            gp_Pnt p2 = BRep_Tool::Pnt(TopExp::LastVertex(edge));

            bool added = false;

            // Debug: Print each edge's start and end
            /*std::cout << "[Debug] Checking edge: (" << p1.X() << "," << p1.Y() << ") -> ("
                << p2.X() << "," << p2.Y() << ")\n";*/

                // Try to match the start of the edge to the last end point
            if (p1.IsEqual(lastEnd, tolerance)) {
                orderedEdges.push_back(edge);
                lastEnd = p2;  // Update last endpoint to current edge's end
                added = true;
                /*std::cout << "[Debug] Added edge (start to end): (" << p1.X() << "," << p1.Y() << ") -> ("
                    << p2.X() << "," << p2.Y() << ")\n";*/
            }
            // Try to match the end of the edge to the last end point (reversed order)
            else if (p2.IsEqual(lastEnd, tolerance)) {
                orderedEdges.push_back(edge);
                lastEnd = p1;  // Update last endpoint to current edge's start
                added = true;
                /*std::cout << "[Debug] Added edge (end to start): (" << p2.X() << "," << p2.Y() << ") -> ("
                    << p1.X() << "," << p1.Y() << ")\n";*/
            }

            // If not added, it means the edge is disconnected, so skip it
            if (!added) {
                /*std::cout << "[Wire] Skipping edge: ("
                    << p1.X() << "," << p1.Y() << ") -> ("
                    << p2.X() << "," << p2.Y() << ") — not connected.\n";*/
            }
        }

        // Debug: Check the final ordered edges and the last connection
        if (!orderedEdges.empty()) {
            TopoDS_Edge lastEdge = orderedEdges.back();
            gp_Pnt lastEndPoint = BRep_Tool::Pnt(TopExp::LastVertex(lastEdge));
            //std::cout << "[Debug] Last edge connects to: (" << lastEndPoint.X() << "," << lastEndPoint.Y() << ")\n";
        }

        // Check if the last edge connects to the start point
        if (!orderedEdges.empty()) {
            TopoDS_Edge lastEdge = orderedEdges.back();
            gp_Pnt lastEndPoint = BRep_Tool::Pnt(TopExp::LastVertex(lastEdge));

            // Debug: Print the first start point and last end point
            /*std::cout << "[Debug] Checking closure: last end point ("
                << lastEndPoint.X() << "," << lastEndPoint.Y() << ") vs. first start point ("
                << firstStartPnt.X() << "," << firstStartPnt.Y() << ")\n";*/

            if (!lastEndPoint.IsEqual(firstStartPnt, tolerance)) {
                // Force closure by adding a final edge to match the start point
                //std::cout << "[Wire] Closing the wire by adding the last edge back to the start point.\n";
                orderedEdges.push_back(ShapeDrawer::CreateEdgeSafe(lastEndPoint, firstStartPnt));
            }
        }

        // Add the edges to the wire builder
        for (auto& edge : orderedEdges) {
            wireBuilder.Add(edge);
            gp_Pnt p1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
            gp_Pnt p2 = BRep_Tool::Pnt(TopExp::LastVertex(edge));
            //std::cout << "[Wire] Adding edge: (" << p1.X() << "," << p1.Y() << ") -> (" << p2.X() << "," << p2.Y() << ")\n";
        }

        if (wireBuilder.IsDone()) {
            TopoDS_Wire wire = wireBuilder.Wire();

            //if (ShapeDrawer::IsClosedWire(wire)) {
                //std::cout << "[Wire] ✅ Successfully created closed wire!\n";

            Handle(AIS_Shape) aisWire = new AIS_Shape(wire);
            aisWire->SetDisplayMode(AIS_WireFrame);
            native->context->Display(aisWire, Standard_True);

            // 🧹 Replace old persisted shapes with the closed wire
            native->persistedLines.clear();
            native->persistedLines.push_back(aisWire);

            // 🧹 Clear current loop edges to prepare for the next drawing
            native->currentLoopEdges.clear();
            /*}
            else {
                std::cout << "[Wire] ⚠ Wire created but NOT closed.\n";
            }*/
        }
        else {
            //std::cout << "[Wire] ❌ Wire builder failed — edges may be out of order or not connected.\n";
        }

        // ✅ End drawing mode
        ViewHelper::ClearCreateEntity(native);
        native->persistedLines.clear();
        native->currentLoopEdges.clear();
    }




    // ========= CREATE EDGE =========
    TopoDS_Edge edge = ShapeDrawer::CreateEdgeSafe(p1, p2);
    if (edge.IsNull())
        return nullptr;

    // ========= WRAP IN AIS_Shape =========
    Handle(AIS_Shape) aisLine = new AIS_Shape(edge);
    aisLine->SetDisplayMode(AIS_WireFrame);

    // ========= APPEARANCE =========
    Quantity_Color color(Quantity_NOC_WHITE);
    Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(color, Aspect_TOL_SOLID, 2.0);
    Handle(Prs3d_Drawer) drawer = aisLine->Attributes();
    drawer->SetWireAspect(lineAspect);
    aisLine->SetAttributes(drawer);

    // ========= DISPLAY =========
    ctx->Display(aisLine, Standard_False);
    ctx->SetColor(aisLine, color, Standard_False);
    ctx->Redisplay(aisLine, Standard_False);
    ctx->UpdateCurrentViewer();

    // ========= PERSIST =========
    if (!aisLine.IsNull()) {
        native->persistedLines.push_back(aisLine);      // optional history
        native->currentLoopEdges.push_back(aisLine);    // ✅ for wire building
    }


    return aisLine;
}

array<int>^ LineDrawer::DrawLineBatch(
    System::IntPtr viewerHandlePtr,
    array<double>^ x1, array<double>^ y1, array<double>^ z1,
    array<double>^ x2, array<double>^ y2, array<double>^ z2,
    array<System::String^>^ lineTypes,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency)
{
    if (viewerHandlePtr == System::IntPtr::Zero)
        return gcnew array<int>(0);

    /*AIS_InteractiveContext* rawCtx =
        static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx)
        return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);*/

    NativeViewerHandle* native = static_cast<NativeViewerHandle*>(viewerHandlePtr.ToPointer());

    if (!native || native->context.IsNull())
        return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx = native->context;


    int n = x1->Length;
    auto ids = gcnew array<int>(n);

    for (int i = 0; i < n; ++i)
    {
        /*gp_Pnt p1(x1[i], y1[i], z1[i]);
        gp_Pnt p2(x2[i], y2[i], z2[i]);*/
        gp_Pnt p1(x1[i], y1[i], 0);
        gp_Pnt p2(x2[i], y2[i], 0);
        if (p1.IsEqual(p2, 1e-9)) { ids[i] = 0; continue; }

        GC_MakeSegment seg(p1, p2);
        if (!seg.IsDone()) { ids[i] = 0; continue; }

        Handle(Geom_TrimmedCurve) geomLine = seg.Value();
        Handle(AIS_Shape) aisLine = new AIS_Shape(BRepBuilderAPI_MakeEdge(geomLine));
        aisLine->SetDisplayMode(AIS_WireFrame);

        // Map DXF linetype string to Aspect_TypeOfLine
        Aspect_TypeOfLine occType = Aspect_TOL_SOLID; // default
        if (lineTypes != nullptr && i < lineTypes->Length && lineTypes[i] != nullptr)
        {
            System::String^ s = lineTypes[i]->Trim()->ToUpperInvariant();
            if (s->IndexOf("DASH") >= 0)         occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED1") >= 0)     occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DASHED2") >= 0)     occType = Aspect_TOL_DASH;
            else if (s->IndexOf("DOT") >= 0)     occType = Aspect_TOL_DOT;
            else if (s->IndexOf("CENTER") >= 0)  occType = Aspect_TOL_DOTDASH;
            else if (s->IndexOf("CENTER1") >= 0)  occType = Aspect_TOL_DOTDASH;
            else if (s->IndexOf("CENTER2") >= 0)  occType = Aspect_TOL_DOTDASH;
            else                                occType = Aspect_TOL_SOLID;
        }

        // Build per-line colour
        double dr = (r != nullptr && i < r->Length) ? (double)r[i] / 255.0 : 0.5;
        double dg = (g != nullptr && i < g->Length) ? (double)g[i] / 255.0 : 0.5;
        double db = (b != nullptr && i < b->Length) ? (double)b[i] / 255.0 : 0.5;
        Quantity_Color qcol(dr, dg, db, Quantity_TOC_RGB);

        // *** get current drawer or create new ***
        Handle(Prs3d_Drawer) drawer = aisLine->Attributes();


        // *** create new LineAspect with our colour + linetype ***
        Handle(Prs3d_LineAspect) lineAsp =
            new Prs3d_LineAspect(qcol, occType, 1.0);

        drawer->SetWireAspect(lineAsp);
        aisLine->SetAttributes(drawer);

        // Display, then still apply colour & transparency via context
        ctx->Display(aisLine, Standard_False);
        ctx->SetColor(aisLine, qcol, Standard_False);

        // restore aspect
        aisLine->Attributes()->SetLineAspect(lineAsp);
        ctx->Redisplay(aisLine, Standard_False);


        /*if (transparency != nullptr && i < transparency->Length)
            ctx->SetTransparency(aisLine, transparency[i], Standard_False);*/

            //ctx->Activate(aisLine);

        ids[i] = (int)aisLine.get(); // track pointer as ID

        native->persistedLines.push_back(aisLine);
    }

    ctx->UpdateCurrentViewer();
    return ids;
}

System::IntPtr LineDrawer::MakeLine(double x1, double y1, double z1, double x2, double y2, double z2)
{
    gp_Pnt p1(x1, y1, z1), p2(x2, y2, z2);
    Handle(AIS_Shape) shape = new AIS_Shape(BRepBuilderAPI_MakeEdge(p1, p2));
    return System::IntPtr(shape.get());
}