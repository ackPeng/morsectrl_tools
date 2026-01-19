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

/** Structure for a get version confirm */
struct PACKED get_version_response
{
    /** Length of version */
    int32_t length;
    /** The version string */
    uint8_t version[128];
};

static void usage(struct morsectrl *mors) {
    mctrl_print("\tversion\t\t\tprints firmware version\n");
}

int version(struct morsectrl *mors, int argc, char *argv[])
{
    int ret = -1;
    uint32_t len;
    struct get_version_response *version;
    struct morsectrl_transport_buff *cmd_tbuff;
    struct morsectrl_transport_buff *rsp_tbuff;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    cmd_tbuff = morsectrl_transport_cmd_alloc(&mors->transport, 0);
    rsp_tbuff = morsectrl_transport_resp_alloc(&mors->transport, sizeof(*version));

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    version = TBUFF_TO_RSP(rsp_tbuff, struct get_version_response);

    if (argc > 1)
    {
        mctrl_err("Invalid command parameters\n");
        usage(mors);
        ret = -1;
        goto exit;
    }

    ret = morsectrl_send_command(&mors->transport, MORSE_COMMAND_GET_VERSION,
                                 cmd_tbuff, rsp_tbuff);
exit:
    if (ret < 0)
    {
        mctrl_err("Get firmware version failed (%d)\n", ret);
    }
    else
    {
        len = le32toh(version->length);
        version->version[len] = '\0';
        mctrl_print("FW Version: %s\n", version->version);
    }

    morsectrl_transport_buff_free(cmd_tbuff);
    morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}
