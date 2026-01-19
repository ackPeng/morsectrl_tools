/*
 * Copyright 2020 Morse Micro
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "portable_endian.h"
#include "command.h"
#include "channel.h"
#include "transport/transport.h"

#ifndef MORSE_CLIENT
#define MORSE_CHANNEL_MAX_ARGS    12
#else
#define MORSE_CHANNEL_MAX_ARGS    11
#endif

static void usage(struct morsectrl *mors) {
    mctrl_print("\tchannel [options]\n");
    mctrl_print("\t\t\t\tsets channel parameters\n");
    mctrl_print(
           "\t\t\t\tor read associated or full channel frequency if none of [-c|-o|-p|-n] given\n");
    mctrl_print("\t\t-a \t\tprints all the channel i.e. Full, DTIM and current\n");
    mctrl_print("\t\t-c <value>\tchannel frequency in kHz\n");
    mctrl_print("\t\t-o <value>\toperating bandwidth in MHz\n");
    mctrl_print("\t\t-p <value>\tprimary bandwidth in MHz\n");
    mctrl_print("\t\t-n <value>\tprimary 1 MHz channel index\n");
#ifndef MORSE_CLIENT
    mctrl_print("\t\t-r\t\tignores regulatory max tx power\n");
#endif
    mctrl_print("\t\t-j\t\tprints full channel information in easily parsable JSON format\n");
}

int channel(struct morsectrl *mors, int argc, char *argv[])
{
    int ret = -1;
    uint32_t freq_khz = 0;
    uint8_t op_channel_bandwidth = BANDWIDTH_DEFAULT;
    uint8_t primary_channel_bandwidth = BANDWIDTH_DEFAULT;
    uint8_t primary_1mhz_channel_index = PRIMARY_1MHZ_CHANNEL_INDEX_DEFAULT;
    struct command_set_channel_req *cmd_set;
    struct command_get_channel_cfm *resp_get;
    bool set_freq = false;
    bool get_all_channels = false;
    bool json = false;
    uint8_t s1g_chan_power = 1;
    struct morsectrl_transport_buff *cmd_set_tbuff;
    struct morsectrl_transport_buff *rsp_set_tbuff;
    struct morsectrl_transport_buff *cmd_get_tbuff;
    struct morsectrl_transport_buff *rsp_get_tbuff;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    cmd_set_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, sizeof(*cmd_set));
    rsp_set_tbuff = morsectrl_transport_resp_alloc(&mors->transport, 0);
    cmd_get_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, 0);
    rsp_get_tbuff = morsectrl_transport_resp_alloc(&mors->transport, sizeof(*resp_get));

    if (!cmd_set_tbuff || !rsp_set_tbuff || !cmd_get_tbuff || !rsp_get_tbuff)
        goto exit;

    cmd_set = TBUFF_TO_CMD(cmd_set_tbuff, struct command_set_channel_req);
    resp_get = TBUFF_TO_RSP(rsp_get_tbuff, struct command_get_channel_cfm);

    if (argc <= MORSE_CHANNEL_MAX_ARGS)
    {
        int option;
        while ((option = getopt(argc, argv, "c:o:p:n:ajr")) != -1)
        {
            switch (option)
            {
                case 'c' :
                    freq_khz = atoi(optarg);
                    set_freq = true;
                    break;
                case 'o' :
                    op_channel_bandwidth = atoi(optarg);
                    set_freq = true;
                    break;
                case 'p' :
                    primary_channel_bandwidth = atoi(optarg);
                    set_freq = true;
                    break;
                case 'n' :
                    primary_1mhz_channel_index = atoi(optarg);
                    set_freq = true;
                    break;
                case 'j' :
                    json = true;
                    break;
                case 'a' :
                    get_all_channels = true;
                    break;
#ifndef MORSE_CLIENT
                /* Only allow morsectrl to disable setting regulatory max tx power on channel
                 * change as it is a debug utility
                 */
                case 'r' :
                    s1g_chan_power = 0;
                    break;
