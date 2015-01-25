#ifndef __PRM_INTERFACES_HH__
#define __PRM_INTERFACES_HH__

class ICommandHandler
{
  public:
    virtual bool handleCommand(int cmd,
        uint64_t arg1, uint64_t arg2, uint64_t arg3) = 0;
};

#endif	// __PRM_INTERFACES_HH__
