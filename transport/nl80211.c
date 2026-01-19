/*
 * Copyright 2020 Morse Micro
 */

#include <linux/nl80211.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <net/if.h>
#include <netlink/attr.h>

#include "../utilities.h"
#include "transport.h"

#define MORSE_OUI 0x0CBF74
#define MORSE_VENDOR_CMD_TO_MORSE 0x00
#define NL80211_BUFFER_SIZE (8192)

/**
 * @brief Prints an error message if possible.
 *
 * @param transport     Transport to print the error message from.
 * @param error_code    Error code.
 * @param error_msg     Error message.
 * @return              0 on success or relevant error.
 */
static int morsectrl_nl80211_error(struct morsectrl_transport *transport,
                                   int error_code, char *error_msg)
{
    if (transport->error_function)
        return transport->error_function("NL80211", error_code, error_msg);
    return 0;
}

static int morsectrl_nl80211_parse(struct morsectrl_transport *transport,
                                   const char *iface_opts,
                                   const char *cfg_opts)
{
    struct morsectrl_nl80211_cfg *cfg;

    (void)cfg_opts;

    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return -ETRANSNL80211ERR;

    transport->has_reset = false;
    cfg = &transport->config.nl80211;

    if (iface_opts == NULL)
        cfg->interface_name = DEFAULT_INTERFACE_NAME;
    else
        cfg->interface_name = iface_opts;

    if (transport->debug)
        mctrl_print("Using %s interface\n", cfg->interface_name);

    return ETRANSSUCC;
}

/**
 * @brief Handle errors from the netlink interface.
 *
 * @param nla   Netlink socket address.
 * @param nlerr Netlink error.
 * @param arg   @ref morsectrl_transport opaque pointer.
 * @return      NL_STOP always.
 */
static int morsectrl_nl80211_error_handler(struct sockaddr_nl *nla,
                                           struct nlmsgerr *nlerr,
                                           void *arg)
{
    struct morsectrl_transport *transport = (struct morsectrl_transport *)arg;

    if (transport && (transport->type != MORSECTRL_TRANSPORT_NL80211))
        morsectrl_nl80211_error(transport, nlerr->error, "Error callback called");

    return NL_STOP;
}

/**
 * @brief Handle acks from the netlink interface.
 *
 * @param msg   Netlink message.
 * @param arg   @ref morsectrl_transport opaque pointer.
 * @return      NL_STOP always.
 */
static int morsectrl_nl80211_ack_handler(struct nl_msg *msg, void *arg)
{
    struct morsectrl_transport *transport = (struct morsectrl_transport *)arg;

    if (transport)
    {
        transport->state.nl80211.wait_for_ack = false;

        if (transport->debug) {
            mctrl_print("nla_msg_dump\n");
            nl_msg_dump(msg, stdout);
        }
    }

    return NL_STOP;
}

/**
 * @brief Handle reception of response messages from the netlink interface.
 *
 * @param msg   Netlink message.
 * @param arg   @ref morsectrl_transport opaque pointer.
 * @return      NL_SKIP if reponse is missing otherwise NL_OK.
 */
static int morsectrl_nl80211_receive_handler(struct nl_msg *msg, void *arg)
{
    struct morsectrl_transport *transport = (struct morsectrl_transport *)arg;
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *attr;
    struct morsectrl_nl80211_state *state;
    uint8_t *data;
    int len;

    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
    {
        morsectrl_nl80211_error(transport, -ETRANSNL80211ERR, "Transport for receive invalid");
        return NL_SKIP;
    }

    state = (struct morsectrl_nl80211_state *)&transport->state.nl80211;

    if (transport->debug)
    {
        mctrl_print("nla_msg_dump\n");
        nl_msg_dump(msg, stdout);
    }

    attr = nla_find(genlmsg_attrdata(gnlh, 0),
                    genlmsg_attrlen(gnlh, 0),
                    NL80211_ATTR_VENDOR_DATA);
    if (!attr)
    {
        morsectrl_nl80211_error(transport, 0, "Vendor data attribute missing");
        return NL_SKIP;
    }

    data = (uint8_t *) nla_data(attr);
    len = nla_len(attr);

    if (len > *state->len)
    {
        morsectrl_nl80211_error(transport, -ETRANSNL80211ERR,
                                "Output buffer too small limiting output");
        len = *state->len;
    }

    memcpy(state->data, data, len);
    *state->len = len;
    return NL_OK;
}

