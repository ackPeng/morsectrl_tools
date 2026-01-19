/*
 * Copyright 2020 Morse Micro
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>

#include "morsectrl.h"
#include "command.h"

struct command_handler
{
    const char *name;
    int (*handler)(struct morsectrl *, int argc, char *argv[]);
    const bool is_intf_cmd;
    const bool direct_chip_supported_cmd;
};

#ifndef MORSE_CLIENT
char TOOL_NAME[] = "morsectrl";
#else
char TOOL_NAME[] = "morse_cli";
#endif

struct command_handler commands[] =
{
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_VERSION)
    {"version", version, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_VERSION)
    {"hw_version", hw_version, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_STATS)
    {"stats", stats, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CHANNEL)
    {"channel", channel, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_BSSCOLOR)
    {"bsscolor", bsscolor, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_AMPDU)
    {"ampdu", ampdu, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_RAW)
    {"raw", raw, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_HEALTHCHECK)
    {"health", health, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CTS_SELF_PS)
    {"cts_self_ps", cts_self_ps, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_LONG_SLEEP)
    {"long_sleep", long_sleep, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_DUTY_CYCLE)
    {"duty_cycle",  duty_cycle,  true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_COREDUMP)
    {"coredump", coredump, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_OPCLASS)
    {"opclass", opclass, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TX_PKT_LIFETIME_US)
    {"tx_pkt_lifetime_us", tx_pkt_lifetime_us, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_PHYSM_WATCHDOG)
    {"physm_watchdog_en", physm_watchdog, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_SET_MAX_AMPDU_LENGTH)
    {"maxampdulen", maxampdulen, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MACADDR)
    {"macaddr", macaddr, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_SEND_WAKE_ACTION_FRAME)
    {"wakeaction", wakeaction, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_STANDBY)
    {"standby", standby, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MPSW)
    {"mpsw", mpsw, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_DHCPC)
    {"dhcpc", dhcpc, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_KEEPALIVE_OFFLOAD)
    {"keepalive", keepalive, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_VENDOR_IE)
    {"vendor_ie", vendor_ie, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_PARAM_GET_SET)
    {"set", param_set, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_PARAM_GET_SET)
    {"get", param_get, true, true},
#endif
#if !defined(MORSE_WIN_BUILD) && (!defined(MORSE_CLIENT) || defined(ENABLE_CMD_JTAG))
    {"jtag", jtag, false, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MCS)
    {"mcs", mcs, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_FEM)
    {"fem", fem, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_BW)
    {"bw", bw, true, true},
#endif
#if !defined(MORSE_WIN_BUILD) && (!defined(MORSE_CLIENT) || defined(ENABLE_CMD_IO))
    {"io", io, false, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_RPG)
    {"rpg", rpg, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_IFS)
    {"ifs", ifs, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_QOS)
    {"qos", qos, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_RI)
    {"ri", ri, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_TRANSMISSION_RATE)
    {"txrate", transmissionrate, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_STA_TYPE)
    {"sta_type", statype, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_ENC_MODE)
    {"enc_mode", encmode, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TXOP)
    {"txop", txop, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CR)
    {"cr", cr, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_NDP_PROBE_SUPPORT)
    {"ndpprobe", ndpprobes, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_LI)
    {"li", li, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TURBO)
    {"turbo", turbo, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TEST_ASSERT)
    {"assert", force_assert, true, true},
#endif
#if !defined(MORSE_WIN_BUILD) && (!defined(MORSE_CLIENT) || defined(ENABLE_CMD_SERIAL))
    {"serial", serial, false, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_LNA_BYPASS)
    {"lnabypass",  lnabypass, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TX_SCALER)
    {"txscaler", txscaler, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TRANSMIT_CW)
    {"transmit_cw", transmit_cw, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_PERIODIC_CAL)
    {"periodic_cal", periodic_cal, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_DTIM_CHANNEL_CHANGE)
    {"dtim_channel_change", dtim_channel_change,  true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_BCN_RSSI_THRESHOLD)
    {"bcn_rssi_threshold", bcn_rssi_threshold,  true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_SIG_FIELD_ERROR_EVT)
    {"sig_field_error_evt", sig_field_error_evt,  true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_ANTENNA)
    {"antenna", antenna, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TDC_PG_DISABLE)
    {"tdc_pg_disable", tdc_pg_disable, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CAPABILITIES)
    {"capabilities", capabilities, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_LOAD_ELF)
    {"load_elf", load_elf, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TRANSRAW)
    {"transraw",  transraw,  true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_OTP)
    {"otp", otp, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_HWKEYDUMP)
    {"hwkeydump", hwkeydump, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TWT)
    {"twt", twt, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CAC)
    {"cac", cac, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TSF)
    {"tsf", tsf, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_PHY_DEAF)
    {"phy_deaf", phy_deaf, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_SET_FSG)
    {"fsg", fsg, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_RESET)
    {"reset", reset, false, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_OVERRIDE_PA_ON_DELAY)
    {"override_pa_on_delay", override_pa_on_delay, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_CHAN_QUERY)
    {"chan_query", chan_query, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_ED_CONFIG)
    {"edconfig", edconfig, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_ECSA_INFO)
    {"ecsa_info", ecsa_info, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_OCS)
    {"ocs", ocs, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_TX_PWR_ADJ)
    {"tx_pwr_adj", tx_pwr_adj, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_SET_AGC_GAINCODE)
    {"set_agc_gaincode", set_agc_gaincode, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MBSSID_IE_INFO)
    {"mbssid", mbssid, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MESH_CONFIG)
    {"mesh_config", mesh_config, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_MBCA)
    {"mbca", mbca, true, false},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_UAPSD_CONFIG)
    {"uapsd", uapsd, true, true},
#endif
#if !defined(MORSE_CLIENT) || defined(ENABLE_CMD_GPIO_CONTROL)
    {"gpio_control", gpio_control, true, true},
#endif
};

#ifndef MORSECTRL_VERSION_STRING
#define MORSECTRL_VERSION_STRING "Undefined"
#endif

static int error_function(const char *prefix, int error_code, const char *error_msg)
{
    mctrl_err("%s, code %d: %s\n", prefix, error_code, error_msg);
    return 0;
}

static void usage(struct morsectrl *mors)
{
    int i;
    mctrl_print("Usage: %s [options] command [command_options]\n", TOOL_NAME);
    mctrl_print("\nOptions:\n"
           "\t-h, --help\t\t\t\tprint this message\n"
           "\t-d, --debug\t\t\t\tshow nl80211 debug messages for given interface command\n"
           "\t-f, --configfile\t\t\tspecify config file with transport/interface/config\n"
           "\t\t\t\t\t\t(command line will override file contents)\n"
           "\t-t, --transport\t\t\t\tspecify transport to use [nl80211 | ftdi_spi]\n"
           "\t-i, --interface\t\t\t\tspecify the interface for the transport (default %s)\n"
           "\t-c, --config\t\t\t\tspecify the config for the transport\n"
           "\t\t\t\t\t\tuse '-c help' to list options for the specified transport\n"
           "\t-v\t\t\t\t\tprints the %s version\n",
           DEFAULT_INTERFACE_NAME, TOOL_NAME);

    mctrl_print("\nTransports Available:\n");
#ifdef ENABLE_TRANS_NL80211
    mctrl_print("\tnl80211: Uses 802.11 netlink interface\n");
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
    mctrl_print("\tftdi_spi: Uses ftdi spi interface\n");
#endif
#if defined(ENABLE_TRANS_NL80211) && defined(ENABLE_TRANS_FTDI_SPI)
    mctrl_print("\tThe set of supported commands is different for each transport.\n");
#endif

    mctrl_print("\nInterface Commands:\n");
    for (i = 0; i < MORSE_ARRAY_SIZE(commands); i++)
    {
        if (commands[i].is_intf_cmd)
        {
#ifdef ENABLE_TRANS_NL80211
            if (mors->transport.type == MORSECTRL_TRANSPORT_NL80211)
            {
                commands[i].handler(mors, 0, NULL);
            }
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
            if (mors->transport.type == MORSECTRL_TRANSPORT_FTDI_SPI &&
                commands[i].direct_chip_supported_cmd)
            {
                commands[i].handler(mors, 0, NULL);
            }
#endif
        }
    }
    mctrl_print("\nGeneral Commands (no interface required):\n");
    for (i = 0; i < MORSE_ARRAY_SIZE(commands); i++)
    {
        if (!commands[i].is_intf_cmd)
        {
#ifdef ENABLE_TRANS_NL80211
            if (mors->transport.type == MORSECTRL_TRANSPORT_NL80211)
            {
                commands[i].handler(mors, 0, NULL);
            }
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
            if (mors->transport.type == MORSECTRL_TRANSPORT_FTDI_SPI &&
                commands[i].direct_chip_supported_cmd)
            {
                commands[i].handler(mors, 0, NULL);
            }
#endif
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    int opt_index, opt;
    int ret = MORSE_OK;
    char *trans_opts = NULL;
    char *iface_opts = NULL;
    char *cfg_opts = NULL;
    char *file_opts = NULL;
    struct morsectrl_transport *transport;

    struct morsectrl mors = {
        .debug = false,
    };

    transport = &mors.transport;
    transport->type = MORSECTRL_TRANSPORT_NONE;
    transport->debug = false;
    transport->error_function = error_function;

    /*build long option array*/
    /* NB optstring and long_optstrings need to be manually kept in sync,*/
    char optstring[] = "+dht:i:c:f:b:v";
    char *long_optstrings[] = {"debug", "help", "transport", "interface", "config", "configfile"};

    /* + 1 for terminating all-0 element*/
    struct option long_options[MORSE_ARRAY_SIZE(long_optstrings) + 1];
    memset(long_options, 0, sizeof(long_options));
    int short_option_offset = 1; /* + 1 to skip leading '+'*/
    for (int i = 0; i < MORSE_ARRAY_SIZE(long_optstrings); i++)
    {
        long_options[i].name = long_optstrings[i];
        long_options[i].val = optstring[i + short_option_offset];
        if (optstring[i + short_option_offset + 1] == ':')
        {
            long_options[i].has_arg = true;
            short_option_offset++;
        }
        else
        {
            long_options[i].has_arg = false;
        }
        long_options[i].flag = NULL;
    }

    while (-1 != (opt = getopt_long(argc, argv, optstring, long_options, &opt_index)))
    {
        switch (opt)
        {
            case 'd':
                mors.debug = true;
                transport->debug = true;
                break;
            case 'h':
                morsectrl_transport_parse(transport, trans_opts, iface_opts, cfg_opts);
                usage(&mors);
                goto exit;
            case 't':
                trans_opts = optarg;
                break;
            case 'i':
                iface_opts = optarg;
                break;
            case 'c':
                cfg_opts = optarg;
                break;
            case 'f':
                file_opts = optarg;
                break;
            case 'v':
                mctrl_print("Morsectrl Version: %s\n", MORSECTRL_VERSION_STRING);
                return 0;
            default:
                mctrl_err("Try %s --help for more information\n", TOOL_NAME);
                return 1;
        }
    }

    if (file_opts)
    {
        ret = morsectrl_config_file_parse(file_opts,
                                          &trans_opts,
                                          &iface_opts,
                                          &cfg_opts,
                                          mors.debug);

        if (ret)
            goto exit;
    }

    ret = morsectrl_transport_parse(transport, trans_opts, iface_opts, cfg_opts);

    if (transport->type == MORSECTRL_TRANSPORT_NONE)
    {
        ret = -ENODEV;
        goto exit;
    }

    if (ret)
        goto exit;

    if (optind >= argc)
    {
        mctrl_err("Could not find the command. Try %s --help\n", TOOL_NAME);
        return MORSE_ARG_ERR;
    }

    argc -= optind;
    argv += optind;

    for (i = 0; i < (int)MORSE_ARRAY_SIZE(commands); i++)
    {
        if (!strcmp(argv[0], commands[i].name))
        {
            optind = 1; /* In case handler wants to use getopt */

            if (mors.debug)
            {
                mctrl_print("Calling: %s ", commands[i].name);
                for (int j = 1; j < argc; j++)
                {
                    mctrl_print("%s ", argv[j]);
                }
                mctrl_print("\n");
            }
#ifdef ENABLE_TRANS_FTDI_SPI
            if (!commands[i].direct_chip_supported_cmd &&
                transport->type == MORSECTRL_TRANSPORT_FTDI_SPI)
            {
                mctrl_err("Command '%s' cannot be used with transport %s\n", commands[i].name,
                        trans_opts);
                mctrl_err("To check valid commands run 'morsectrl -t %s -h'\n", trans_opts);
                ret = ETRANSFTDISPIERR;
                goto exit;
            }
#endif
            if (!strcmp(commands[i].name, "version"))
                mctrl_print("Morsectrl Version: %s\n", MORSECTRL_VERSION_STRING);

            if (commands[i].is_intf_cmd ||
                (!strncmp(commands[i].name, "reset", strlen(commands[i].name)) &&
                 transport->has_reset))
            {
                ret = morsectrl_transport_init(transport);
                if (ret)
                {
                    mctrl_err("Transport init failed\n");
                    goto exit;
                }
            }

            ret = commands[i].handler(&mors, argc, argv);
            goto transport_exit;
        }
    }

    ret = MORSE_CMD_ERR;
    mctrl_err("Invalid command '%s'\n", argv[0]);
    mctrl_err("Try %s --help for more information\n", TOOL_NAME);

transport_exit:
    if ((i < (int)MORSE_ARRAY_SIZE(commands)) && commands[i].is_intf_cmd)
        morsectrl_transport_deinit(transport);
exit:
    /**
     * For return codes less than 0, or greater than 255 (i.e. the nix return code error range)
     * remap error to MORSE_CMD_ERR. The return code 255 (-1) is avoided as ssh uses this to
     * indicate an ssh error.
     */
    if ((ret < 0) || (ret > 254))
    {
        ret = MORSE_CMD_ERR;
    }
    return ret;
}
