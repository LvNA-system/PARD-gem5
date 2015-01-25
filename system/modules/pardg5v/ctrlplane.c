#include "cpa.h"
#include "ctrlplane.h"

//extern struct control_plane_op cache_cp_op;
//extern struct control_plane_op memcntrl_cp_op;
extern struct control_plane_op sys_cp_op;

struct control_plane_op *control_plane_tbl [] = {
    //&cache_cp_op, &memcntrl_cp_op,
    &sys_cp_op,
    NULL,
};

struct control_plane_op * get_control_plane_op(char type)
{
    for (struct control_plane_op **cpop = control_plane_tbl;
         *cpop != NULL; cpop++)
    {
        if ((*cpop)->type == type)
            return *cpop;
    }
    return NULL;
}

