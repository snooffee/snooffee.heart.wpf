using PotaOCC;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using static Potacad.Helpers.DxfHelper;
namespace Potacad.Helpers
{
    public static class EntityDrawerHelper
    {
        public static readonly Dictionary<int, object> _dxfShapeDict = new();
        public static int _nextShapeId = 1; // incremental unique ID for each shape

        private const int BatchSize = 5000;

        public static IEnumerable<(string code, string value)> EnumerateDxfPairs(TextReader sr)
        {
            string? code;
            while ((code = sr.ReadLine()) != null)
            {
                string? value = sr.ReadLine();
                if (value == null)
                    yield break;

                yield return (code.Trim(), value.Trim());
            }
        }
        public static List<(string code, string value)> ReadDxfPairs(string path)
        {
            var pairs = new List<(string code, string value)>(8192);
            using var sr = new StreamReader(path);
            string? code;
            while ((code = sr.ReadLine()) != null)
            {
                string? value = sr.ReadLine();
                if (value == null) break;
                pairs.Add((code.Trim(), value.Trim()));
            }
            return pairs;
        }
        public static IEnumerator<(string Code, string Value)> PushBack(
        IEnumerator<(string Code, string Value)> enumerator,
        (string Code, string Value) current)
        {
            // Create a new enumerator with the "pushed back" item
            IEnumerable<(string, string)> Inner()
            {
                yield return current;
                while (enumerator.MoveNext())
                    yield return enumerator.Current;
            }
            return Inner().GetEnumerator();
        }