#endif
                case '?' :
                    usage(mors);
                    return -1;
                default :
                    mctrl_err("Invalid argument: %c\n", option);
                    usage(mors);
                    goto exit;
            }
        }
    }
    else
    {
        mctrl_err("Invalid number of arguments (max %d)\n", MORSE_CHANNEL_MAX_ARGS);
        usage(mors);
        goto exit;
    }

    if (set_freq)
    {
        if (freq_khz < MIN_FREQ_KHZ || freq_khz > MAX_FREQ_KHZ)
        {
            mctrl_err("Invalid frequency %d. Must be between %d kHz and %d kHz\n",
                    freq_khz, MIN_FREQ_KHZ, MAX_FREQ_KHZ);
            usage(mors);
            goto exit;
        }

        cmd_set->operating_channel_freq_hz = htole32(KHZ_TO_HZ(freq_khz));
        cmd_set->operating_channel_bw_mhz = op_channel_bandwidth;
        cmd_set->primary_channel_bw_mhz = primary_channel_bandwidth;
        cmd_set->primary_1mhz_channel_index = primary_1mhz_channel_index;
        cmd_set->dot11_mode = 0; /* TODO */
        cmd_set->s1g_chan_power = s1g_chan_power;

        ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_SET_CHANNEL,
                                     cmd_set_tbuff, rsp_set_tbuff);

        if (ret < 0)
        {
            mctrl_err("Failed to set channel: error(%d)\n", ret);
            goto exit;
        }
    }

    ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_GET_FULL_CHANNEL,
                                 cmd_get_tbuff, rsp_get_tbuff);

    if (ret < 0)
    {
        mctrl_err("Failed to get channel frequency: error(%d)\n", ret);
        goto exit;
    }
    if (json)
    {
        mctrl_print("{\n" \
               "    \"channel_frequency\":%d,\n" \
               "    \"channel_op_bw\":%d,\n" \
               "    \"channel_primary_bw\":%d,\n" \
               "    \"channel_index\":%d,\n" \
               "    \"bw_mhz\":%d\n" \
               "}\n",
               (resp_get->operating_channel_freq_hz / 1000),
               resp_get->operating_channel_bw_mhz,
               resp_get->primary_channel_bw_mhz,
               resp_get->primary_1mhz_channel_index,
               resp_get->operating_channel_bw_mhz);
    }
    else
    {
        mctrl_print("Full Channel Information\n" \
               "\tOperating Frequency: %d kHz\n" \
               "\tOperating BW: %d MHz\n" \
               "\tPrimary BW: %d MHz\n" \
               "\tPrimary Channel Index: %d\n",
               (resp_get->operating_channel_freq_hz / 1000),
               resp_get->operating_channel_bw_mhz,
               resp_get->primary_channel_bw_mhz,
               resp_get->primary_1mhz_channel_index);
    }

    if (get_all_channels)
    {
        ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_GET_DTIM_CHANNEL,
                                     cmd_get_tbuff, rsp_get_tbuff);
        if (ret < 0)
        {
            mctrl_err("Failed to get channel frequency: error(%d)\n", ret);
            goto exit;
        }

        mctrl_print("DTIM Channel Information\n" \
               "\tOperating Frequency: %d kHz\n" \
               "\tOperating BW: %d MHz\n" \
               "\tPrimary BW: %d MHz\n" \
               "\tPrimary Channel Index: %d\n",
               (resp_get->operating_channel_freq_hz / 1000),
               resp_get->operating_channel_bw_mhz,
               resp_get->primary_channel_bw_mhz,
               resp_get->primary_1mhz_channel_index);

        ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_GET_CURRENT_CHANNEL,
                                     cmd_get_tbuff, rsp_get_tbuff);
        if (ret < 0)
        {
            mctrl_err("Failed to get channel frequency: error(%d)\n", ret);
            goto exit;
        }

        mctrl_print("Current Channel Information\n" \
               "\tOperating Frequency: %d kHz\n" \
               "\tOperating BW: %d MHz\n" \
               "\tPrimary BW: %d MHz\n" \
               "\tPrimary Channel Index: %d\n",
               (resp_get->operating_channel_freq_hz / 1000),
               resp_get->operating_channel_bw_mhz,
               resp_get->primary_channel_bw_mhz,
               resp_get->primary_1mhz_channel_index);
    }

exit:
    morsectrl_transport_buff_free(cmd_set_tbuff);
    morsectrl_transport_buff_free(rsp_set_tbuff);
    morsectrl_transport_buff_free(cmd_get_tbuff);
    morsectrl_transport_buff_free(rsp_get_tbuff);
    return ret;
}
