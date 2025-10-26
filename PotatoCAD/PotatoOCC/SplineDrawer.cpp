#include "pch.h"
#include "NativeViewerHandle.h"
#include "SplineDrawer.h"
#include <AIS_InteractiveContext.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColStd_HArray1OfReal.hxx>
#include <TColStd_HArray1OfInteger.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <AIS_Shape.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopoDS_Edge.hxx>
#include <Quantity_Color.hxx>
#include <Standard_ConstructionError.hxx>

using namespace PotatoOCC;
using namespace System::Collections::Generic;

array<int>^ SplineDrawer::DrawSplineBatch(
    System::IntPtr ctxPtr,
    array<double>^ x, array<double>^ y, array<double>^ z,
    array<int>^ r, array<int>^ g, array<int>^ b,
    array<double>^ transparency,
    int degree)
{
    // Early return if inputs are invalid
    if (ctxPtr == System::IntPtr::Zero || x == nullptr || y == nullptr || z == nullptr)
        return gcnew array<int>(0);

    // Cast the context pointer to the OpenCASCADE context pointer
    AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());
    if (!rawCtx) return gcnew array<int>(0);

    Handle(AIS_InteractiveContext) ctx(rawCtx);
    int nSplines = x->Length;

    // Create a list to store the IDs of the drawn splines
    List<int>^ ids = gcnew List<int>();

    // Iterate through all the splines
    for (int splineIdx = 0; splineIdx < nSplines; ++splineIdx)
    {
        // Extract data for the current spline
        array<double>^ currentX = x;
        array<double>^ currentY = y;
        array<double>^ currentZ = z;
        int rVal = r[splineIdx];
        int gVal = g[splineIdx];
        int bVal = b[splineIdx];
        double transparencyVal = transparency[splineIdx];

        int nPoints = currentX->Length;

        // Ensure the number of control points is compatible with the degree
        if (nPoints < degree + 1) {
            //std::cerr << "OpenCASCADE error: Not enough control points for the specified degree " << degree << std::endl;
            continue;  // Skip invalid spline
        }

        // Prepare control points for the B-Spline
        Handle(TColgp_HArray1OfPnt) poles = new TColgp_HArray1OfPnt(1, nPoints);
        //std::cout << "Creating B-spline with " << nPoints << " points and degree " << degree << std::endl;
        for (int i = 0; i < nPoints; ++i)
        {
            //std::cout << "Control Point " << i << ": X=" << currentX[i] << ", Y=" << currentY[i] << ", Z=" << currentZ[i] << std::endl;
            poles->SetValue(i + 1, gp_Pnt(currentX[i], currentY[i], currentZ[i]));
        }

        // Prepare knots and multiplicities (adjust based on the degree)
        Handle(TColStd_HArray1OfReal) knotVals = new TColStd_HArray1OfReal(1, nPoints + degree + 1);
        Handle(TColStd_HArray1OfInteger) multiplicities = new TColStd_HArray1OfInteger(1, nPoints + degree + 1);

        // Create the knot vector: Simple open knot vector for non-periodic B-splines
        double minSpacing = 1e-5; // Minimum allowable spacing between knots

        for (int i = 0; i < nPoints + degree + 1; ++i)
        {
            if (i < degree)
                knotVals->SetValue(i + 1, 0.0);
            else if (i >= nPoints)
                knotVals->SetValue(i + 1, 1.0);
            else
            {
                // Compute internal knot value with a safeguard against very small intervals
                double internalKnot = (Standard_Real)(i - degree + 1) / (nPoints - degree);
                if (i > 0)
                {
                    // Enforce a minimum spacing between consecutive knots
                    double prevKnot = knotVals->Value(i);
                    internalKnot = std::max(internalKnot, prevKnot + minSpacing);
                }
                knotVals->SetValue(i + 1, internalKnot);
            }

            multiplicities->SetValue(i + 1, 1);
        }

        // Create the B-Spline curve
        try
        {
            // Create the B-Spline curve with the given control points, knots, multiplicities, and degree
            Handle(Geom_BSplineCurve) spline = new Geom_BSplineCurve(
                poles->Array1(),      // Control points
                knotVals->Array1(),   // Knot values
                multiplicities->Array1(), // Multiplicities
                degree,               // Degree (passed from the function)
                true
            );

            // Wrap the spline into an AIS_Shape for visualization
            Handle(AIS_Shape) aisSpline = new AIS_Shape(BRepBuilderAPI_MakeEdge(spline));

            // Apply color and transparency
            Quantity_Color qcol(rVal / 255.0, gVal / 255.0, bVal / 255.0, Quantity_TOC_RGB);  // Convert to RGB (normalized)

            ctx->Display(aisSpline, Standard_False);
            ctx->SetColor(aisSpline, qcol, Standard_False);
            ctx->SetTransparency(aisSpline, transparencyVal, Standard_False);

            ctx->Activate(aisSpline, TopAbs_SHAPE, Standard_False);

            // Ensure the shape is properly activated and displayed
            ctx->Redisplay(aisSpline, Standard_False); // Redraw the shape in the viewer

            // Store the ID of the AIS_Shape
            int shapeId = (int)aisSpline.get();  // Get the ID of the drawn spline
            ids->Add(shapeId);
        }
        catch (const Standard_Failure& e) {
            // If an error occurs during spline creation, log the error and continue with the next spline
            std::cerr << "OpenCASCADE error: " << e.GetMessageString() << std::endl;
            continue;
        }
    }

    // Return the IDs of the drawn splines
    ctx->UpdateCurrentViewer();
    return ids->ToArray();
}

