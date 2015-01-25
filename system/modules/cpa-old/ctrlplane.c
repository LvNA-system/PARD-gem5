#include "cpa.h"
#include "ctrlplane.h"

extern struct control_plane_op cache_cp_op;
extern struct control_plane_op memcntrl_cp_op;

struct control_plane_op *control_plane_tbl [] = {
    &cache_cp_op, &memcntrl_cp_op,
    NULL,
};
