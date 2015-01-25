#include "cpa.h"

int cpn_bus_read_config_qword(struct control_plane_bus *bus,
                              struct control_plane_adaptor *adaptor,
                              uint16_t cpid, uint16_t ldom, uint32_t addr,
                              uint64_t *value)
{
    spin_lock(&adaptor->lock);
    // select CP
    cpa_writel(0, adaptor->cmd_virtual, cpid);
    // send command
    cpa_writew(  CP_LDOMID_OFFSET, adaptor->data_virtual, ldom);
    cpa_writel(CP_DESTADDR_OFFSET, adaptor->data_virtual, addr);
    cpa_writeb(     CP_CMD_OFFSET, adaptor->data_virtual, CP_CMD_GETENTRY);
    // get result
    *value = cpa_readq(CP_DATA_OFFSET, adaptor->data_virtual);
    spin_unlock(&adaptor->lock);

    return 0;
}

int cpn_bus_write_config_qword(struct control_plane_bus *bus,
                              struct control_plane_adaptor *adaptor,
                              uint16_t cpid, uint16_t ldom, uint32_t addr,
                              uint64_t value)
{
    spin_lock(&adaptor->lock);
    // select CP
    cpa_writel(0, adaptor->cmd_virtual, cpid);
    // send command
    cpa_writew(  CP_LDOMID_OFFSET, adaptor->data_virtual, ldom);
    cpa_writel(CP_DESTADDR_OFFSET, adaptor->data_virtual, addr);
    cpa_writeq(    CP_DATA_OFFSET, adaptor->data_virtual, value);
    cpa_writeb(     CP_CMD_OFFSET, adaptor->data_virtual, CP_CMD_SETENTRY);
    spin_unlock(&adaptor->lock);

    return 0;
}

int cpn_bus_write_command(struct control_plane_bus *bus,
                          struct control_plane_adaptor *adaptor,
                          uint8_t cmd,
                          uint16_t cpid, uint16_t ldom, uint32_t addr,
                          uint64_t value)
{
    spin_lock(&adaptor->lock);
    // select CP
    cpa_writel(0, adaptor->cmd_virtual, cpid);
    // send command
    cpa_writew(  CP_LDOMID_OFFSET, adaptor->data_virtual, ldom);
    cpa_writel(CP_DESTADDR_OFFSET, adaptor->data_virtual, addr);
    cpa_writeq(    CP_DATA_OFFSET, adaptor->data_virtual, value);
    cpa_writeb(     CP_CMD_OFFSET, adaptor->data_virtual, cmd);
    spin_unlock(&adaptor->lock);

    return 0;
}

EXPORT_SYMBOL(cpn_bus_read_config_qword);
EXPORT_SYMBOL(cpn_bus_write_config_qword);
EXPORT_SYMBOL(cpn_bus_write_command);
