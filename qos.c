/*
 * Copyright 2020 Morse Micro
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "portable_endian.h"

#include "command.h"

struct PACKED command_qos_params_req
{
    /** index of the QoS queue whose values will be changed */
    uint8_t queue_idx;

    /** How many slots to wait for AIFS */
    uint8_t aifs_slot_count;

    /** Contention window min/max values */
    uint16_t contention_window_min;
    uint16_t contention_window_max;

    /** Maximum possible TX OP in us */
    uint32_t max_txop_us;
};

struct PACKED command_qos_params_cfm
{
    /** How many slots to wait for AIFS */
    uint8_t aifs_slot_count;

    /** Contention window min/max values */
    uint16_t contention_window_min;
    uint16_t contention_window_max;

    /** Maximum possible TX OP in us */
    uint32_t max_txop_us;
};

static void usage(struct morsectrl *mors) {
    mctrl_print("\tqos [options] <queue_ID>\n");
    mctrl_print("\t\t\t\tsets/reads QOS parameters for given queue ID\n");
    mctrl_print("\t\t-c  <value>\tnumber of AIFS slots to wait for\n");
    mctrl_print("\t\t-t  <value>\tmaximum possible TX OP in us\n");
    mctrl_print("\t\t-m  <min> <max>\tcontention window MIN,MAX values\n");
}

int qos(struct morsectrl *mors, int argc, char *argv[])
{
    int ret = -1;
    int option;
    struct command_qos_params_req *cmd;
    struct command_qos_params_cfm *resp;
    struct morsectrl_transport_buff *cmd_tbuff;
    struct morsectrl_transport_buff *rsp_tbuff;
    uint8_t set = 0;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    if (argc < 2)
    {
        mctrl_err("Too few arguments\n");
        usage(mors);
        return -1;
    }

    cmd_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, sizeof(*cmd));
    rsp_tbuff = morsectrl_transport_resp_alloc(&mors->transport, sizeof(*resp));

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    cmd = TBUFF_TO_CMD(cmd_tbuff, struct command_qos_params_req);
    resp = TBUFF_TO_RSP(rsp_tbuff, struct command_qos_params_cfm);

    cmd->queue_idx = -1;
    cmd->aifs_slot_count = -1;
    cmd->contention_window_min = -1;
    cmd->contention_window_max = -1;
    cmd->max_txop_us = -1;

    while ((option = getopt(argc, argv, "c:t:m:")) != -1)
    {
        switch (option)
        {
            case 'c' :
                cmd->aifs_slot_count = htole32(atoi(optarg));
                set = 1;
                break;
            case 't' :
                cmd->max_txop_us = htole32(atoi(optarg));
                set = 1;
                break;
            case 'm' :
                cmd->contention_window_min = htole32(atoi(optarg));
                cmd->contention_window_max = (argv[optind] == NULL) ?
                                    cmd->contention_window_max : htole32(atoi(argv[optind]));
                if (((cmd->contention_window_max == 0) && (argv[optind][0] != '0'))
                                                || (cmd->contention_window_max == (uint16_t)(-1)))
                {
                    mctrl_err("Invalid maximum value\n");
                    usage(mors);
                    ret = -1;
                    goto exit;
                }
                optind++;
                if (cmd->contention_window_min > cmd->contention_window_max)
                {
                    mctrl_err("Min should never exceed Max\n");
                    ret = -1;
                    goto exit;
                }
                set = 1;
                break;
            case '?' :
                usage(mors);
                ret = -1;
                goto exit;
            default :
                mctrl_err("Invalid argument\n");
                usage(mors);
                ret = -1;
                goto exit;
        }
    }
    if (optind + 1 != argc)
    {
        mctrl_err("Invalid arguments\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    cmd->queue_idx = (argv[optind] == NULL) ? cmd->queue_idx : htole32(atoi(argv[optind]));
    if (((cmd->queue_idx == 0) && (argv[optind][0] != '0')) || (cmd->queue_idx == (uint8_t)(-1)))
    {
        mctrl_err("Invalid queue ID\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    if (set)
    {
        ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_SET_QOS_PARAMS,
                                     cmd_tbuff, rsp_tbuff);

        if (ret < 0)
        {
            mctrl_err("Command set qos error (%d)\n", ret);
            goto exit;
        }
    }

    ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_GET_QOS_PARAMS,
                                 cmd_tbuff, rsp_tbuff);

exit:
    if (!ret)
    {
        mctrl_print("QoS (min): %d\t(max): %d\n",
               (uint16_t)resp->contention_window_min, (uint16_t)resp->contention_window_max);
        mctrl_print("AIFS count: %d\n", (uint8_t)resp->aifs_slot_count);
        mctrl_print("Max TX OP (us): %d\n", (uint32_t)resp->max_txop_us);
    }
    else
    {
        mctrl_err("Command get qos error (%d)\n", ret);
    }

    morsectrl_transport_buff_free(cmd_tbuff);
    morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}