static int morsectrl_nl80211_init(struct morsectrl_transport *transport)
{
    struct morsectrl_nl80211_state *state;
    struct morsectrl_nl80211_cfg *cfg;
    int ret = 0;
#ifdef NETLINK_EXT_ACK
    int option_value;
#endif

    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return -ETRANSNL80211ERR;

    cfg = &transport->config.nl80211;
    state = &transport->state.nl80211;
    state->interface_index = if_nametoindex(cfg->interface_name);

    if (state->interface_index == 0)
    {
        morsectrl_nl80211_error(transport, state->interface_index,
                                "Invalid interface index");

        return -ETRANSNL80211ERR;
    }

    state->wait_for_ack = false;
    state->nl_socket = nl_socket_alloc();

    if (state->nl_socket == NULL)
    {
        ret = -ENOMEM;
        morsectrl_nl80211_error(transport, ret, "Failed to allocate netlink socket");
        goto exit;
    }

    ret = genl_connect(state->nl_socket);
    if (ret < ETRANSSUCC)
    {
        morsectrl_nl80211_error(transport, ret, "genl_connect failed");
        goto exit_socket_free;
    }
    nl_socket_set_buffer_size(state->nl_socket,
                              NL80211_BUFFER_SIZE,
                              NL80211_BUFFER_SIZE);
#ifdef NETLINK_EXT_ACK
    /* try to set NETLINK_EXT_ACK to 1, ignoring errors */
    option_value = 1;
    setsockopt(nl_socket_get_fd(state->nl_socket),
               SOL_NETLINK, NETLINK_EXT_ACK,
               &option_value, sizeof(option_value));
#endif
    state->nl80211_id = genl_ctrl_resolve(state->nl_socket, "nl80211");
    if (state->nl80211_id < 0)
    {
        ret = -ENOENT;
        morsectrl_nl80211_error(transport, ret, "Failed to get netlink id");
        goto exit_socket_free;
    }

    state->s_cb = nl_cb_alloc(transport->debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
    state->cb = nl_cb_alloc(NL_CB_DEFAULT);
    if (!state->cb || !state->s_cb)
    {
        ret = -ENOMEM;
        morsectrl_nl80211_error(transport, ret, "Failed to allocate netlink callbacks");
        goto exit_cb_free;
    }

    nl_cb_err(state->cb, NL_CB_CUSTOM, morsectrl_nl80211_error_handler, transport);
    nl_cb_set(state->cb, NL_CB_VALID, NL_CB_CUSTOM, morsectrl_nl80211_receive_handler, transport);
    nl_cb_set(state->cb, NL_CB_ACK, NL_CB_CUSTOM, morsectrl_nl80211_ack_handler, transport);
    nl_socket_set_cb(state->nl_socket, state->s_cb);

    return ret;

exit_cb_free:
    nl_cb_put(state->cb);
    nl_cb_put(state->s_cb);
exit_socket_free:
    nl_socket_free(state->nl_socket);
exit:
    return ret;
}

static int morsectrl_nl80211_deinit(struct morsectrl_transport *transport)
{
    struct morsectrl_nl80211_state *state;

    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return -ETRANSNL80211ERR;

    state = &transport->state.nl80211;
    nl_cb_put(state->cb);
    nl_cb_put(state->s_cb);
    nl_socket_free(state->nl_socket);
    memset(state, 0, sizeof(*state));
    return ETRANSSUCC;
}

/**
 * @brief Allocate @ref morsectrl_transport_buff for commands and responses.
 *
 * @param size  Size of command and morse headers.
 * @return      Allocated @ref morsectrl_transport_buff or NULL on failure.
 */
static struct morsectrl_transport_buff *morsectrl_nl80211_alloc(size_t size)
{
    struct morsectrl_transport_buff *buff;

    if (size <= 0)
        return NULL;

    buff = (struct morsectrl_transport_buff *)malloc(sizeof(struct morsectrl_transport_buff));
    if (!buff)
        return NULL;

    buff->capacity = size;
    buff->memblock = (uint8_t *)malloc(buff->capacity);
    if (!buff->memblock)
    {
        free(buff);
        return NULL;
    }

    /* In this case there isn't any framing required in a contiguous block of memory. */
    buff->data = buff->memblock;
    buff->data_len = buff->capacity;

    return buff;
}

static struct morsectrl_transport_buff *morsectrl_nl80211_write_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return NULL;

    return morsectrl_nl80211_alloc(size);
}