void GenerateKnots(int numPoints, int degree, std::vector<double>& knots, std::vector<int>& multiplicities)
{
    int totalKnots = numPoints + degree + 1;  // Total number of knots (n + p + 1)
    knots.resize(totalKnots);
    multiplicities.resize(totalKnots);

    // Set the first 'degree+1' knots to 0
    for (int i = 0; i <= degree; ++i) {
        knots[i] = 0.0;
        multiplicities[i] = degree + 1;  // The multiplicity of the first 'degree+1' knots is degree + 1
    }

    // Set the last 'degree+1' knots to 1
    for (int i = totalKnots - degree - 1; i < totalKnots; ++i) {
        knots[i] = 1.0;
        multiplicities[i] = degree + 1;  // The multiplicity of the last 'degree+1' knots is degree + 1
    }

    // Generate internal knots (evenly spaced between 0 and 1)
    double step = 1.0 / (numPoints - degree);  // Evenly distribute the internal knots
    for (int i = degree + 1; i < totalKnots - degree - 1; ++i) {
        knots[i] = (i - degree) * step;
        multiplicities[i] = 1;  // Internal knots have multiplicity 1
    }

    // Debug output for the generated knots
    /*
    std::cout << "Generated Knots: ";
    for (double knot : knots) {
        std::cout << knot << " ";
    }
    std::cout << std::endl;

    std::cout << "Generated Multiplicities: ";
    for (int mult : multiplicities) {
        std::cout << mult << " ";
    }
    std::cout << std::endl;
    */
}

