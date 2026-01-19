/*
 * Copyright 2022 Morse Micro
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include "portable_endian.h"

#include "command.h"
#include "utilities.h"

#define RAW_CMD_FULL_CONFIG_PARAMS      (9)
#define RAW_CMD_ENABLE_SINGLE           (3)
#define RAW_CMD_ENABLE_GLOBAL           (2)
#define RAW_CMD_ENABLE_TYPE_GLOBAL      (0)
#define RAW_CMD_ENABLE_TYPE_SINGLE      (1)

#define RAW_CMD_MAX_3BIT_SLOTS          (0b111)
#define RAW_CMD_MIN_SLOT_DUR_US         (500)
#define RAW_CMD_MAX_SLOT_DUR_US         (RAW_CMD_MIN_SLOT_DUR_US + (200 * (1 << 11) - 1))
#define RAW_CMD_MAX_RAW_DUR_US          (RAW_CMD_MAX_SLOT_DUR_US * RAW_CMD_MAX_3BIT_SLOTS)
#define RAW_CMD_MAX_START_TIME_US       (UINT8_MAX * 2 * 1024)

struct PACKED set_raw_command
{
    uint8_t enable_type;
    uint8_t enable;
    uint8_t idx;
    uint8_t config_type;
    uint32_t start_time_us;
    uint32_t raw_duration_us;
    uint8_t num_slots;
    uint8_t cross_slot_boundary;
    uint16_t max_beacon_spread;
    uint16_t nominal_stas_per_beacon;
};

static void usage(struct morsectrl *mors)
{
    mctrl_print(
            "\traw <enable|disable> [priority] [<start_time_us> <raw_duration_us> <num_slots> "
            "<x_slot> <max_beacon_spread> <nominal_beacons_per_sta>]\n");
    mctrl_print("\t\t\t\t'enable' will enable RAW\n");
    mctrl_print("\t\t\t\t'disable' will disable RAW\n");
    mctrl_print(
            "\t\t\t\tWithout the priority specified RAW will be globally enabled/disabled\n");
    mctrl_print(
            "\t\t\t\tThis global enable is seperate from the individual priority enables\n");
    mctrl_print("\t\t0-7\t\tNumber (priority) of the RAW to set\n");
    mctrl_print("\t\t0-%u\tStart time from last beacon or RAW (us)\n",
            RAW_CMD_MAX_START_TIME_US);
    mctrl_print("\t\t(%u * num_slots)-%u\n\t\t\t\tRAW duration time (us)\n",
            RAW_CMD_MIN_SLOT_DUR_US, RAW_CMD_MAX_RAW_DUR_US);
    mctrl_print("\t\t1-63\t\tNumber of slots\n");
    mctrl_print("\t\tenable|disable\tCross slot boundary bleed over allowed\n");
    mctrl_print("\t\t0-65535\tMaximum beacons to spread STAs over (0 no limit)\n");
    mctrl_print("\t\t0-65535\tNominal STAs in each beacon (0 disable beacon spreading)\n");
}

int raw(struct morsectrl *mors, int argc, char *argv[])
{
    int ret = -1;
    uint32_t temp;
    int enabled;
    struct set_raw_command *cmd;
    struct morsectrl_transport_buff *cmd_tbuff;
    struct morsectrl_transport_buff *rsp_tbuff;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    cmd_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, sizeof(*cmd));
    rsp_tbuff = morsectrl_transport_resp_alloc(&mors->transport, 0);

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    cmd = TBUFF_TO_CMD(cmd_tbuff, struct set_raw_command);

    if ((argc != RAW_CMD_ENABLE_GLOBAL) && (argc != RAW_CMD_ENABLE_SINGLE) &&
        (argc != RAW_CMD_FULL_CONFIG_PARAMS))
    {
        mctrl_err("Invalid command parameters\n");
        usage(mors);
        return -1;
    }

    enabled = expression_to_int(argv[1]);
    if (enabled == -1)
    {
        mctrl_err("Invalid command parameters\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    if (argc == RAW_CMD_ENABLE_GLOBAL)
    {
        cmd->enable_type = RAW_CMD_ENABLE_TYPE_GLOBAL;
        cmd->enable = enabled;
    }
    else if ((argc == RAW_CMD_ENABLE_SINGLE) || (argc == RAW_CMD_FULL_CONFIG_PARAMS))
    {
        cmd->enable_type = RAW_CMD_ENABLE_TYPE_SINGLE;
        cmd->enable = enabled;
    }

    if ((argc == RAW_CMD_ENABLE_SINGLE) || (argc == RAW_CMD_FULL_CONFIG_PARAMS))
    {
        temp = atoi(argv[2]);
        if ((temp < 0) || (temp > 7))
        {
            mctrl_err("Invalid RAW priority number, must be 0-7\n");
            ret = -1;
            goto exit;
        }
        cmd->idx = temp;
    }

    if (argc == RAW_CMD_FULL_CONFIG_PARAMS)
    {
        cmd->config_type = 1;

        temp = atoi(argv[3]);
        if (temp > 65535)
        {
            mctrl_err("Invalid start time, must be 0-65535\n");
            ret = -1;
            goto exit;
        }
        cmd->start_time_us = htole32(temp);

        temp = atoi(argv[5]);
        if ((temp < 1) || (temp > 63))
        {
            mctrl_err("Invalid number of slots, must be 1-63\n");
            ret = -1;
            goto exit;
        }
        cmd->num_slots = temp;

        temp = atoi(argv[4]);
        if (temp < (500 * cmd->num_slots) || temp > (RAW_CMD_MAX_SLOT_DUR_US * cmd->num_slots))
        {
            mctrl_err("Invalid RAW duration, must be %u-%u, perhaps reduce number of slots?\n",
                    cmd->num_slots * RAW_CMD_MIN_SLOT_DUR_US,
                    cmd->num_slots * RAW_CMD_MAX_SLOT_DUR_US);
            ret = -1;
            goto exit;
        }
        cmd->raw_duration_us = htole32(temp);

        enabled = expression_to_int(argv[6]);
        if (enabled == -1)
        {
            mctrl_err("Invalid cross slot boundary value\n");
            ret = -1;
            goto exit;
        }
        cmd->cross_slot_boundary = (uint8_t)enabled;

        temp = atoi(argv[7]);
        if (temp > UINT16_MAX)
        {
            mctrl_err("Invalid RAW max beacon spread\n");
            ret = -1;
            goto exit;
        }
        cmd->max_beacon_spread = htole16(temp);

        temp = atoi(argv[8]);
        if (temp > UINT16_MAX)
        {
            mctrl_err("Invalid RAW nominal STAs per beacon\n");
            ret = -1;
            goto exit;
        }
        cmd->nominal_stas_per_beacon = htole16(temp);
    }
    else
    {
        cmd->config_type = 0;
    }

    ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_SET_RAW,
                                 cmd_tbuff, rsp_tbuff);

exit:
    if (ret < 0)
    {
        mctrl_err("Failed to set RAW config\n");
    }
    else
    {
        if ((argc == RAW_CMD_ENABLE_SINGLE) || (argc == RAW_CMD_FULL_CONFIG_PARAMS)) {
            mctrl_print("\tRAW number (priority):\t\t\t%u\n", cmd->idx);
            mctrl_print("\t\tRAW:\t\t\t\t%s\n", (cmd->enable) ? "enabled" : "disabled");
        }
        else
        {
            mctrl_print("\tRAW:\t%s\n", (cmd->enable) ? "enabled" : "disabled");
        }

        if (argc == RAW_CMD_FULL_CONFIG_PARAMS)
        {
            mctrl_print("\t\tStart Time (us):\t\t%u\n", cmd->start_time_us);
            mctrl_print("\t\tRaw Duration (us):\t\t%u\n", cmd->raw_duration_us);
            mctrl_print("\t\tNumber of slots:\t\t%u\n", cmd->num_slots);
            mctrl_print("\t\tCross slot boundary bleed:\t%s\n",
                (cmd->cross_slot_boundary) ? "enabled" : "disabled");
            mctrl_print("\t\tMax beacon spread:\t\t%u\n", cmd->max_beacon_spread);
            mctrl_print("\t\tNominal STAs per beacon:\t%u\n", cmd->nominal_stas_per_beacon);
        }
    }

    morsectrl_transport_buff_free(cmd_tbuff);
    morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}
