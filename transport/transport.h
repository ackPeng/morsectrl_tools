/*
 * Copyright 2022 Morse Micro
 */

#pragma once


#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#ifdef ENABLE_TRANS_FTDI_SPI
#include "ftd2xx.h"
#include "libmpsse_spi.h"
#endif

#define ETRANSSUCC          (0)
#define ETRANSERR           (2)
#define ETRANSNL80211ERR    (3)
#define ETRANSFTDISPIERR    (4)

/* Helper macros :) */
#define TBUFF_TO_CMD(cmd_tbuf, cmdtype) ((cmdtype *)((struct command *)cmd_tbuf->data)->data)
#define TBUFF_TO_RSP(rsp_tbuf, cmdtype) ((cmdtype *)((struct response *)rsp_tbuf->data)->data)

#define MAX_SERIAL_NUMBER_LEN (16)

/** Types of transport supported. */
enum morsectrl_transport_type
{
    MORSECTRL_TRANSPORT_NONE = 0,
#ifdef ENABLE_TRANS_NL80211
    MORSECTRL_TRANSPORT_NL80211,
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
    MORSECTRL_TRANSPORT_FTDI_SPI,
#endif
};

/** Contains memory used to store commands and framing. */
struct morsectrl_transport_buff
{
    /** Memory block to be allocated with room for commands and transport framing. */
    uint8_t *memblock;
    /** Total capacity of the allocated memory block. */
    size_t capacity;
    /**
     * Pointer into the memory block to where the data starts (whichever is first out of commands
     * and framing).
     */
    uint8_t *data;
    /** Current size of the data (can be data or data and framing). */
    size_t data_len;
};

#ifdef ENABLE_TRANS_NL80211
/** State information for the NL80211 interface. */
struct morsectrl_nl80211_state
{
    int interface_index;
    int nl80211_id;
    size_t *len;
    uint8_t *data;
    struct nl_sock* nl_socket;
    struct nl_cb *cb;
    struct nl_cb *s_cb;
    bool wait_for_ack;
};

/** Configuration for the NL80211 interface. */
struct morsectrl_nl80211_cfg
{
    const char *interface_name;
};
#endif

#ifdef ENABLE_TRANS_FTDI_SPI
/** State information for the FTDI SPI interface. */
struct morsectrl_ftdi_spi_state
{
    FT_HANDLE handle;
    FT_HANDLE reset_handle;
};

/** Configuration for the FTDI SPI interface. */
struct morsectrl_ftdi_spi_cfg
{
    ChannelConfig channel;
    UCHAR reset_pin_num;
    UCHAR jtag_reset_pin_num;
    uint32_t reset_ms;
    /** Size taken from FT_DEVICE_LIST_INFO_NODE structure in libmpsse library. */
    char serial_num[MAX_SERIAL_NUMBER_LEN];
};

struct morsectrl_ftdi_spi_chan_info
{
    DWORD spi_loc_id;
    DWORD spi_loc_id_ch;
    DWORD reset_loc_id;
    DWORD reset_loc_id_ch;
};
#endif

/** Transport configuration and state data. */
struct morsectrl_transport
{
    enum morsectrl_transport_type type;
    const struct morsectrl_transport_ops *tops;
    bool debug;
    bool has_reset;
    int (*error_function)(const char *prefix, int error_code, const char *error_msg);
    union
    {
#ifdef ENABLE_TRANS_NL80211
        struct morsectrl_nl80211_cfg nl80211;
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
        struct morsectrl_ftdi_spi_cfg ftdi_spi;
#endif
    } config;

    union
    {
#ifdef ENABLE_TRANS_NL80211
        struct morsectrl_nl80211_state nl80211;
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
        struct morsectrl_ftdi_spi_state ftdi_spi;
#endif
    } state;
};

/**
 * @brief Transport operations
 *
 * Each transport must implement these functions and provide and provide a const copy of this struct
 * which can be used to seamlessly.
 */