        // A method to handle batching and drawing
        public static async Task DrawEntitiesBatch<T>(List<T> entitiesList, Func<List<T>, Task> drawBatchFunc)
        {
            var batch = new List<T>(BatchSize);

            foreach (var entity in entitiesList)
            {
                batch.Add(entity);
                if (batch.Count >= BatchSize)
                {
                    await drawBatchFunc(batch);  // Call drawing method
                    batch.Clear();  // Clear the batch
                    await Task.Yield();  // Yield for UI update
                }
            }

            // Handle any remaining items
            if (batch.Count > 0)
            {
                await drawBatchFunc(batch);
            }
        }
        // ----- Color mapper -----
        public static (byte r, byte g, byte b) AcadColorToRgb(int index)
        {
            return index switch
            {
                1 => (255, 0, 0),
                2 => (255, 255, 0),
                3 => (0, 255, 0),
                4 => (0, 255, 255),
                5 => (0, 0, 255),
                6 => (255, 0, 255),
                7 => (128, 128, 128),
                _ => (128, 128, 128)
            };
        }
        public static unsafe async Task Spline(ViewerHandle viewer, List<(double x, double y, double z)> controlPoints, int color, int degree, List<double> rawKnots = null, List<int> rawMultiplicities = null)
        {
            if (viewer?.ContextPtr == IntPtr.Zero || controlPoints == null || controlPoints.Count < 2)
                return;

            int numPoints = controlPoints.Count;
            //Console.WriteLine($"Number of control points: {numPoints}");

            if (degree < 1 || degree >= numPoints)
            {
                Console.WriteLine($"Invalid spline degree {degree} for {numPoints} control points.");
                return;
            }

            // Convert to arrays
            double[] xArr = controlPoints.Select(p => p.x).ToArray();
            double[] yArr = controlPoints.Select(p => p.y).ToArray();
            double[] zArr = controlPoints.Select(p => p.z).ToArray();


            // Debugging: Print out control points
            /*
            Console.WriteLine("Control Points:");
            foreach (var p in controlPoints)
            {
                Console.WriteLine($"({p.x}, {p.y}, {p.z})");
            }
            */

            var rgb = AcadColorToRgb(color);
            int[] rArr = Enumerable.Repeat((int)rgb.r, numPoints).ToArray();
            int[] gArr = Enumerable.Repeat((int)rgb.g, numPoints).ToArray();
            int[] bArr = Enumerable.Repeat((int)rgb.b, numPoints).ToArray();
            double[] transparencyArr = new double[numPoints]; // All 0.0 = opaque

            // Handle knots and multiplicities
            double[] knotArr;
            int[] multArr;

            if (rawKnots != null && rawMultiplicities != null &&
                rawKnots.Count == rawMultiplicities.Count &&
                rawKnots.Count > 0)
            {
                (knotArr, multArr) = SanitizeKnotMultiplicity(rawKnots, rawMultiplicities, degree);
            }
            else
            {
                (knotArr, multArr) = GenerateUniformClampedKnots(numPoints, degree);
            }

            int numKnots = knotArr.Length;

            //Console.WriteLine("Knots: " + string.Join(", ", knotArr));
            //Console.WriteLine("Multiplicities: " + string.Join(", ", multArr));

            // Pin arrays and get pointers
            fixed (double* xPtr = xArr)
            fixed (double* yPtr = yArr)
            fixed (double* zPtr = zArr)
            fixed (int* rPtr = rArr)
            fixed (int* gPtr = gArr)
            fixed (int* bPtr = bArr)
            fixed (double* transparencyPtr = transparencyArr)
            fixed (double* knotPtr = knotArr)
            fixed (int* multPtr = multArr)
            {
                var ids = SplineDrawer.DrawSplineWithKnotsBatch(
                    viewer.ContextPtr,
                    xPtr, yPtr, zPtr,
                    rPtr, gPtr, bPtr,
                    transparencyPtr,
                    degree,
                    knotPtr,
                    multPtr,
                    numPoints,
                    numKnots);

                for (int i = 0; i < ids.Length; i++)
                {
                    _dxfShapeDict[ids[i]] = new { Type = "Spline", Index = i };
                }
            }
        }
        /// <summary>Generate a safe uniform clamped knot vector + multiplicities.</summary>
        public static (double[] knots, int[] multiplicities) GenerateUniformClampedKnots(int numPoints, int degree)
        {
            // Total number of knots should be numPoints + degree + 1
            int numKnots = numPoints + degree + 1;
            // Number of internal knots should be numKnots - 2 * (degree + 1)
            int internalKnotsCount = numKnots - 2 * (degree + 1);

            // Log the input values and total knots
            //Console.WriteLine($"Generating knots for numPoints: {numPoints}, degree: {degree}");
            //Console.WriteLine($"Total Knots Expected: {numKnots}, Internal Knots Count: {internalKnotsCount}");

            // Lists to store knots and multiplicities
            List<double> knots = new List<double>();
            List<int> multiplicities = new List<int>();

            // Add the start knot (0.0) with multiplicity degree + 1
            knots.Add(0.0);
            multiplicities.Add(degree + 1);

            // Add internal knots (uniformly spaced between 0 and 1)
            if (internalKnotsCount > 0)
            {
                double step = 1.0 / (internalKnotsCount + 1);
                for (int i = 1; i <= internalKnotsCount; i++)
                {
                    knots.Add(i * step);  // Evenly distribute internal knots
                    multiplicities.Add(1); // Internal knots always have multiplicity 1
                }
            }

            // Add the end knot (1.0) with multiplicity degree + 1
            knots.Add(1.0);
            multiplicities.Add(degree + 1);

            // Log the generated knots and multiplicities
            //Console.WriteLine($"Generated Knots: {string.Join(", ", knots)}");
            //Console.WriteLine($"Generated Multiplicities: {string.Join(", ", multiplicities)}");

            // Verify that the number of knots matches the expected count
            //Console.WriteLine($"Final Knots Count: {knots.Count} (Expected: {numKnots})");

            // Return the final result
            return (knots.ToArray(), multiplicities.ToArray());
        }
        /// <summary>Sanitize provided knots and multiplicities: ensure spacing, enforce monotonicity, clamp mults.</summary>
        private static (double[] knots, int[] mults) SanitizeKnotMultiplicity(List<double> rawKnots, List<int> rawMults, int degree)
        {
            double epsilon = 1e-7;

            List<double> uniq = new List<double>();
            List<int> mults = new List<int>();

            // Merge duplicate knots and their multiplicities
            for (int i = 0; i < rawKnots.Count; i++)
            {
                double kv = rawKnots[i];
                int mv = rawMults[i];

                if (uniq.Count == 0)
                {
                    uniq.Add(kv);
                    mults.Add(mv);
                }
                else
                {
                    double last = uniq[1];
                    if (kv <= last + epsilon)
                    {
                        // Merge multiplicity for duplicate or too close knots
                        mults[1] += mv;
                    }
                    else
                    {
                        uniq.Add(kv);
                        mults.Add(mv);
                    }
                }
            }

            // Enforce multiplicity <= degree for interior knots
            for (int i = 1; i + 1 < mults.Count; i++)
            {
                if (mults[i] > degree)
                    mults[i] = degree;
            }

            // First/last multiplicity should be degree + 1 for clamped knots
            mults[0] = Math.Max(mults[0], degree + 1);
            mults[1] = Math.Max(mults[1], degree + 1);

            //Console.WriteLine("Sanitized Knots: " + string.Join(", ", uniq));
            //Console.WriteLine("Sanitized Multiplicities: " + string.Join(", ", mults));

            return (uniq.ToArray(), mults.ToArray());
        }
        public static async Task Polyline(ViewerHandle viewer, List<(double x, double y, double z, int color)> polylineVertices, bool isClosed)
        {
            if (polylineVertices == null || polylineVertices.Count < 2 || viewer?.ContextPtr == IntPtr.Zero) return;

            int n = polylineVertices.Count;

            double[] xArr = polylineVertices.Select(p => p.x).ToArray();
            double[] yArr = polylineVertices.Select(p => p.y).ToArray();
            double[] zArr = polylineVertices.Select(p => p.z).ToArray();

            int[] rArr = polylineVertices.Select(p => (int)AcadColorToRgb(p.color).r).ToArray();
            int[] gArr = polylineVertices.Select(p => (int)AcadColorToRgb(p.color).g).ToArray();
            int[] bArr = polylineVertices.Select(p => (int)AcadColorToRgb(p.color).b).ToArray();

            double[] transparencyArr = new double[n]; // all 0 (opaque)
            bool[] closedArr = new bool[n];           // same value for all vertices
            for (int i = 0; i < n; i++) closedArr[i] = isClosed;

            var ids = PolylineDrawer.DrawPolylineBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                rArr, gArr, bArr,
                transparencyArr,
                closedArr);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Polyline", Data = polylineVertices[i] };
        }
        public static async Task LwPolyline(ViewerHandle viewer, List<(double x, double y, double z, double bulge, int color)> lwpolyVertices, bool isClosed)
        {
            if (lwpolyVertices == null || lwpolyVertices.Count < 2 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = lwpolyVertices.Count;

            // Extract coordinates
            double[] xArr = lwpolyVertices.Select(p => p.x).ToArray();
            double[] yArr = lwpolyVertices.Select(p => p.y).ToArray();
            double[] zArr = lwpolyVertices.Select(p => p.z).ToArray();
            double[] bulgeArr = lwpolyVertices.Select(p => p.bulge).ToArray();

            // Convert colors
            int[] rArr = lwpolyVertices.Select(p => (int)AcadColorToRgb(p.color).r).ToArray();
            int[] gArr = lwpolyVertices.Select(p => (int)AcadColorToRgb(p.color).g).ToArray();
            int[] bArr = lwpolyVertices.Select(p => (int)AcadColorToRgb(p.color).b).ToArray();

            // Transparency (opaque) and closed flags
            double[] transparencyArr = new double[n]; // all 0
            bool[] closedArr = new bool[n];
            for (int i = 0; i < n; i++) closedArr[i] = isClosed;

            // Call the drawing batch method
            var ids = LwPolylineDrawer.DrawLwPolylineBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                bulgeArr,
                rArr, gArr, bArr,
                transparencyArr,
                closedArr);

            // Store in shape dictionary
            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "LwPolyline", Data = lwpolyVertices[i] };
        }
        public static async Task Vertex(ViewerHandle viewer, List<(double x, double y, double z1, double z2, double z3, double z4, int color)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero) return;

            int n = batch.Count;

            // Prepare the arrays for coordinates (all z-coordinates are assumed to be 0 for 2D points)
            double[] xArr = batch.Select(b => b.x).ToArray();
            double[] yArr = batch.Select(b => b.y).ToArray();
            double[] zArr = new double[n];  // Z is assumed to be 0 for 2D points

            // Colors
            int[] rArr = batch.Select(b => (int)AcadColorToRgb(b.color).r).ToArray();
            int[] gArr = batch.Select(b => (int)AcadColorToRgb(b.color).g).ToArray();
            int[] bArr = batch.Select(b => (int)AcadColorToRgb(b.color).b).ToArray();

            // Transparency
            double[] tArr = new double[n]; // Default transparency (0 = fully opaque)

            // Call the method that interfaces with OpenCASCADE to draw the points
            var ids = VertexDrawer.DrawVertexBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,  // Coordinates
                rArr, gArr, bArr,   // Colors
                tArr);              // Transparency

            // Store the IDs of the drawn entities in the dictionary (optional)
            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Vertex", Data = batch[i] };
        }
        public static async Task Text(ViewerHandle viewer, List<(double x, double y, string value, double height, double rotation, int color)> textsListRaw)
        {
            int n = textsListRaw.Count;

            // Prepare arrays
            double[] x = textsListRaw.Select(t => t.x).ToArray();
            double[] y = textsListRaw.Select(t => t.y).ToArray();
            double[] z = new double[n]; // 2D DXF, Z = 0
            string[] textVals = textsListRaw.Select(t => t.value ?? "").ToArray();
            double[] heights = textsListRaw.Select(t => t.height).ToArray();
            double[] rotations = textsListRaw.Select(t => t.rotation).ToArray();
            double[] widthFactors = Enumerable.Repeat(1.0, n).ToArray();  // Default width factor = 1.0

            // Prepare RGB colors
            var rgbList = textsListRaw.Select(t => AcadColorToRgb(t.color)).ToArray();
            int[] r = rgbList.Select(c => (int)c.r).ToArray();
            int[] g = rgbList.Select(c => (int)c.g).ToArray();
            int[] b = rgbList.Select(c => (int)c.b).ToArray();

            // Transparency (opaque)
            double[] transparency = new double[n];

            // Call the method to draw texts
            TextDrawer.DrawTextBatch(
                viewer.NativeHandle,
                x, y, z,
                textVals,
                heights,
                rotations,
                widthFactors,
                r, g, b,
                transparency,
                1.0 // scene scale
            );
        }
        public static async Task ByBlock(ViewerHandle viewer, List<(double x1, double y1, double z1, double x2, double y2, double z2, int color)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero) return;

            int n = batch.Count;

            // Prepare the arrays for coordinates
            double[] x1Arr = batch.Select(b => b.x1).ToArray();
            double[] y1Arr = batch.Select(b => b.y1).ToArray();
            double[] z1Arr = new double[n];  // Z is assumed to be 0 by default

            double[] x2Arr = batch.Select(b => b.x2).ToArray();
            double[] y2Arr = batch.Select(b => b.y2).ToArray();
            double[] z2Arr = new double[n];

            // Colors
            int[] rArr = batch.Select(b => (int)AcadColorToRgb(b.color).r).ToArray();
            int[] gArr = batch.Select(b => (int)AcadColorToRgb(b.color).g).ToArray();
            int[] bArr = batch.Select(b => (int)AcadColorToRgb(b.color).b).ToArray();

            // Transparency
            double[] tArr = new double[n]; // Default transparency (0 = fully opaque)

            // Call the method that interfaces with OpenCASCADE
            var ids = ByblockDrawer.DrawByblockBatch(
                viewer.ContextPtr,
                x1Arr, y1Arr, z1Arr,
                x2Arr, y2Arr, z2Arr,
                rArr, gArr, bArr,
                tArr);

            // Store the IDs of the drawn entities in the dictionary
            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "ByBlock", Data = batch[i] };
        }
        public static async Task Solid(ViewerHandle viewer, List<(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, int color)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero) return;

            int n = batch.Count;
            double[] x1Arr = batch.Select(s => s.x1).ToArray();
            double[] y1Arr = batch.Select(s => s.y1).ToArray();
            double[] z1Arr = new double[n];

            double[] x2Arr = batch.Select(s => s.x2).ToArray();
            double[] y2Arr = batch.Select(s => s.y2).ToArray();
            double[] z2Arr = new double[n];

            double[] x3Arr = batch.Select(s => s.x3).ToArray();
            double[] y3Arr = batch.Select(s => s.y3).ToArray();
            double[] z3Arr = new double[n];

            double[] x4Arr = batch.Select(s => s.x4).ToArray();
            double[] y4Arr = batch.Select(s => s.y4).ToArray();
            double[] z4Arr = new double[n];

            int[] rArr = batch.Select(s => (int)AcadColorToRgb(s.color).r).ToArray();
            int[] gArr = batch.Select(s => (int)AcadColorToRgb(s.color).g).ToArray();
            int[] bArr = batch.Select(s => (int)AcadColorToRgb(s.color).b).ToArray();
            double[] tArr = new double[n];

            var ids = SolidDrawer.DrawSolidBatch(
                viewer.ContextPtr,
                x1Arr, y1Arr, z1Arr,
                x2Arr, y2Arr, z2Arr,
                x3Arr, y3Arr, z3Arr,
                x4Arr, y4Arr, z4Arr,
                rArr, gArr, bArr,
                tArr);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Solid", Data = batch[i] };
        }
        public static async Task Arc(ViewerHandle viewer, List<(double cx, double cy, double r, double startAngle, double endAngle, int color, string lineType)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;
            double[] xArr = batch.Select(a => a.cx).ToArray();
            double[] yArr = batch.Select(a => a.cy).ToArray();
            double[] zArr = new double[n];
            double[] rArr = batch.Select(a => a.r).ToArray();
            double[] startArr = batch.Select(a => a.startAngle).ToArray();
            double[] endArr = batch.Select(a => a.endAngle).ToArray();
            int[] cR = batch.Select(a => (int)AcadColorToRgb(a.color).r).ToArray();
            int[] cG = batch.Select(a => (int)AcadColorToRgb(a.color).g).ToArray();
            int[] cB = batch.Select(a => (int)AcadColorToRgb(a.color).b).ToArray();
            double[] tArr = new double[n];
            string[] lineTypeArr = batch.Select(a => a.lineType ?? "CONTINUOUS").ToArray();

            var ids = ArcDrawer.DrawArcBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                rArr,
                startArr, endArr,
                lineTypeArr,
                cR, cG, cB,
                tArr);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Arc", Data = batch[i] };
        }
        public static async Task Point(ViewerHandle viewer, List<(double x, double y, int color)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;
            double[] xArr = batch.Select(p => p.x).ToArray();
            double[] yArr = batch.Select(p => p.y).ToArray();
            double[] zArr = new double[n]; // 2D, so Z=0

            var rgb = batch.Select(p => AcadColorToRgb(p.color)).ToArray();
            int[] rArr = rgb.Select(c => (int)c.r).ToArray();
            int[] gArr = rgb.Select(c => (int)c.g).ToArray();
            int[] bArr = rgb.Select(c => (int)c.b).ToArray();
            double[] tArr = new double[n]; // no transparency

            var ids = PointDrawer.DrawPointBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                rArr, gArr, bArr,
                tArr);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Point", Data = batch[i] };
        }
        public static async Task Line(ViewerHandle viewer, List<(double x1, double y1, double x2, double y2, int color, string lineType)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;
            double[] x1Arr = batch.Select(b => b.x1).ToArray();
            double[] y1Arr = batch.Select(b => b.y1).ToArray();
            double[] z1Arr = new double[n];
            double[] x2Arr = batch.Select(b => b.x2).ToArray();
            double[] y2Arr = batch.Select(b => b.y2).ToArray();
            double[] z2Arr = new double[n];
            int[] rArr = batch.Select(b => (int)AcadColorToRgb(b.color).r).ToArray();
            int[] gArr = batch.Select(b => (int)AcadColorToRgb(b.color).g).ToArray();
            int[] bArr = batch.Select(b => (int)AcadColorToRgb(b.color).b).ToArray();
            double[] tArr = new double[n];

            string[] lineTypeArr = batch.Select(b => b.lineType ?? "CONTINUOUS").ToArray();

            var ids = LineDrawer.DrawLineBatch(
                viewer.NativeHandle,
                x1Arr, y1Arr, z1Arr,
                x2Arr, y2Arr, z2Arr,
                lineTypeArr,
                rArr, gArr, bArr, tArr);
            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Line", Data = batch[i] };

        }
        public static async Task Circle(ViewerHandle viewer, List<(double cx, double cy, double r, int color, string lineType)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;
            double[] xArr = batch.Select(c => c.cx).ToArray();
            double[] yArr = batch.Select(c => c.cy).ToArray();
            double[] zArr = new double[n];
            double[] rArr = batch.Select(c => c.r).ToArray();
            int[] cR = batch.Select(c => (int)AcadColorToRgb(c.color).r).ToArray();
            int[] cG = batch.Select(c => (int)AcadColorToRgb(c.color).g).ToArray();
            int[] cB = batch.Select(c => (int)AcadColorToRgb(c.color).b).ToArray();
            double[] tArr = new double[n];

            string[] lineTypeArr = batch.Select(b => b.lineType ?? "CONTINUOUS").ToArray();

            var ids = CircleDrawer.DrawCircleBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                rArr,
                lineTypeArr,
                cR, cG, cB, tArr);
            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Circle", Data = batch[i] };

        }
        public static async Task Ellipse(ViewerHandle viewer, List<(double cx, double cy, double semiMajor, double semiMinor, double rotationAngle, int color, string lineType)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;
            double[] xArr = batch.Select(c => c.cx).ToArray();
            double[] yArr = batch.Select(c => c.cy).ToArray();
            double[] zArr = new double[n];  // Assuming 0 for z coordinate (planar ellipses)
            double[] semiMajorArr = batch.Select(c => c.semiMajor).ToArray();
            double[] semiMinorArr = batch.Select(c => c.semiMinor).ToArray();
            double[] rotationAngleArr = batch.Select(c => c.rotationAngle).ToArray();
            int[] cR = batch.Select(c => (int)AcadColorToRgb(c.color).r).ToArray();
            int[] cG = batch.Select(c => (int)AcadColorToRgb(c.color).g).ToArray();
            int[] cB = batch.Select(c => (int)AcadColorToRgb(c.color).b).ToArray();
            double[] tArr = new double[n];  // Assuming no transparency (can be adjusted if needed)

            string[] lineTypeArr = batch.Select(b => b.lineType ?? "CONTINUOUS").ToArray();

            var ids = EllipseDrawer.DrawEllipseBatch(
                viewer.ContextPtr,
                xArr, yArr, zArr,
                semiMajorArr, semiMinorArr, rotationAngleArr,
                lineTypeArr,
                cR, cG, cB, tArr);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Ellipse", Data = batch[i] };
        }
        public static async Task Hatch(ViewerHandle viewer, List<List<(double x, double y)>> boundaryGroups, int color, string patternName, bool isSolid)
        {
            try
            {
                // ✅ Safety checks
                if (viewer == null || viewer.ContextPtr == IntPtr.Zero || viewer.ContextPtr == null)
                {
                    Console.WriteLine("❌ Viewer or ContextPtr is null");
                    return;
                }

                if (boundaryGroups == null || boundaryGroups.Count == 0)
                {
                    Console.WriteLine("❌ Hatch requires at least one boundary");
                    return;
                }

                // Filter valid boundaries (each boundary must have >=3 points)
                var validGroups = boundaryGroups
                    .Select(g => g.Where(p => !double.IsNaN(p.x) && !double.IsNaN(p.y)).ToList())
                    .Where(g => g.Count >= 3)
                    .ToList();

                if (validGroups.Count == 0)
                {
                    Console.WriteLine("❌ No valid boundaries for hatch");
                    return;
                }

                // ✅ Prepare arrays for OCCT native call (outer + inner wires)
                var boundaryXArr = new double[1][][];
                var boundaryYArr = new double[1][][];
                var boundaryZArr = new double[1][][];

                // Single hatch entry (batch of 1)
                boundaryXArr[0] = validGroups.Select(g => g.Select(p => p.x).ToArray()).ToArray();
                boundaryYArr[0] = validGroups.Select(g => g.Select(p => p.y).ToArray()).ToArray();
                boundaryZArr[0] = validGroups.Select(g => new double[g.Count]).ToArray(); // planar Z = 0

                var rgb = AcadColorToRgb(color);
                int[] rArr = new int[1] { (int)rgb.r };
                int[] gArr = new int[1] { (int)rgb.g };
                int[] bArr = new int[1] { (int)rgb.b };

                double[] tArr = new double[1] { 0.0 }; // opaque
                string[] patternArr = new string[1] { patternName ?? "SOLID" };
                bool[] solidArr = new bool[1] { isSolid };

                // ✅ Call native OCCT batch
                var ids = HatchDrawer.DrawHatchBatch(
                    viewer.NativeHandle,
                    boundaryXArr,
                    boundaryYArr,
                    boundaryZArr,
                    rArr, gArr, bArr,
                    tArr,
                    patternArr,
                    solidArr
                );

                if (ids == null)
                {
                    Console.WriteLine("❌ HatchDrawer returned null");
                    return;
                }

                // ✅ Register hatch IDs
                for (int i = 0; i < ids.Length; i++)
                {
                    if (!_dxfShapeDict.ContainsKey(ids[i]))
                    {
                        _dxfShapeDict[ids[i]] = new
                        {
                            Type = "Hatch",
                            Data = new { boundaryGroups, color, patternName, isSolid }
                        };
                    }
                }

                Console.WriteLine($"✅ Hatch created successfully. Total: {ids.Length}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"❌ Exception in Hatch(): {ex}");
            }
        }
        public static async Task Faces3D(ViewerHandle viewer, List<(double x1, double y1, double z1, double x2, double y2, double z2, double x3, double y3, double z3, double x4, double y4, double z4, int color)> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;

            // Coordinates
            double[] x1Arr = batch.Select(f => f.x1).ToArray();
            double[] y1Arr = batch.Select(f => f.y1).ToArray();
            double[] z1Arr = batch.Select(f => f.z1).ToArray();

            double[] x2Arr = batch.Select(f => f.x2).ToArray();
            double[] y2Arr = batch.Select(f => f.y2).ToArray();
            double[] z2Arr = batch.Select(f => f.z2).ToArray();

            double[] x3Arr = batch.Select(f => f.x3).ToArray();
            double[] y3Arr = batch.Select(f => f.y3).ToArray();
            double[] z3Arr = batch.Select(f => f.z3).ToArray();

            double[] x4Arr = batch.Select(f => f.x4).ToArray();
            double[] y4Arr = batch.Select(f => f.y4).ToArray();
            double[] z4Arr = batch.Select(f => f.z4).ToArray();

            // Colors
            int[] rArr = batch.Select(f => (int)AcadColorToRgb(f.color).r).ToArray();
            int[] gArr = batch.Select(f => (int)AcadColorToRgb(f.color).g).ToArray();
            int[] bArr = batch.Select(f => (int)AcadColorToRgb(f.color).b).ToArray();

            // Transparency (fully opaque)
            double[] tArr = new double[n];

            var ids = Faces3DDrawer.DrawFaces3DBatch(
                viewer.ContextPtr,
                x1Arr, y1Arr, z1Arr,
                x2Arr, y2Arr, z2Arr,
                x3Arr, y3Arr, z3Arr,
                x4Arr, y4Arr, z4Arr,
                rArr, gArr, bArr,
                tArr);

            // Store entity info for reference / selection
            for (int i = 0; i < ids.Length; i++) _dxfShapeDict[ids[i]] = new { Type = "3DFACE", Data = batch[i] };
        }
        private static (int r, int g, int b) ColorNameToRgb(string colorName)
        {
            switch (colorName?.ToLower())
            {
                case "red": return (255, 0, 0);
                case "green": return (0, 255, 0);
                case "blue": return (0, 0, 255);
                case "yellow": return (255, 255, 0);
                case "cyan": return (0, 255, 255);
                case "magenta": return (255, 0, 255);
                case "white": return (255, 255, 255);
                case "black": return (0, 0, 0);
                default: return (255, 255, 255); // default white
            }
        }
        public static async Task Dimension(ViewerHandle viewer, List<DimensionItem> batch)
        {
            if (batch == null || batch.Count == 0 || viewer?.ContextPtr == IntPtr.Zero)
                return;

            int n = batch.Count;

            double[] startXArr = batch.Select(d => d.StartX).ToArray();
            double[] startYArr = batch.Select(d => d.StartY).ToArray();
            double[] startZArr = batch.Select(d => d.StartZ).ToArray();

            double[] endXArr = batch.Select(d => d.EndX).ToArray();
            double[] endYArr = batch.Select(d => d.EndY).ToArray();
            double[] endZArr = batch.Select(d => d.EndZ).ToArray();

            double[] leaderXArr = batch.Select(d => d.LeaderX).ToArray();
            double[] leaderYArr = batch.Select(d => d.LeaderY).ToArray();
            double[] leaderZArr = new double[n]; // Z=0 for leader

            int[] cR = batch.Select(d => ColorNameToRgb(d.DimColor).r).ToArray();
            int[] cG = batch.Select(d => ColorNameToRgb(d.DimColor).g).ToArray();
            int[] cB = batch.Select(d => ColorNameToRgb(d.DimColor).b).ToArray();

            // 2️⃣ Hardcode color to blue
            //int[] cR = Enumerable.Repeat(0, n).ToArray();
            //int[] cG = Enumerable.Repeat(0, n).ToArray();
            //int[] cB = Enumerable.Repeat(255, n).ToArray();


            string[] textArr = batch.Select(d => d.Text ?? "").ToArray();
            string[] typeArr = batch.Select(d => d.Type ?? "Linear").ToArray();

            var ids = DimensionDrawer.DrawDimensionBatch(
                viewer.ContextPtr,
                startXArr, startYArr, startZArr,
                endXArr, endYArr, endZArr,
                leaderXArr, leaderYArr, leaderZArr,
                textArr,
                typeArr,
                typeArr,
                cR, cG, cB);

            for (int i = 0; i < ids.Length; i++)
                _dxfShapeDict[ids[i]] = new { Type = "Dimension", Data = batch[i] };
        }
    }
}