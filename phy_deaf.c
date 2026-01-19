/*
 * Copyright 2021 Morse Micro
 */

#include <stdio.h>
#include "command.h"
#include "utilities.h"

struct PACKED command_phy_deaf
{
    /* 0 sets PHY into normal operation, 1 sets PHY into deaf/blocked mode */
    uint8_t enable;
};

static void usage(struct morsectrl *mors) {
    mctrl_print("\tphy_deaf <command>\n");
    mctrl_print("\t\tenable\tput the phy in a deaf/blocked mode, it will not be able"
           " to receive or schedule transmission from the mac\n");
    mctrl_print("\t\tdisable\treturn to normal operation\n");
}

int phy_deaf(struct morsectrl *mors, int argc, char *argv[])
{
    int ret  = -1;
    uint8_t phy_deaf;
    struct command_phy_deaf *cmd;
    struct morsectrl_transport_buff *cmd_tbuff;
    struct morsectrl_transport_buff *rsp_tbuff;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    if (argc != 2)
    {
        mctrl_err("Invalid command parameters\n");
        usage(mors);
        return -1;
    }

    phy_deaf = expression_to_int(argv[1]);
    if (phy_deaf == -1)
    {
        mctrl_err("Invalid command parameters\n");
        usage(mors);
        return -1;
    }

    cmd_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, sizeof(*cmd));
    rsp_tbuff = morsectrl_transport_resp_alloc(&mors->transport, 0);

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    cmd = TBUFF_TO_CMD(cmd_tbuff, struct command_phy_deaf);
    cmd->enable = phy_deaf;
    ret = morsectrl_send_command(&mors->transport, MORSE_TEST_COMMAND_PHY_DEAF,
                                 cmd_tbuff, rsp_tbuff);
exit:
    if (ret < 0)
    {
        mctrl_err("Failed to set phy_deaf mode\n");
    }

    if (cmd_tbuff)
        morsectrl_transport_buff_free(cmd_tbuff);
    if (rsp_tbuff)
        morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}
