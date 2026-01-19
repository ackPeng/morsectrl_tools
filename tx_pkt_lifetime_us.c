/*
 * Copyright 2022 Morse Micro
 */

#include <stdlib.h>
#include <stdio.h>

#include "command.h"
#include "utilities.h"

struct PACKED set_tx_pkt_lifetime_us_command
{
    uint32_t lifetime_us;
};

static void usage(struct morsectrl *mors) {
    mctrl_print(
        "\ttx_pkt_lifetime_us <value>\t\tset Tx-pkt lifetime expiry within 50000-500000us\n");
}

int tx_pkt_lifetime_us(struct morsectrl *mors, int argc, char *argv[])
{
    int ret = -1;
    struct set_tx_pkt_lifetime_us_command *cmd;
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

    cmd_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, sizeof(*cmd));
    rsp_tbuff = morsectrl_transport_resp_alloc(&mors->transport, 0);

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    cmd = TBUFF_TO_CMD(cmd_tbuff, struct set_tx_pkt_lifetime_us_command);

    if (check_string_is_int(argv[1]))
    {
        cmd->lifetime_us = atoi(argv[1]);
    }
    else
    {
        mctrl_err("Invalid value [50000 to 500000] us\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    if (cmd->lifetime_us < 50000 || cmd->lifetime_us > 500000)
    {
        mctrl_err("Invalid value [50000 to 500000] us\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_SET_TX_PKT_LIFETIME_US,
                                 cmd_tbuff, rsp_tbuff);

exit:
    if (ret < 0)
    {
        mctrl_err("Failed to set tx pkt lifetime\n");
    }
    else
    {
        mctrl_print("\t Tx-pkt lifetime expriy is set : %d us\n", cmd->lifetime_us);
    }

    morsectrl_transport_buff_free(cmd_tbuff);
    morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}