static struct morsectrl_transport_buff *morsectrl_nl80211_read_alloc(
    struct morsectrl_transport *transport, size_t size)
{
    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return NULL;

    return morsectrl_nl80211_alloc(size);
}

static int morsectrl_nl80211_send(struct morsectrl_transport *transport,
                                  struct morsectrl_transport_buff *cmd,
                                  struct morsectrl_transport_buff *resp)
{
    int ret = ETRANSSUCC;
    void* header;
    struct morsectrl_nl80211_state *state;
    struct nl_msg* msg;

    if (!transport || (transport->type != MORSECTRL_TRANSPORT_NL80211))
        return -ETRANSNL80211ERR;

    state = &transport->state.nl80211;
    state->data = resp->data;
    state->len = &resp->data_len;

    msg = nlmsg_alloc();
    if (msg == NULL)
    {
        ret = -ENOMEM;
        morsectrl_nl80211_error(transport, ret, "Failed to allocate netlink message");
        goto exit;
    }

    header = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, state->nl80211_id,
                         0, 0, NL80211_CMD_VENDOR, 0);
    if (header == NULL)
    {
        ret = -ENOMEM;
        morsectrl_nl80211_error(transport, ret, "Unable to put msg");
        goto exit_message_free;
    }

    ret = nla_put_u32(msg, NL80211_ATTR_IFINDEX, state->interface_index);
    if (ret < ETRANSSUCC)
    {
        morsectrl_nl80211_error(transport, ret, "Unable to put interface index");
        goto exit_message_free;
    }

    NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_ID, MORSE_OUI);
    NLA_PUT_U32(msg, NL80211_ATTR_VENDOR_SUBCMD, MORSE_VENDOR_CMD_TO_MORSE);
    NLA_PUT(msg, NL80211_ATTR_VENDOR_DATA, cmd->data_len, cmd->data);

    state->wait_for_ack = true;
    ret = nl_send_auto_complete(state->nl_socket, msg);
    if (ret < ETRANSSUCC)
    {
        morsectrl_nl80211_error(transport, ret, "Failed to send_auto_complete");
        goto exit_message_free;
    }

    ret = nl_recvmsgs(state->nl_socket, state->cb);

    if (ret < ETRANSSUCC)
    {
        morsectrl_nl80211_error(transport, ret, "Failed to rcvmsgs");
        goto exit_message_free;
    }

    if (state->wait_for_ack)
    {
        ret = nl_wait_for_ack(state->nl_socket);
        if (ret < 0)
        {
            morsectrl_nl80211_error(transport, ret, "Failed to wait for ACK");
        }
    }

nla_put_failure:
exit_message_free:
    state->wait_for_ack = false;
    nlmsg_free(msg);
exit:
    return ret;
}

const struct morsectrl_transport_ops nl80211_ops = {
    .parse = morsectrl_nl80211_parse,
    .init = morsectrl_nl80211_init,
    .deinit = morsectrl_nl80211_deinit,
    .write_alloc = morsectrl_nl80211_write_alloc,
    .read_alloc = morsectrl_nl80211_read_alloc,
    .send = morsectrl_nl80211_send,
    .reg_read = NULL,
    .reg_write = NULL,
    .mem_read = NULL,
    .mem_write = NULL,
    .raw_read = NULL,
    .raw_write = NULL,
    .raw_read_write = NULL,
    .reset_device = NULL,
};
