#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager3d.hxx>
#include <Graphic3d_ArrayOfSegments.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_Root.hxx>
#include <SelectMgr_Selection.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <gp_Elips.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <cmath>
#include <gp_Circ.hxx>
#include <Select3D_SensitiveEntity.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <Select3D_SensitiveBox.hxx>
#include <TopoDS_Edge.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>

class AIS_OverlayEllipse : public AIS_InteractiveObject
{
public:
    AIS_OverlayEllipse()
        : myCenter(0, 0, 0), myRadiusX(0.0), myRadiusY(0.0)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    void SetEllipse(const gp_Pnt& center, double radiusX, double radiusY)
    {
        myCenter = center;
        myRadiusX = radiusX;
        myRadiusY = radiusY;
        this->Redisplay(Standard_True);
    }

    virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& thePM,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer theMode) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        const int numSegments = 64;
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(2 * numSegments);  // 2 vertices per segment

        // Loop through each segment and calculate its start and end points
        for (int i = 0; i < numSegments; ++i)
        {
            double ang1 = (2.0 * M_PI * i) / numSegments;
            double ang2 = (2.0 * M_PI * (i + 1)) / numSegments;

            double x1 = myCenter.X() + myRadiusX * cos(ang1);
            double y1 = myCenter.Y() + myRadiusY * sin(ang1);
            double x2 = myCenter.X() + myRadiusX * cos(ang2);
            double y2 = myCenter.Y() + myRadiusY * sin(ang2);

            // Add the segment between the two calculated points, cast the coordinates to Standard_ShortReal
            segs->AddVertex(Standard_ShortReal(x1), Standard_ShortReal(y1), Standard_ShortReal(myCenter.Z()));
            segs->AddVertex(Standard_ShortReal(x2), Standard_ShortReal(y2), Standard_ShortReal(myCenter.Z()));
        }

        Quantity_Color colLine(Quantity_NOC_GREEN);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);

        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer theMode) override
    {
        // Create the ellipse geometry using the center, and two radii
        gp_Elips ellipse(gp_Ax2(myCenter, gp_Dir(0, 0, 1)), myRadiusX, myRadiusY);

        // Convert the gp_Elips to a TopoDS_Edge
        gp_Circ circle(gp_Ax2(myCenter, gp_Dir(0, 0, 1)), myRadiusX);
        BRepBuilderAPI_MakeEdge edgeMaker(circle);
        TopoDS_Edge edge = edgeMaker.Edge();

        // Compute the bounding box for the edge (approximated ellipse as a circle)
        Bnd_Box boundingBox;
        BRepBndLib::Add(edge, boundingBox);

        // Create a sensitive entity from the bounding box (for selection purposes)
        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
        Handle(Select3D_SensitiveBox) sensitiveBox = new Select3D_SensitiveBox(owner, boundingBox);

        // Add the sensitive entity (the bounding box) to the selection manager
        theSelection->Add(sensitiveBox);
    }



private:
    gp_Pnt myCenter;   // Center point of the ellipse
    double myRadiusX;  // Horizontal radius (X-axis)
    double myRadiusY;  // Vertical radius (Y-axis)
};