TopoDS_Edge DrawSplineWithKnots(
    const std::vector<double>& xArr,
    const std::vector<double>& yArr,
    const std::vector<double>& zArr,
    int degree,
    const std::vector<double>& knotArr,
    const std::vector<int>& multArr)
{
    int n = xArr.size();
    if (n < degree + 1) {
        throw std::runtime_error("Too few control points.");
    }
    int nk = knotArr.size();
    if (nk != multArr.size()) {
        throw std::runtime_error("Knot and multiplicity arrays mismatch.");
    }

    // Build poles (control points)
    TColgp_Array1OfPnt poles(1, n);
    for (int i = 0; i < n; ++i) {
        poles.SetValue(i + 1, gp_Pnt(xArr[i], yArr[i], zArr[i]));
    }

    // Build knots and multiplicities
    TColStd_Array1OfReal occKnots(1, nk);
    TColStd_Array1OfInteger occMults(1, nk);
    for (int i = 0; i < nk; ++i) {
        occKnots.SetValue(i + 1, knotArr[i]);
        occMults.SetValue(i + 1, multArr[i]);
    }

    // Create the BSpline curve
    Handle(Geom_BSplineCurve) curve;
    try {
        curve = new Geom_BSplineCurve(poles, occKnots, occMults, degree, false /* periodic */);
        //std::cout << "Spline created successfully!" << std::endl;
    }
    catch (Standard_ConstructionError& e) {
        std::string msg = e.GetMessageString();
        throw std::runtime_error("OCC ConstructionError: " + msg);
    }

    // Create an edge from the curve
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(curve);

    return edge;
}
Handle(AIS_InteractiveContext) globalViewerContext;
void AddShapeToViewer(const Handle(AIS_Shape)& shape, const Quantity_Color& col)
{
    if (shape.IsNull() || globalViewerContext.IsNull()) {
        std::cout << "Viewer context or shape is null!" << std::endl;
        return;
    }

    shape->SetColor(col);
    globalViewerContext->Display(shape, true); // true = immediate redraw
    globalViewerContext->UpdateCurrentViewer();  // Refresh the viewer
    //std::cout << "Shape added to the viewer!" << std::endl;
}
array<int>^ SplineDrawer::DrawSplineWithKnotsBatch(
    IntPtr ctxPtr,
    const double* xArr,
    const double* yArr,
    const double* zArr,
    const int* rArr,
    const int* gArr,
    const int* bArr,
    const double* transparencyArr,
    int degree,
    const double* knotArr,
    const int* multArr,
    int numPoints,
    int numKnots)
{
    try
    {
        if (ctxPtr == IntPtr::Zero) return gcnew array<int>(0);
        AIS_InteractiveContext* rawCtx = static_cast<AIS_InteractiveContext*>(ctxPtr.ToPointer());

        Handle(AIS_InteractiveContext) ctx(rawCtx);

        if (numPoints < 2 || numKnots < 2) return gcnew array<int>(0);

        // Convert input arrays to std::vector
        std::vector<double> vx(xArr, xArr + numPoints);
        std::vector<double> vy(yArr, yArr + numPoints);
        std::vector<double> vz(zArr, zArr + numPoints);
        std::vector<double> vk(knotArr, knotArr + numKnots);
        std::vector<int> vm(multArr, multArr + numKnots);

        // Build the spline edge
        TopoDS_Edge edge = DrawSplineWithKnots(vx, vy, vz, degree, vk, vm);

        // Debug: Ensure spline edge is created successfully
        //std::cout << "Edge created successfully, preparing to add to viewer." << std::endl;


        // Wrap it in AIS_Shape
        Handle(AIS_Shape) aisShape = new AIS_Shape(edge);

        // Set color (from first pointfs color)
        Quantity_Color qcol(
            static_cast<Standard_Real>(rArr[0]) / 255.0,
            static_cast<Standard_Real>(gArr[0]) / 255.0,
            static_cast<Standard_Real>(bArr[0]) / 255.0,
            Quantity_TOC_RGB);

        ctx->SetColor(aisShape, qcol, Standard_False);
        ctx->Display(aisShape, Standard_False);

        // Return an ID (assuming one shape)
        static int nextId = 1;
        int thisId = nextId++;

        // Return as managed array
        array<int>^ result = gcnew array<int>(1);
        result[0] = thisId;
        return result;
    }
    catch (Standard_Failure& e)
    {
        std::cerr << "OpenCASCADE exception: " << e.GetMessageString() << std::endl;
        throw;
    }
}