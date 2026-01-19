/*
 * Copyright 2022 Morse Micro
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "transport.h"
#include "../command.h"
#include "../utilities.h"

const char *transport_none = "none";
const char *transport_nl80211 = "nl80211";
const char *transport_ftdi_spi = "ftdi_spi";

int morsectrl_transport_parse(struct morsectrl_transport *transport,
                              const char *trans_opts,
                              const char *iface_opts,
                              const char *cfg_opts)
{
    int ret = -ETRANSERR;

    if (transport->debug)
        mctrl_print("Transport set to: ");

    if (trans_opts == NULL)
    {
        if (transport->debug)
#ifdef ENABLE_TRANS_NL80211
        /* Select NL80211 by default */
            mctrl_print("NL80211\n");
        transport->type = MORSECTRL_TRANSPORT_NL80211;
        transport->tops = &nl80211_ops;
#else
            mctrl_print("None\n");
        transport->type = MORSECTRL_TRANSPORT_NONE;
        transport->tops = NULL;
#endif
    }
#ifdef ENABLE_TRANS_NL80211
    else if (!strncmp(trans_opts, transport_nl80211, strlen(transport_nl80211)))
    {
        if (transport->debug)
            mctrl_print("NL80211\n");
        transport->type = MORSECTRL_TRANSPORT_NL80211;
        transport->tops = &nl80211_ops;
    }
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
    else if (!strncmp(trans_opts, transport_ftdi_spi, strlen(transport_ftdi_spi)))
    {
        if (transport->debug)
            mctrl_print("FTDI SPI\n");
        transport->type = MORSECTRL_TRANSPORT_FTDI_SPI;
        transport->tops = &ftdi_spi_ops;
    }
#endif
    else
    {
        if (transport->debug)
            mctrl_print("None\n");
        transport->type = MORSECTRL_TRANSPORT_NONE;
        transport->tops = NULL;
    }

    if (transport->tops)
        ret = transport->tops->parse(transport, iface_opts, cfg_opts);

    if (ret)
        transport->error_function("Transport parsing", -ETRANSERR, "Invalid transport");

    return ret;
}

int morsectrl_transport_init(struct morsectrl_transport *transport)
{
    if (transport->tops)
        return transport->tops->init(transport);

    return -ETRANSERR;
}

int morsectrl_transport_deinit(struct morsectrl_transport *transport)
{
    int ret = ETRANSSUCC;

    if (transport->tops)
        ret = transport->tops->deinit(transport);

    transport->tops = NULL;

    return ret;
}

struct morsectrl_transport_buff *morsectrl_transport_cmd_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport->tops)
        return NULL;

    /* Add the size of the command header and pass down to the correct transport. */
    return transport->tops->write_alloc(transport, sizeof(struct command) + size);
}

struct morsectrl_transport_buff *morsectrl_transport_resp_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport->tops)
        return NULL;

    /* Add the size of the response header and pass down to the correct transport. */
    return transport->tops->read_alloc(transport, sizeof(struct response) + size);
}

struct morsectrl_transport_buff *morsectrl_transport_raw_read_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport->tops || !transport->tops->read_alloc)
        return NULL;

    /* Add the size of the response header and pass down to the correct transport. */
    return transport->tops->read_alloc(transport, size);
}

struct morsectrl_transport_buff *morsectrl_transport_raw_write_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport->tops || !transport->tops->write_alloc)
        return NULL;

    /* Add the size of the response header and pass down to the correct transport. */
    return transport->tops->write_alloc(transport, size);
}

int morsectrl_transport_buff_free(struct morsectrl_transport_buff *buff)
{
    if (!buff)
        return -ETRANSERR;

    free(buff->memblock);
    free(buff);

    return ETRANSSUCC;
}

void morsectrl_transport_set_cmd_data_length(struct morsectrl_transport_buff *tbuff,
                                             uint16_t length)
{
    tbuff->data_len = sizeof(struct command) + length;
}

int morsectrl_transport_reg_read(struct morsectrl_transport *transport,
                                 uint32_t addr, uint32_t *value)
{
    if (!transport->tops || !transport->tops->reg_read)
        return -ETRANSERR;

    return transport->tops->reg_read(transport, addr, value);
}

int morsectrl_transport_reg_write(struct morsectrl_transport *transport,
                                  uint32_t addr, uint32_t value)
{
    if (!transport->tops || !transport->tops->reg_write)
        return -ETRANSERR;

    return transport->tops->reg_write(transport, addr, value);
}

int morsectrl_transport_mem_read(struct morsectrl_transport *transport,
                                 struct morsectrl_transport_buff *read,
                                 uint32_t addr)
{
    if (!transport->tops || !transport->tops->mem_read)
        return -ETRANSERR;

    return transport->tops->mem_read(transport, read, addr);
}

int morsectrl_transport_mem_write(struct morsectrl_transport *transport,
                                  struct morsectrl_transport_buff *write,
                                  uint32_t addr)
{
    if (!transport->tops || !transport->tops->mem_write)
        return -ETRANSERR;

    return transport->tops->mem_write(transport, write, addr);
}

int morsectrl_transport_send(struct morsectrl_transport *transport,
                             struct morsectrl_transport_buff *cmd,
                             struct morsectrl_transport_buff *resp)
{
    if (!transport->tops)
        return -ETRANSERR;

    return transport->tops->send(transport, cmd, resp);
}

int morsectrl_transport_raw_read(struct morsectrl_transport *transport,
                                 struct morsectrl_transport_buff *read,
                                 bool start,
                                 bool finish)
{
    if (!transport->tops)
        return -ETRANSERR;

    return transport->tops->raw_read(transport, read, start, finish);
}

int morsectrl_transport_raw_write(struct morsectrl_transport *transport,
                                  struct morsectrl_transport_buff *write,
                                  bool start,
                                  bool finish)
{
    if (!transport->tops)
        return -ETRANSERR;

    return transport->tops->raw_write(transport, write, start, finish);
}

int morsectrl_transport_raw_read_write(struct morsectrl_transport *transport,
                                       struct morsectrl_transport_buff *read,
                                       struct morsectrl_transport_buff *write,
                                       bool start,
                                       bool finish)
{
    if (!transport->tops)
        return -ETRANSERR;

    return transport->tops->raw_read_write(transport, read, write, start, finish);
} /* NOLINT */

int morsectrl_transport_reset_device(struct morsectrl_transport *transport)
{
    if (!transport->tops || !transport->tops->reset_device)
        return -ETRANSERR;

    return transport->tops->reset_device(transport);
}
