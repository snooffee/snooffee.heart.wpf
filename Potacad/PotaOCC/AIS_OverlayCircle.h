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
        : myRadius(0.0), myIsPlane(false)
    {
        this->SetZLayer(Graphic3d_ZLayerId_TopOSD);
        Handle(Graphic3d_TransformPers) trpers = new Graphic3d_TransformPers(Graphic3d_TMF_2d);
        this->SetTransformPersistence(trpers);
    }
    void SetCircle(const gp_Ax2& axis, double radius)
    {
        myAxis = axis;
        myRadius = radius;
        myIsPlane = true;
        this->Redisplay(Standard_True);
    }
    void SetCircle(const gp_Pnt& center, double radius)
    {
        myCenter = center;
        myRadius = radius;
        myIsPlane = false;
        this->Redisplay(Standard_True);
    }
    virtual void Compute(const Handle(PrsMgr_PresentationManager3d)& thePM,
        const Handle(Prs3d_Presentation)& thePresentation,
        const Standard_Integer theMode) override
    {
        Handle(Graphic3d_Group) aGroup = Prs3d_Root::CurrentGroup(thePresentation);

        const int numSegments = 64;
        Handle(Graphic3d_ArrayOfSegments) segs = new Graphic3d_ArrayOfSegments(2 * numSegments);

        if (myIsPlane)
        {
            gp_Pnt center = myAxis.Location();
            gp_Dir normal = myAxis.Direction();
            gp_Dir xDir = myAxis.XDirection();
            gp_Dir yDir = normal.Crossed(xDir);

            for (int i = 0; i < numSegments; ++i)
            {
                double ang1 = 2.0 * M_PI * i / numSegments;
                double ang2 = 2.0 * M_PI * (i + 1) / numSegments;

                gp_Pnt p1 = center.Translated(xDir.XYZ() * myRadius * cos(ang1) + yDir.XYZ() * myRadius * sin(ang1));
                gp_Pnt p2 = center.Translated(xDir.XYZ() * myRadius * cos(ang2) + yDir.XYZ() * myRadius * sin(ang2));

                segs->AddVertex(Standard_ShortReal(p1.X()), Standard_ShortReal(p1.Y()), Standard_ShortReal(p1.Z()));
                segs->AddVertex(Standard_ShortReal(p2.X()), Standard_ShortReal(p2.Y()), Standard_ShortReal(p2.Z()));
            }
        }
        else
        {
            gp_Pnt center = myCenter;
            for (int i = 0; i < numSegments; ++i)
            {
                double ang1 = 2.0 * M_PI * i / numSegments;
                double ang2 = 2.0 * M_PI * (i + 1) / numSegments;

                gp_Pnt p1(center.X() + myRadius * cos(ang1),
                    center.Y() + myRadius * sin(ang1),
                    center.Z());
                gp_Pnt p2(center.X() + myRadius * cos(ang2),
                    center.Y() + myRadius * sin(ang2),
                    center.Z());

                segs->AddVertex(Standard_ShortReal(p1.X()), Standard_ShortReal(p1.Y()), Standard_ShortReal(p1.Z()));
                segs->AddVertex(Standard_ShortReal(p2.X()), Standard_ShortReal(p2.Y()), Standard_ShortReal(p2.Z()));
            }
        }

        Quantity_Color colLine(Quantity_NOC_RED);
        Handle(Graphic3d_AspectLine3d) asp = new Graphic3d_AspectLine3d(colLine, Aspect_TOL_SOLID, 1.0f);
        aGroup->SetPrimitivesAspect(asp);
        aGroup->AddPrimitiveArray(segs);
    }

    virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSelection,
        const Standard_Integer theMode) override
    {
        gp_Circ circ = myIsPlane ? gp_Circ(myAxis, myRadius)
            : gp_Circ(gp_Ax2(myCenter, gp_Dir(0, 0, 1)), myRadius);

        Handle(SelectMgr_EntityOwner) owner = new SelectMgr_EntityOwner(this);
        Handle(Select3D_SensitiveCircle) sc = new Select3D_SensitiveCircle(owner, circ, Standard_False);
        theSelection->Add(sc);
    }

private:
    gp_Ax2 myAxis;
    gp_Pnt myCenter;
    double myRadius;
    bool myIsPlane;  // true if using gp_Ax2, false if screen plane
};