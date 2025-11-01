using PotaOCC;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq.Expressions;
using System.Threading.Tasks;
using static Potacad.Helpers.EntityDrawerHelper;
using static Potacad.MouseActions;
using static Potacad.Helpers.DxfHelper;
using static PotaOCC.ShapeDrawer;
using static PotaOCC.ViewerManager;
using Application = System.Windows.Application;
using System.Linq;

namespace Potacad.Helpers
{
    public class DxfHelper
    {
        private readonly dynamic viewer;
        private readonly dynamic HostPanel;
        private readonly dynamic loadingOverlay;

        public DxfHelper(dynamic viewer, dynamic hostPanel, dynamic loadingOverlay)
        {
            this.viewer = viewer;
            this.HostPanel = hostPanel;
            this.loadingOverlay = loadingOverlay;
        }

        // ============================================================
        #region --- DimensionItem Record ---
        // ============================================================
        public class DimensionItem
        {
            public double StartX { get; set; }
            public double StartY { get; set; }
            public double StartZ { get; set; }

            public double EndX { get; set; }
            public double EndY { get; set; }
            public double EndZ { get; set; }

            public double LeaderX { get; set; }
            public double LeaderY { get; set; }
            public double LeaderZ { get; set; }

            public string Text { get; set; } = string.Empty;
            public string Type { get; set; } = "Linear"; // Linear, Radial, Diameter, Angular
            public string DimColor { get; set; } = "Yellow";
        }
        #endregion

        // ============================================================
        #region --- Dimension Data Models ---
        // ============================================================
        public class LinearDimensionData
        {
            public double startX, startY, startZ;
            public double endX, endY, endZ;
            public double textX, textY, textHeight;
            public double rotationAngle;
            public int dimColor, textColor;
            public string linetype;
            public string text;
        }

        public class RadialDimensionData
        {
            public double centerX, centerY, centerZ;
            public double radius;
            public double textX, textY, textHeight;
            public double rotationAngle;
            public int dimColor, textColor;
            public string linetype;
            public string text;
        }

        public class DiameterDimensionData
        {
            public double centerX, centerY, centerZ;
            public double diameter;
            public double textX, textY, textHeight;
            public double rotationAngle;
            public int dimColor, textColor;
            public string linetype;
            public string text;
        }

        public class AngularDimensionData
        {
            public double vertexX, vertexY, vertexZ;
            public double line1X, line1Y, line1Z;
            public double line2X, line2Y, line2Z;
            public double textX, textY, textHeight;
            public double rotationAngle;
            public int dimColor, textColor;
            public string linetype;
            public string text;
        }
        #endregion


