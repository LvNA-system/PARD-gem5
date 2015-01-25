#include "prm/CPConnector.hh"
#include "prm/ControlPlane.hh"

ControlPlane::ControlPlane(const Params *p)
    : AbstractControlPlane(p), connector(p->connector)
{
    connector->registerControlPlane(this);
}

ControlPlane::~ControlPlane()
{
}

void
ControlPlane::registerCommandHandler(ICommandHandler *handler)
{
    connector->registerCommandHandler(handler);
}

