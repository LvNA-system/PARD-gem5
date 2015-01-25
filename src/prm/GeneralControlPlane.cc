#include "prm/GeneralControlPlane.hh"

GeneralControlPlane::GeneralControlPlane(Params *p)
    : ControlPlane(p)
{
}

GeneralControlPlane::~GeneralControlPlane()
{
}

GeneralControlPlane *
GeneralControlPlaneParams::create()
{
    return new GeneralControlPlane(this);
}