        // ✅ DXF Entity Parser
        private (
        List<(List<(double x, double y, double bulge)> vertices, bool closed, int color)> lwpolylinesList,
        List<(List<(double x, double y)> boundaryPoints, int color, string patternName, bool isSolid)> hatchList,
        List<LinearDimensionData> linearDimensionsList,
        List<AngularDimensionData> angularDimensionsList,
        List<RadialDimensionData> radialDimensionsList,
        List<DiameterDimensionData> diameterDimensionsList,
        List<(double cx, double cy, double semiMajor, double semiMinor, double rotationAngle, int color, string linetype)> ellipsesList,
        List<(List<(double x, double y, double z)> controlPoints, int color, int degree)> splinesList,
        List<(List<(double x, double y, double z, int color)> vertices, bool closed)> polylinesList,
        List<(double x, double y, double z, int color)> verticesList,
        List<(double x1, double y1, double x2, double y2, int color)> byBlocksList,
        List<(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, int color)> solidsList,
        List<(double cx, double cy, double r, double startAngle, double endAngle, int color, string linetype)> arcsList,
        List<(double x, double y, int color)> pointsList,
        List<(double x1, double y1, double x2, double y2, int color, string linetype)> linesList,
        List<(double cx, double cy, double r, int color, string linetype)> circlesList,
        List<(double x, double y, string value, double height, double rotation, int color)> textsList)
        ParseDxfEntities(string filePath)
        {
            var pairs = ReadDxfPairs(filePath);

            var lwpolylinesList = new List<(List<(double x, double y, double bulge)> vertices, bool closed, int color)>(512);
            var linearDimensionsList = new List<LinearDimensionData>(1024);
            var angularDimensionsList = new List<AngularDimensionData>(1024);
            var radialDimensionsList = new List<RadialDimensionData>(1024);
            var diameterDimensionsList = new List<DiameterDimensionData>(1024);
            var ellipsesList = new List<(double cx, double cy, double semiMajor, double semiMinor, double rotationAngle, int color, string linetype)>(1024);
            var splinesList = new List<(List<(double x, double y, double z)> controlPoints, int color, int degree)>(1024);
            var polylinesList = new List<(List<(double x, double y, double z, int color)> vertices, bool closed)>(1024);
            var verticesList = new List<(double, double, double, int)>(4096);
            var byBlocksList = new List<(double, double, double, double, int)>(4096);
            var solidsList = new List<(double, double, double, double, double, double, double, double, int)>(1024);
            var arcsList = new List<(double, double, double, double, double, int, string)>(2048);
            var pointsList = new List<(double, double, int)>(2048);
            var linesList = new List<(double, double, double, double, int, string)>(4096);
            var circlesList = new List<(double, double, double, int, string)>(2048);
            var textsList = new List<(double, double, string, double, double, int)>(2048);
            var hatchList = new List<(List<(double x, double y)> boundaryPoints, int color, string patternName, bool isSolid)>(512);

            bool inEntities = false;

            for (int i = 0; i < pairs.Count; i++)
            {
                var (code, value) = pairs[i];

                // wait until ENTITIES
                if (!inEntities)
                {
                    if (code == "0" && value.Equals("SECTION", StringComparison.OrdinalIgnoreCase)
                        && i + 1 < pairs.Count)
                    {
                        var (c2, v2) = pairs[++i];
                        if (c2 == "2" && v2.Equals("ENTITIES", StringComparison.OrdinalIgnoreCase))
                            inEntities = true;
                    }
                    continue;
                }

                if (code != "0") continue;

                string entity = value.ToUpperInvariant();
                if (entity == "ENDSEC") break;

                // working variables
                double x1 = 0, y1 = 0, x2 = 0, y2 = 0, x3 = 0, y3 = 0, x4 = 0, y4 = 0;
                double cx = 0, cy = 0, r = 0, startAngle = 0, endAngle = 0, semiMajor = 0, semiMinor = 0, rotationAngle = 0;
                double tx = 0, ty = 0, height = 0, rotation = 0;
                double px = 0, py = 0, pz = 0;
                string textValue = string.Empty;
                int color = 7;

                string linetype = "CONTINUOUS";

                // --- POLYLINE special handling ---
                if (entity == "POLYLINE")
                {
                    var currentVertices = new List<(double x, double y, double z, int color)>(32);
                    bool isClosed = false;
                    int localColor = 7;

                    i++; // move to next pair after POLYLINE
                    while (i < pairs.Count)
                    {
                        var (c, v) = pairs[i];

                        if (c == "0" && v.Equals("SEQEND", StringComparison.OrdinalIgnoreCase))
                        {
                            i++; // consume SEQEND
                            break;
                        }

                        if (c == "0" && v.Equals("VERTEX", StringComparison.OrdinalIgnoreCase))
                        {
                            double vx = 0, vy = 0, vz = 0; int vcolor = localColor;

                            // read attributes of this vertex
                            i++; // move to first attribute after VERTEX
                            while (i < pairs.Count)
                            {
                                var (vc, vv) = pairs[i];
                                if (vc == "0") break; // next entity starts here

                                if (vc == "10") double.TryParse(vv, NumberStyles.Float, CultureInfo.InvariantCulture, out vx);
                                else if (vc == "20") double.TryParse(vv, NumberStyles.Float, CultureInfo.InvariantCulture, out vy);
                                else if (vc == "30") double.TryParse(vv, NumberStyles.Float, CultureInfo.InvariantCulture, out vz);
                                else if (vc == "62") int.TryParse(vv, out vcolor);

                                i++;
                            }

                            currentVertices.Add((vx, vy, vz, vcolor));
                            continue; // don’t increment i again here; loop handles it
                        }

                        // flags or color for the polyline itself
                        if (c == "70" && int.TryParse(v, out int flag)) isClosed = (flag & 1) != 0;
                        else if (c == "62") int.TryParse(v, out localColor);

                        i++;
                    }

                    if (currentVertices.Count > 0)
                        polylinesList.Add((currentVertices, isClosed));

                    i--; // so the outer for sees the right next record
                    continue;
                }

                // --- LWPOLYLINE special handling ---
                if (entity == "LWPOLYLINE")
                {
                    var currentVertices = new List<(double x, double y, double bulge)>(32);
                    bool isClosed = false;
                    int localColor = 7;

                    i++; // move to next pair after LWPOLYLINE
                    while (i < pairs.Count)
                    {
                        var (c, v) = pairs[i];

                        if (c == "0") break; // next entity

                        if (c == "10") // X coordinate
                        {
                            double x = 0, y = 0, bulge = 0;

                            double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out x);

                            if (i + 1 < pairs.Count && pairs[i + 1].Item1 == "20")
                            {
                                double.TryParse(pairs[i + 1].Item2, NumberStyles.Float, CultureInfo.InvariantCulture, out y);
                                i++; // skip Y
                            }

                            // Optional bulge (code 42)
                            if (i + 1 < pairs.Count && pairs[i + 1].Item1 == "42")
                            {
                                double.TryParse(pairs[i + 1].Item2, NumberStyles.Float, CultureInfo.InvariantCulture, out bulge);
                                i++;
                            }

                            currentVertices.Add((x, y, bulge));
                        }
                        else if (c == "70" && int.TryParse(v, out int flag))
                        {
                            isClosed = (flag & 1) != 0;
                        }
                        else if (c == "62")
                        {
                            int.TryParse(v, out localColor);
                        }

                        i++;
                    }

                    if (currentVertices.Count > 0)
                        lwpolylinesList.Add((currentVertices, isClosed, localColor));

                    i--; // outer loop continues correctly
                    continue;
                }


                #region --- SPLINE special handling --- 
                // Got error (OpenCASCADE error: BSpline curve: Knots interval values too close) so comment up for future study *****
                if (entity == "SPLINE")
                {
                    int localColor = 7;
                    List<(double x, double y, double z)> controlPoints = new();
                    int splineDegree = 0;

                    i++; // Move to next pair
                    while (i < pairs.Count)
                    {
                        var (c, v) = pairs[i];

                        if (c == "0") break; // End of entity

                        if (c == "70") int.TryParse(v, out _); // spline flags (optional)
                        else if (c == "71") int.TryParse(v, out splineDegree); // spline degree
                        else if (c == "62") int.TryParse(v, out localColor);
                        else if (c == "10")
                        {
                            double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out double x);
                            double y = 0, z = 0;

                            // read next two values: 20 (Y), 30 (Z)
                            if (i + 2 < pairs.Count &&
                                pairs[i + 1].Item1 == "20" &&
                                pairs[i + 2].Item1 == "30")
                            {
                                double.TryParse(pairs[i + 1].Item2, NumberStyles.Float, CultureInfo.InvariantCulture, out y);
                                double.TryParse(pairs[i + 2].Item2, NumberStyles.Float, CultureInfo.InvariantCulture, out z);
                                controlPoints.Add((x, y, z));
                                i += 2; // move past 20, 30
                            }
                        }

                        i++;
                    }

                    if (controlPoints.Count > 0)
                    {
                        splinesList.Add((controlPoints, localColor, splineDegree));
                    }

                    i--; // backtrack to let main loop proceed correctly
                    continue;
                }