struct morsectrl_transport_ops
{
    /** Parse the configuration for the chosen transport. */
    int (*parse)(struct morsectrl_transport *transport,
                 const char *iface_opts,
                 const char *cfg_opts);
    /** Initialise the provided transport. */
    int (*init)(struct morsectrl_transport *transport);
    /** Deinitialise the provided transport. */
    int (*deinit)(struct morsectrl_transport *transport);
    /** Allocate memory for a command. */
    struct morsectrl_transport_buff *(*write_alloc)(
        struct morsectrl_transport *transport, size_t size);
    /** Allocate memory for a response. */
    struct morsectrl_transport_buff *(*read_alloc)(
        struct morsectrl_transport *transport, size_t size);
    /** Send a command and receive a response. */
    int (*send)(struct morsectrl_transport *transport,
                struct morsectrl_transport_buff *cmd,
                struct morsectrl_transport_buff *resp);
    /** Read a 32bit register. */
    int (*reg_read)(struct morsectrl_transport *transport,
                    uint32_t addr, uint32_t *value);
    /** Write a 32bit register. */
    int (*reg_write)(struct morsectrl_transport *transport,
                     uint32_t addr, uint32_t value);
    /** Read a word aligned memory block. */
    int (*mem_read)(struct morsectrl_transport *transport,
                    struct morsectrl_transport_buff *read,
                    uint32_t addr);
    /** Write a word aligned memory block. */
    int (*mem_write)(struct morsectrl_transport *transport,
                     struct morsectrl_transport_buff *write,
                     uint32_t addr);
    /** Perform a raw read from the transport. */
    int (*raw_read)(struct morsectrl_transport *transport,
                    struct morsectrl_transport_buff *read,
                    bool start,
                    bool finish);
    /** Perform a raw write from the transport. */
    int (*raw_write)(struct morsectrl_transport *transport,
                     struct morsectrl_transport_buff *write,
                     bool start,
                     bool finish);
    /** Perform a raw read and write simultaneously from the transport. */
    int (*raw_read_write)(struct morsectrl_transport *transport,
                          struct morsectrl_transport_buff *read,
                          struct morsectrl_transport_buff *write,
                          bool start,
                          bool finish);
    /** Reset the device. */
    int (*reset_device)(struct morsectrl_transport *transport);
};

/** Special transport string for testing, don't tell anyone. */
extern const char *transport_none;
#ifdef ENABLE_TRANS_NL80211
/** NL80211 transport string. */
extern const char *transport_nl80211;
#endif
#ifdef ENABLE_TRANS_NL80211
/** NL80211 transport string. */
extern const char *transport_ftdi_spi;
#endif

/**
 * @brief Parses the commandline options to set the correct transport and fill the configuration.
 *
 * @param transport     Pointer to uninitialised transport struct.
 * @param trans_opts    Transport string from the commandline.
 * @param iface_opts    Interface string from the commandline.
 * @param cfg_opts      Configuration string from the commandline.
 * @return              0 on success or relevant error.
 */
int morsectrl_transport_parse(struct morsectrl_transport *transport,
                              const char *trans_opts,
                              const char *iface_opts,
                              const char *cfg_opts);

/**
 * @brief Initialises the transport.
 *
 * @param transport Initalised transport (has parse the configuration)
 * @return          0 on success or relevant error.
 */
int morsectrl_transport_init(struct morsectrl_transport *transport);

/**
 * @brief Deinitialises the transport
 *
 * @param transport Transport to deinit.
 * @return          0 on success or relevant error.
 */
int morsectrl_transport_deinit(struct morsectrl_transport *transport);

/**
 * @brief Allocates memory for a command to send using the provided transport.
 *
 * @param transport Transport to allocate command memory for.
 * @param size      Size of the commnd to allocate memory for. Includes the morse command header.
 * @return          the allocated @ref morsectrl_transport_buff or NULL on failure.
 */
struct morsectrl_transport_buff *morsectrl_transport_cmd_alloc(
    struct morsectrl_transport *transport, size_t size);

/**
 * @brief Allocates memory for a response to send using the provided transport.
 *
 * @param transport Transport to allocate command memory for.
 * @param size      Size of the response to allocate memory for. Includes the morse response header.
 * @return          the allocated @ref morsectrl_transport_buff or NULL on failure.
 */
struct morsectrl_transport_buff *morsectrl_transport_resp_alloc(
    struct morsectrl_transport *transport, size_t size);

/**
 * @brief Allocates memory for writing raw data using the provided transport.
 *
 * @param transport Transport to allocate command memory for.
 * @param size      Size of the data to allocate memory for.
 * @return          the allocated @ref morsectrl_transport_buff or NULL on failure.
 */
struct morsectrl_transport_buff *morsectrl_transport_raw_write_alloc(
    struct morsectrl_transport *transport, size_t size);

