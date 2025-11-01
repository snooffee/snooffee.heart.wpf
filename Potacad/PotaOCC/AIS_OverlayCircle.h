#include <AIS_InteractiveObject.hxx>
#include <Prs3d_Presentation.hxx>
#include <PrsMgr_PresentationManager3d.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_Group.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Quantity_Color.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <Prs3d_Root.hxx>
#include <SelectMgr_Selection.hxx>
#include <SelectMgr_EntityOwner.hxx>
#include <Select3D_SensitiveCircle.hxx>
#include <gp_Circ.hxx>
#include <gp_Ax2.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <cmath>

class AIS_OverlayCircle : public AIS_InteractiveObject
{
public:
    AIS_OverlayCircle()
        : myCenter(0, 0, 0), myRadius(0.0)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }

    void SetCircle(const gp_Pnt& center, double radius)
    {
        myCenter = center;
        myRadius = radius;
        this->Redisplay(Standard_True);
    }

    virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& thePM,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer theMode) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        const int numSegments = 64;  // fewer segments for simplicity
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(2 * numSegments);  // 2 vertices per segment

        // Loop through each segment and calculate its start and end points
        for (int i = 0; i < numSegments; ++i)
        {
            double ang1 = (2.0 * M_PI * i) / numSegments;
            double ang2 = (2.0 * M_PI * (i + 1)) / numSegments;

            double x1 = myCenter.X() + myRadius * cos(ang1);
            double y1 = myCenter.Y() + myRadius * sin(ang1);
            double x2 = myCenter.X() + myRadius * cos(ang2);
            double y2 = myCenter.Y() + myRadius * sin(ang2);

            // Add the segment between the two calculated points, cast the coordinates to Standard_ShortReal
            segs->AddVertex(Standard_ShortReal(x1), Standard_ShortReal(y1), Standard_ShortReal(myCenter.Z()));
            segs->AddVertex(Standard_ShortReal(x2), Standard_ShortReal(y2), Standard_ShortReal(myCenter.Z()));
        }

        Quantity_Color colLine(Quantity_NOC_RED);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);

        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer theMode) override
    {
        gp_Circ circ(gp_Ax2(myCenter, gp_Dir(0, 0, 1)), myRadius);
        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
        Handle(Select3D_SensitiveCircle) sc = new Select3D_SensitiveCircle(owner, circ, Standard_False);
        theSelection->Add(sc);
    }

private:
    gp_Pnt myCenter;
    double myRadius;
};