                #endregion

                // --- ELLIPSE special handling --- 
                if (entity == "ELLIPSE")
                {
                    i++; // move to next pair after ELLIPSE
                    while (i < pairs.Count)
                    {
                        var (c, v) = pairs[i];

                        // End of entity
                        if (c == "0") break;

                        if (c == "10") double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out cx); // X center
                        else if (c == "20") double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out cy); // Y center
                        else if (c == "40") double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out semiMajor); // Semi-major axis
                        else if (c == "41") double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out semiMinor); // Semi-minor axis
                        else if (c == "50") double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out rotationAngle); // Rotation angle
                        else if (c == "62") int.TryParse(v, out color); // Color
                        else if (c == "6") linetype = v; // Line type

                        i++;
                    }

                    // Add the parsed ellipse data to the ellipsesList
                    ellipsesList.Add((cx, cy, semiMajor, semiMinor, rotationAngle, color, linetype));
                    i--; // so the outer loop sees the right next record
                    continue;
                }

                // ---------------- DIMENSION ----------------
                if (entity == "DIMENSION")
                {
                    double textX = 0, textY = 0, textZ = 0, textHeight = 2.5;
                    x1 = 0; y1 = 0; double z1 = 0;
                    x2 = 0; y2 = 0; double z2 = 0;
                    x3 = 0; y3 = 0; double z3 = 0;
                    x4 = 0; y4 = 0; double z4 = 0;
                    double x5 = 0, y5 = 0, z5 = 0;
                    double x6 = 0, y6 = 0, z6 = 0;
                    rotationAngle = 0.0;
                    string text = string.Empty;
                    linetype = string.Empty;
                    int dimColor = 7, textColor = 7;
                    string dimType = "LINEAR";

                    i++;
                    while (i < pairs.Count)
                    {
                        (code, value) = pairs[i];
                        if (code == "0") { i--; break; }

                        switch (code)
                        {
                            case "2":
                                if (!string.IsNullOrEmpty(value))
                                {
                                    string dimName = value.Trim().ToUpperInvariant();
                                    if (dimName.Contains("RADIAL")) dimType = "RADIUS";
                                    else if (dimName.Contains("DIAMETER")) dimType = "DIAMETER";
                                    else if (dimName.Contains("ANGULAR")) dimType = "ANGULAR";
                                    else dimType = "LINEAR";
                                }
                                break;

                            case "10": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x1); break;
                            case "20": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y1); break;
                            case "30": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z1); break;

                            case "11": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x2); break;
                            case "21": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y2); break;
                            case "31": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z2); break;

                            case "12": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x3); break;
                            case "22": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y3); break;
                            case "32": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z3); break;

                            case "13": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out textX); break;
                            case "23": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out textY); break;
                            case "33": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out textZ); break;

                            case "14": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x4); break;
                            case "24": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y4); break;
                            case "34": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z4); break;

                            case "15": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x5); break;
                            case "25": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y5); break;
                            case "35": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z5); break;

                            case "16": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out x6); break;
                            case "26": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out y6); break;
                            case "36": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out z6); break;

                            case "1":
                            case "3":
                                text = value; break;

                            case "40": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out textHeight); break;
                            case "50": double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out rotationAngle); break;

                            case "62": int.TryParse(value, out dimColor); break;
                            case "6": linetype = value; break;

                            case "70":
                                if (int.TryParse(value, out int flag))
                                {
                                    dimType = flag switch
                                    {
                                        0 or 1 => "LINEAR",
                                        2 => "ALIGNED",
                                        3 => "ANGULAR",
                                        4 => "DIAMETER",
                                        5 => "RADIUS",
                                        _ => "LINEAR"
                                    };
                                }
                                break;
                        }
                        i++;
                    }

                    // Add to lists
                    switch (dimType)
                    {
                        case "LINEAR":
                        case "ALIGNED":
                            linearDimensionsList.Add(new LinearDimensionData
                            {
                                startX = x1,
                                startY = y1,
                                startZ = z1,
                                endX = x2,
                                endY = y2,
                                endZ = z2,
                                textX = textX,
                                textY = textY,
                                textHeight = textHeight,
                                rotationAngle = rotationAngle,
                                dimColor = dimColor,
                                textColor = textColor,
                                linetype = linetype,
                                text = string.IsNullOrEmpty(text) ? "DIM" : text
                            });
                            linesList.Add((x1, y1, x2, y2, dimColor, linetype));
                            textsList.Add((textX, textY, string.IsNullOrEmpty(text) ? "DIM" : text, textHeight, rotationAngle, dimColor));
                            break;

                        case "RADIUS":
                            radialDimensionsList.Add(new RadialDimensionData
                            {
                                centerX = x1,
                                centerY = y1,
                                centerZ = z1,
                                radius = Math.Sqrt(Math.Pow(x2 - x1, 2) + Math.Pow(y2 - y1, 2)),
                                textX = textX,
                                textY = textY,
                                textHeight = textHeight,
                                rotationAngle = rotationAngle,
                                dimColor = dimColor,
                                textColor = textColor,
                                linetype = linetype,
                                text = string.IsNullOrEmpty(text) ? "DIM" : text
                            });
                            break;

                        case "DIAMETER":
                            diameterDimensionsList.Add(new DiameterDimensionData
                            {
                                centerX = x1,
                                centerY = y1,
                                centerZ = z1,
                                diameter = Math.Sqrt(Math.Pow(x2 - x1, 2) + Math.Pow(y2 - y1, 2)) * 2,
                                textX = textX,
                                textY = textY,
                                textHeight = textHeight,
                                rotationAngle = rotationAngle,
                                dimColor = dimColor,
                                textColor = textColor,
                                linetype = linetype,
                                text = string.IsNullOrEmpty(text) ? "DIM" : text
                            });
                            break;

                        case "ANGULAR":
                            angularDimensionsList.Add(new AngularDimensionData
                            {
                                vertexX = x1,
                                vertexY = y1,
                                vertexZ = z1,
                                line1X = x2,
                                line1Y = y2,
                                line1Z = z2,
                                line2X = x3,
                                line2Y = y3,
                                line2Z = z3,
                                textX = textX,
                                textY = textY,
                                textHeight = textHeight,
                                rotationAngle = rotationAngle,
                                dimColor = dimColor,
                                textColor = textColor,
                                linetype = linetype,
                                text = string.IsNullOrEmpty(text) ? "DIM" : text
                            });
                            break;
                    }
                }

                // --- HATCH entity handling ---
                if (entity == "HATCH")
                {
                    string patternName = "SOLID"; // default pattern
                    int localColor = 7;           // default color
                    bool isSolid = false;         // default fill
                    var loops = new List<List<(double x, double y)>>(); // all loops of this hatch
                    List<(double x, double y)> currentLoop = null;

                    i++; // move to next pair after HATCH
                    while (i < pairs.Count)
                    {
                        (code, value) = pairs[i];

                        if (code == "0")
                            break; // end of HATCH

                        switch (code)
                        {
                            case "2": // Hatch pattern name
                                patternName = value.Trim();
                                break;

                            case "62": // Color number
                                int.TryParse(value, out localColor);
                                break;

                            case "70": // Hatch flags
                                if (int.TryParse(value, out int flag))
                                    isSolid = (flag & 1) != 0; // bit 1 = solid fill
                                break;

                            case "91": // Number of loops
                                       // Optional: can pre-allocate loop list
                                loops = new List<List<(double x, double y)>>(int.Parse(value));
                                break;

                            case "92": // Loop type
                                       // 1 = polyline, 2 = external, etc.
                                       // Start a new loop
                                currentLoop = new List<(double x, double y)>();
                                loops.Add(currentLoop);
                                break;

                            case "10": // X coordinate
                                double.TryParse(value, NumberStyles.Float, CultureInfo.InvariantCulture, out double x);
                                if (i + 1 < pairs.Count && pairs[i + 1].Item1 == "20")
                                {
                                    double.TryParse(pairs[i + 1].Item2, NumberStyles.Float, CultureInfo.InvariantCulture, out double y);
                                    if (currentLoop == null)
                                    {
                                        // If no loop started, create default loop
                                        currentLoop = new List<(double x, double y)>();
                                        loops.Add(currentLoop);
                                    }
                                    currentLoop.Add((x, y));
                                    i++; // skip the Y coordinate pair
                                }
                                break;

                                // Optional: handle bulges or arcs if needed (codes 42,43)
                                // case "42": // start bulge
                                // case "43": // end bulge
                        }

                        i++;
                    }

                    // Flatten loops for HatchDrawer
                    foreach (var loop in loops)
                    {
                        if (loop.Count >= 3) // need at least 3 points to make a face
                        {
                            hatchList.Add((loop, localColor, patternName, isSolid));
                        }
                    }

                    i--; // Adjust index so outer loop continues correctly
                    continue;
                }



                // --- normal entity loop ---
                for (++i; i < pairs.Count; i++)
                {
                    var (c, v) = pairs[i];
                    if (c == "0") { i--; break; }

                    double dval = 0; int ival = 0;
                    double.TryParse(v, NumberStyles.Float, CultureInfo.InvariantCulture, out dval);
                    int.TryParse(v, out ival);

                    switch (entity)
                    {
                        case "SOLID":
                            if (c == "10") x1 = dval;
                            else if (c == "20") y1 = dval;
                            else if (c == "11") x2 = dval;
                            else if (c == "21") y2 = dval;
                            else if (c == "12") x3 = dval;
                            else if (c == "22") y3 = dval;
                            else if (c == "13") x4 = dval;
                            else if (c == "23") y4 = dval;
                            else if (c == "62") color = ival;
                            break;

                        case "ARC":
                            if (c == "10") cx = dval;
                            else if (c == "20") cy = dval;
                            else if (c == "40") r = dval;
                            else if (c == "50") startAngle = dval;
                            else if (c == "51") endAngle = dval;
                            else if (c == "62") color = ival;
                            else if (c == "6") linetype = v;
                            break;

                        case "POINT":
                            if (c == "10") px = dval;
                            else if (c == "20") py = dval;
                            else if (c == "62") color = ival;
                            break;

                        case "LINE":
                            if (c == "10") x1 = dval;
                            else if (c == "20") y1 = dval;
                            else if (c == "11") x2 = dval;
                            else if (c == "21") y2 = dval;
                            else if (c == "62") color = ival;
                            else if (c == "6") linetype = v;
                            break;

                        case "CIRCLE":
                            if (c == "10") cx = dval;
                            else if (c == "20") cy = dval;
                            else if (c == "40") r = dval;
                            else if (c == "62") color = ival;
                            else if (c == "6") linetype = v;
                            break;

                        case "TEXT":
                            if (c == "10") tx = dval;
                            else if (c == "20") ty = dval;
                            else if (c == "1") textValue = v;
                            else if (c == "40") height = dval;
                            else if (c == "50") rotation = dval;
                            else if (c == "62") color = ival;
                            break;

                        case "BYBLOCK":
                            if (c == "10") x1 = dval;
                            else if (c == "20") y1 = dval;
                            else if (c == "11") x2 = dval;
                            else if (c == "21") y2 = dval;
                            else if (c == "62") color = ival;
                            break;

                        case "VERTEX":
                            if (c == "10") x1 = dval;
                            else if (c == "20") y1 = dval;
                            else if (c == "30") pz = dval;
                            else if (c == "62") color = ival;
                            break;
                    }
                }

                // finalize
                switch (entity)
                {
                    case "SOLID":
                        solidsList.Add((x1, y1, x2, y2, x3, y3, x4, y4, color));
                        break;
                    case "ARC":
                        arcsList.Add((cx, cy, r, startAngle, endAngle, color, linetype));
                        break;
                    case "POINT":
                        pointsList.Add((px, py, color));
                        break;
                    case "LINE":
                        linesList.Add((x1, y1, x2, y2, color, linetype));
                        break;
                    case "CIRCLE":
                        circlesList.Add((cx, cy, r, color, linetype));
                        break;
                    case "TEXT":
                        textsList.Add((tx, ty, textValue, height, rotation, color));
                        break;
                    case "BYBLOCK":
                        byBlocksList.Add((x1, y1, x2, y2, color));
                        break;
                    case "VERTEX":
                        verticesList.Add((x1, y1, pz, color));
                        break;
                }
            }

            return (
                lwpolylinesList,
                hatchList,
                linearDimensionsList,
                angularDimensionsList,
                radialDimensionsList,
                diameterDimensionsList,
                ellipsesList,
                splinesList,
                polylinesList,
                verticesList,
                byBlocksList,
                solidsList,
                arcsList,
                pointsList,
                linesList,
                circlesList,
                textsList);
        }


        // ============================================================
        #region --- Dimension Builder ---
        // ============================================================
        public static class DimensionBuilder
        {
            public static List<DimensionItem> BuildAllDimensions(
                List<LinearDimensionData> linearList,
                List<RadialDimensionData> radialList,
                List<DiameterDimensionData> diameterList,
                List<AngularDimensionData> angularList)
            {
                var result = new List<DimensionItem>(linearList.Count + radialList.Count + diameterList.Count + angularList.Count);

                // Linear
                result.AddRange(linearList.Select(l => new DimensionItem
                {
                    StartX = l.startX,
                    StartY = l.startY,
                    EndX = l.endX,
                    EndY = l.endY,
                    Text = "Linear",
                    Type = "Linear"
                }));

                // Radial
                result.AddRange(radialList.Select(r => new DimensionItem
                {
                    StartX = r.centerX,
                    StartY = r.centerY,
                    LeaderX = r.centerX + r.radius,
                    LeaderY = r.centerY,
                    Text = "R",
                    Type = "Radial"
                }));

                // Diameter
                result.AddRange(diameterList.Select(d => new DimensionItem
                {
                    StartX = d.centerX - d.diameter / 2,
                    EndX = d.centerX + d.diameter / 2,
                    StartY = d.centerY,
                    EndY = d.centerY,
                    Text = "Φ",
                    Type = "Diameter"
                }));

                // Angular
                result.AddRange(angularList.Select(a => new DimensionItem
                {
                    StartX = a.vertexX,
                    StartY = a.vertexY,
                    LeaderX = a.line1X,
                    LeaderY = a.line1Y,
                    Text = "∠",
                    Type = "Angular"
                }));

                return result;
            }
        }
        #endregion


        // ✅ Load DXF and draw
        public async Task LoadDxfAsync(string filePath)
        {
            if (viewer == null) return; // viewer not ready yet

            if (string.IsNullOrEmpty(filePath) || !File.Exists(filePath)) return;

            try
            {
                // parse on background thread
                var (
                    lwpolylinesList,
                    hatchList,
                    linearDimensionsList,
                    angularDimensionsList,
                    radialDimensionsList,
                    diameterDimensionsList,
                    ellipsesList,
                    splinesList,
                    polylinesList,
                    verticesList,
                    byBlocksList,
                    solidsList,
                    arcsList,
                    pointsList,
                    linesList,
                    circlesList,
                    textsListRaw) = await Task.Run(() => ParseDxfEntities(filePath));

                // make sure viewer is present
                if (viewer == null || viewer.NativeHandle == IntPtr.Zero) return;

                // Clear existing shapes
                if (viewer?.NativeHandle != IntPtr.Zero)
                    ClearAllShapes(viewer.NativeHandle);

                _dxfShapeDict.Clear(); // <— clear previous IDs

                // Batch and draw ByBlock entities
                if (byBlocksList?.Count > 0)
                {
                    await DrawEntitiesBatch(byBlocksList, async (batch) =>
                    {
                        var byBlockBatch = batch.Select(b => (b.x1, b.y1, 0.0, b.x2, b.y2, 0.0, b.color)).ToList();
                        await ByBlock(viewer, byBlockBatch);
                    });
                }

                // Batch and draw polylines
                if (verticesList?.Count > 0)
                {
                    await DrawEntitiesBatch(verticesList, async (batch) =>
                    {
                        var vertexBatch = batch.Select(v => (v.x, v.y, 0.0, 0.0, 0.0, 0.0, v.color)).ToList();
                        await Vertex(viewer, vertexBatch);  // Replace with actual method for drawing vertices
                    });

                }

                foreach (var (vertices, closed) in polylinesList)
                {
                    await Polyline(viewer, vertices, closed);
                }

                // --- Draw LWPOLYLINEs ---
                if (lwpolylinesList?.Count > 0)
                {
                    foreach (var (vertices, closed, color) in lwpolylinesList)
                    {
                        // Merge outer color into vertices if the inner color is not already set
                        var lwVerticesWithColor = vertices
                            .Select(v => (v.x, v.y, 0.0, v.bulge, color))
                            .ToList();

                        await LwPolyline(viewer, lwVerticesWithColor, closed);
                    }
                }

                // Batch and draw solids
                if (solidsList?.Count > 0)
                {
                    await DrawEntitiesBatch(solidsList, async (batch) =>
                    {
                        await Solid(viewer, batch);
                    });
                }

                // Batch and draw arcs
                if (arcsList?.Count > 0)
                {
                    await DrawEntitiesBatch(arcsList, async (batch) =>
                    {
                        await Arc(viewer, batch);
                    });
                }

                // Batch and draw points
                if (pointsList?.Count > 0)
                {
                    await DrawEntitiesBatch(pointsList, async (batch) =>
                    {
                        await Point(viewer, batch);
                    });
                }

                // Batch and draw lines
                if (linesList?.Count > 0)
                {
                    await DrawEntitiesBatch(linesList, async (batch) =>
                    {
                        await Line(viewer, batch);
                    });
                }

                // Batch and draw circles
                if (circlesList?.Count > 0)
                {
                    await DrawEntitiesBatch(circlesList, async (batch) =>
                    {
                        await Circle(viewer, batch);
                    });
                }

                // --- Batch and Draw Ellipses ---
                if (ellipsesList?.Count > 0)
                {
                    await DrawEntitiesBatch(ellipsesList, async (batch) =>
                    {
                        await Ellipse(viewer, batch);
                    });
                }

                // --- Draw SPLINES ---
                if (splinesList?.Count > 0)
                {
                    await DrawEntitiesBatch(splinesList, async (batch) =>
                    {
                        // Handle drawing of splines here
                        foreach (var (controlPoints, color, degree) in batch)
                        {
                            // Assuming `DrawSpline` is a method to draw splines that takes control points, color, and degree
                            await Spline(viewer, controlPoints, color, degree);
                        }
                    });
                }

                // --- Draw HATCH entities ---
                //if (hatchList?.Count > 0)
                //{
                //    await DrawEntitiesBatch(hatchList, async (batch) =>
                //    {
                //        foreach (var (boundaryPoints, color, patternName, isSolid) in batch)
                //        {
                //            await Hatch(viewer, new List<List<(double x, double y)>> { boundaryPoints }, color, patternName, isSolid);
                //        }

                //    });
                //}


                // --- Draw Dimensions ---
                var allDimensions = DimensionBuilder.BuildAllDimensions(linearDimensionsList, radialDimensionsList, diameterDimensionsList, angularDimensionsList);
                foreach (var dim in allDimensions)
                    await Dimension(viewer, new List<DimensionItem> { dim });



                // --- Batch Draw ---
                if (allDimensions.Count > 0)
                {
                    await DrawEntitiesBatch(allDimensions, async (batch) =>
                    {
                        foreach (var dim in batch)
                        {
                            await Dimension(viewer, new List<DimensionItem> { dim });
                        }
                    });
                }

                // --- Prepare Texts for DrawTextBatch ---
                if (textsListRaw?.Count > 0 && viewer?.NativeHandle != IntPtr.Zero)
                {
                    await DrawEntitiesBatch(textsListRaw, async (batch) =>
                    {
                        await Text(viewer, batch);
                    });
                }

                await Attach(HostPanel, viewer);

                if (viewer?.ViewPtr != IntPtr.Zero)
                {
                    // Make sure FitAll runs on UI thread and after host layout
                    await Application.Current.Dispatcher.InvokeAsync(async () =>
                    {
                        // ✅ Use HostPanel dimensions
                        int width = Math.Max(HostPanel.Width, 1);
                        int height = Math.Max(HostPanel.Height, 1);

                        ResizeViewer(viewer.NativeHandle, width, height);

                        // ✅ reset camera to +Z, no twist, fit all
                        PotaOCC.ViewHelper.ResetView(viewer.ViewPtr);

                        SetShaded(viewer.NativeHandle);
                    });
                }
            }
            finally
            {
                await Application.Current.Dispatcher.InvokeAsync(() =>
                {
                    loadingOverlay.Visible = false;
                    ShapeDrawer.ResetView(viewer.NativeHandle);
                });
            }
        }
    }

}