/**
 * @brief Allocates memory for reading raw data using the provided transport.
 *
 * @param transport Transport to allocate command memory for.
 * @param size      Size of the data to allocate memory for.
 * @return          the allocated @ref morsectrl_transport_buff or NULL on failure.
 */
struct morsectrl_transport_buff *morsectrl_transport_raw_read_alloc(
    struct morsectrl_transport *transport, size_t size);

/**
 * @brief Frees memory for a @ref morsectrl_transport_buff.
 *
 * @param transport Transport to free memory for.
 * @return          0 on success or relevant error.
 */
int morsectrl_transport_buff_free(struct morsectrl_transport_buff *buff);

int morsectrl_transport_reg_read(struct morsectrl_transport *transport,
                                 uint32_t addr, uint32_t *value);

int morsectrl_transport_reg_write(struct morsectrl_transport *transport,
                                  uint32_t addr, uint32_t value);

int morsectrl_transport_mem_read(struct morsectrl_transport *transport,
                                 struct morsectrl_transport_buff *read,
                                 uint32_t addr);

int morsectrl_transport_mem_write(struct morsectrl_transport *transport,
                                  struct morsectrl_transport_buff *write,
                                  uint32_t addr);

/**
 * @brief Send a command using the specified transport.
 *
 * @param transport Transport to send the command on.
 * @param cmd       Buffer containing command to send.
 * @param resp      Buffer to received response into.
 * @return          0 on success or relevant error.
 */
int morsectrl_transport_send(struct morsectrl_transport *transport,
                             struct morsectrl_transport_buff *cmd,
                             struct morsectrl_transport_buff *resp);

/**
 * @brief Reads raw data from the transport.
 *
 * @param transport Transport to read raw data from.
 * @param read      Buffer to read data into.
 * @param finish    Finishes the transaction if true, otherwise leave open for
 *                  more raw reads/writes/read writes.
 * @return int      0 on success or relevenat error.
 */
int morsectrl_transport_raw_read(struct morsectrl_transport *transport,
                                 struct morsectrl_transport_buff *read,
                                 bool start,
                                 bool finish);

/**
 * @brief Writes raw data to the transport.
 *
 * @param transport Transport to read raw data from.
 * @param write     Buffer to write data from.
 * @param finish    Finishes the transaction if true, otherwise leave open for
 *                  more raw reads/writes/read writes.
 * @return int      0 on success or relevenat error.
 */
int morsectrl_transport_raw_write(struct morsectrl_transport *transport,
                                  struct morsectrl_transport_buff *write,
                                  bool start,
                                  bool finish);

/**
 * @brief Reads and writes raw data to/from the transport.
 *
 * @note If read and write buffer lengths differ, behaviour is transport dependent.
 *
 * @param transport Transport to read/write raw data from/to.
 * @param read      Buffer to read data into.
 * @param write     Buffer to write data from.
 * @param finish    Finishes the transaction if true, otherwise leave open for
 *                  more raw reads/writes/read writes.
 * @return int      0 on success or relevenat error.
 */
int morsectrl_transport_raw_read_write(struct morsectrl_transport *transport,
                                       struct morsectrl_transport_buff *read,
                                       struct morsectrl_transport_buff *write,
                                       bool start,
                                       bool finish);

/**
 * @brief Reset a device using transport's hardware methods.
 *
 * @param transport Transport to perform hard reset.
 * @return int      0 on success or relevant error.
 */
int morsectrl_transport_reset_device(struct morsectrl_transport *transport);

/**
 * @brief Get the interface name
 *
 * @param transport Transport
 *
 * @return Interface name, or NULL if not defined.
 */
static inline const char *morsectrl_transport_get_ifname(struct morsectrl_transport *transport)
{
#ifdef ENABLE_TRANS_NL80211
    if (transport->type == MORSECTRL_TRANSPORT_NL80211)
    {
        return transport->config.nl80211.interface_name;
    }
#endif

    return NULL;
}

/**
 * @brief Set the length of the data actually used in a command
 *
 * @param tbuff Command transport buffer
 * @param length Length of command data
 */
void morsectrl_transport_set_cmd_data_length(struct morsectrl_transport_buff *tbuff,
                                             uint16_t length);

#ifdef ENABLE_TRANS_NL80211
/** NL80211 transport operations. */
extern const struct morsectrl_transport_ops nl80211_ops;
#endif
#ifdef ENABLE_TRANS_FTDI_SPI
/** FTDI SPI transport operations. */
extern const struct morsectrl_transport_ops ftdi_spi_ops;
#endif
