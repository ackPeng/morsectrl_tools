/*
 * Copyright 2020 Morse Micro
 */

#pragma once

#include <stdbool.h>
#include "utilities.h"
#include "transport/transport.h"

#define MORSE_ARRAY_SIZE(array) (sizeof((array)) / sizeof((array[0])))

#define MORSE_OK 0
#define MORSE_ARG_ERR 1
#define MORSE_CMD_ERR 2

typedef struct statistics_offchip_data offchip_stats_t; /* typedef reqd for circular definition */

struct morsectrl {
    bool debug;
    struct morsectrl_transport transport;
    offchip_stats_t *stats;
    size_t n_stats;
};

/**
 * @brief Parse a config file to obtain transport, interface and other configuration options.
 *
 * The parser will allocate the memory for the transport, interface and configuration strings.
 *
 * @param file_opts     String containing the filename of the config file
 * @param trans_opts    Transport string (to be filled by parser)
 * @param iface_opts    Interface string (to be filled by parser)
 * @param cfg_opts      Other configuration options (to be filled by parser)
 * @param debug         Whether or not to print debug information while parsing
 *
 * @return              0 if success otherwise -1
 */
int morsectrl_config_file_parse(const char *file_opts,
                                char **trans_opts,
                                char **iface_opts,
                                char **cfg_opts,
                                bool debug);

/* commands */
int version(struct morsectrl *mors, int argc, char *argv[]);
int hw_version(struct morsectrl *mors, int argc, char *argv[]);
int reset(struct morsectrl *mors, int argc, char *argv[]);
int stats(struct morsectrl *mors, int argc, char *argv[]);
int channel(struct morsectrl *mors, int argc, char *argv[]);
int bsscolor(struct morsectrl *mors, int argc, char *argv[]);
int ampdu(struct morsectrl *mors, int argc, char *argv[]);
int raw(struct morsectrl *mors, int argc, char *argv[]);
int health(struct morsectrl *mors, int argc, char *argv[]);
int cts_self_ps(struct morsectrl *mors, int argc, char *argv[]);
int long_sleep(struct morsectrl *mors, int argc, char *argv[]);
int duty_cycle(struct morsectrl *mors, int argc, char *argv[]);
int coredump(struct morsectrl *mors, int argc, char *argv[]);
int opclass(struct morsectrl *mors, int argc, char *argv[]);
int tx_pkt_lifetime_us(struct morsectrl *mors, int argc, char *argv[]);
int physm_watchdog(struct morsectrl *mors, int argc, char *argv[]);
int maxampdulen(struct morsectrl *mors, int argc, char *argv[]);
int macaddr(struct morsectrl *mors, int argc, char *argv[]);
int wakeaction(struct morsectrl *mors, int argc, char *argv[]);
int standby(struct morsectrl *mors, int argc, char *argv[]);
int mpsw(struct morsectrl *mors, int argc, char *argv[]);
int dhcpc(struct morsectrl *mors, int argc, char *argv[]);
int keepalive(struct morsectrl *mors, int argc, char *argv[]);
int vendor_ie(struct morsectrl *mors, int argc, char *argv[]);
int param_get(struct morsectrl *mors, int argc, char *argv[]);
int param_set(struct morsectrl *mors, int argc, char *argv[]);
int mcs(struct morsectrl *mors, int argc, char *argv[]);
int fem(struct morsectrl *mors, int argc, char *argv[]);
int jtag(struct morsectrl *mors, int argc, char *argv[]);
int bw(struct morsectrl *mors, int argc, char *argv[]);
int rpg(struct morsectrl *mors, int argc, char *argv[]);
int ifs(struct morsectrl *mors, int argc, char *argv[]);
int qos(struct morsectrl *mors, int argc, char *argv[]);
int ri(struct morsectrl *mors, int argc, char *argv[]);
int transmissionrate(struct morsectrl *mors, int argc, char *argv[]);
int statype(struct morsectrl *mors, int argc, char *argv[]);
int encmode(struct morsectrl *mors, int argc, char *argv[]);
int txop(struct morsectrl *mors, int argc, char *argv[]);
int cr(struct morsectrl *mors, int argc, char *argv[]);
int ndpprobes(struct morsectrl *mors, int argc, char *argv[]);
int li(struct morsectrl *mors, int argc, char *argv[]);
int turbo(struct morsectrl *mors, int argc, char *argv[]);
int force_assert(struct morsectrl *mors, int argc, char *argv[]);
int lnabypass(struct morsectrl *mors, int argc, char *argv[]);
int txscaler(struct morsectrl *mors, int argc, char *argv[]);
int transmit_cw(struct morsectrl *mors, int argc, char *argv[]);
int periodic_cal(struct morsectrl *mors, int argc, char *argv[]);
int dtim_channel_change(struct morsectrl *mors, int argc, char *argv[]);
int bcn_rssi_threshold(struct morsectrl *mors, int argc, char *argv[]);
int sig_field_error_evt(struct morsectrl *mors, int argc, char *argv[]);
int antenna(struct morsectrl *mors, int argc, char *argv[]);
int tdc_pg_disable(struct morsectrl *mors, int argc, char *argv[]);
int capabilities(struct morsectrl *mors, int argc, char *argv[]);
int load_elf(struct morsectrl *mors, int argc, char *argv[]);
int transraw(struct morsectrl *mors, int argc, char *argv[]);
int otp(struct morsectrl *mors, int argc, char *argv[]);
int hwkeydump(struct morsectrl *mors, int argc, char *argv[]);
int twt(struct morsectrl *mors, int argc, char *argv[]);
int cac(struct morsectrl *mors, int argc, char *argv[]);
int tsf(struct morsectrl *mors, int argc, char *argv[]);
int phy_deaf(struct morsectrl *mors, int argc, char *argv[]);
int override_pa_on_delay(struct morsectrl *mors, int argc, char *argv[]);
int fsg(struct morsectrl *mors, int argc, char *argv[]);
int chan_query(struct morsectrl *mors, int argc, char *argv[]);
int edconfig(struct morsectrl *mors, int argc, char *argv[]);
int ecsa_info(struct morsectrl *mors, int argc, char *argv[]);
int tx_pwr_adj(struct morsectrl *mors, int argc, char *argv[]);
int set_agc_gaincode(struct morsectrl *mors, int argc, char *argv[]);
int mbssid(struct morsectrl *mors, int argc, char *argv[]);
int mesh_config(struct morsectrl *mors, int argc, char *argv[]);
int mbca(struct morsectrl *mors, int argc, char *argv[]);
int uapsd(struct morsectrl *mors, int argc, char *argv[]);
int gpio_control(struct morsectrl *mors, int argc, char *argv[]);

#ifndef MORSE_WIN_BUILD
int io(struct morsectrl *mors, int argc, char *argv[]);
int serial(struct morsectrl *mors, int argc, char *argv[]);
#endif
int ocs(struct morsectrl *mors, int argc, char *argv[]);
