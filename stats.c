/*
 * Copyright 2020 Morse Micro
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>

#include "portable_endian.h"
#include "command.h"
#include "elf_file.h"
#include "offchip_statistics.h"
#include "stats_format.h"
#include "utilities.h"
#include "transport/transport.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif


/* Read and return a single word from a file.
 * The result is always null terminated.
 */
static char *get_word_from_file(const char *path, char *word, size_t n)
{
    FILE *infile = fopen(path, "r");
    if (infile)
    {
        fgets(word, n, infile); /* fgets always null terminates */
        fclose(infile);
        return strip(word);
    }
    return NULL;
}

/* Get the firmware path from the driver debugfs file
 * ...using phy name from sysfs file
 * ...using wlan name from morsectrl struct
 * The append it to the firmware path.
 *
 * If any of these steps fail, nothing is written to the firmware path.
 */
static void get_override_firmware_path(struct morsectrl *mors, char *firmware_full_path, size_t n)
{
    char path_buf[MAX_PATH];
    char content_buf[MAX_PATH];
    const char *sysfs_path_fmt = "/sys/class/net/%s/phy80211/name";
    const char *debugfs_path_fmt = "/sys/kernel/debug/ieee80211/%s/morse/firmware_path";
    const char *firmware_path_fmt = "/lib/firmware/%s";

    char *phy_name;
    char *firmware_path;

#ifdef CONFIG_TRANS_NL80211
    if (mors->transport.type == MORSECTRL_TRANSPORT_NL80211)
    {
        snprintf(path_buf, sizeof(path_buf), sysfs_path_fmt,
                mors->transport.config.nl80211.interface_name);
    }
    else
    {
#else
    {
#endif
        snprintf(path_buf, sizeof(path_buf), sysfs_path_fmt, DEFAULT_INTERFACE_NAME);
    }

    phy_name = get_word_from_file(path_buf, content_buf, sizeof(content_buf));
    if (phy_name)
    {
        snprintf(path_buf, sizeof(path_buf), debugfs_path_fmt, phy_name);
        firmware_path = get_word_from_file(path_buf, content_buf, sizeof(content_buf));
        if (firmware_path)
        {
            snprintf(firmware_full_path, n, firmware_path_fmt, firmware_path);
        }
    }
}

static int load_offchip_statistics(struct morsectrl *mors, const char *filename)
{
    FILE *infile;
    char firmware_path[MAX_PATH] = "/lib/firmware/morse/mm6108.bin";

    if (!filename)
    {
        filename = firmware_path;
        get_override_firmware_path(mors, firmware_path, sizeof(firmware_path));
    }

    infile = fopen(filename, "rb");
    if (infile)
    {
        uint8_t *buf = NULL;

        load_file(infile, &buf);
        if (buf)
        {
            morse_stats_load(&mors->stats, &mors->n_stats, buf);
            free(buf);
        }
        fclose(infile);
    }
    else
    {
        mctrl_err("Error - could not open %s to read stats metadata\n", filename);
        return -1;
    }
    return 0;
}

static void usage(struct morsectrl *mors)
{
    mctrl_print("\tstats [options]\t\treads/resets stats (for all cores if none were mentioned)\n");
    mctrl_print("\t\t-a\t\tApp core\n");
    mctrl_print("\t\t-m\t\tMac core\n");
    mctrl_print("\t\t-u\t\tUphy core\n");
    mctrl_print("\t\t-j\t\toutputs stats in a json format\n");
    mctrl_print("\t\t-p\t\toutputs stats in a human-readable json format\n");
    mctrl_print("\t\t-r\t\tresets stats for mentioned cores (resets all if none were mentioned)\n");
    mctrl_print("\t\t-f \"key\"\tfilters stats according to the given key (case sensetive)\n");
    mctrl_print("\t\t-s filename\tthe location of the firmware ELF\n");
}


int morsectrl_stats_cmd(struct morsectrl *mors, int cmd, int reset,
                            const char *filter_string, enum format_type format_val)
{
    int ret = -1;
    int resp_sz;
    const struct format_table *table;
    struct stats_response *resp;
    struct morsectrl_transport_buff *cmd_tbuff =
        morsectrl_transport_cmd_alloc(&mors->transport, 0);
    struct morsectrl_transport_buff *rsp_tbuff =
        morsectrl_transport_resp_alloc(&mors->transport, sizeof(*resp));

    if (!cmd_tbuff || !rsp_tbuff)
        goto exit;

    resp = TBUFF_TO_RSP(rsp_tbuff, struct stats_response);

    if (reset)
        cmd += 1;

    ret = morsectrl_send_command(&mors->transport, cmd, cmd_tbuff, rsp_tbuff);
    resp_sz = rsp_tbuff->data_len - sizeof(struct response);

    if (ret)
    {
        /* Try the deprecated command */
        ret = morsectrl_send_command(&mors->transport, OLD_STATS_COMMAND_MASK & cmd,
                                     cmd_tbuff, rsp_tbuff);
        resp_sz = rsp_tbuff->data_len - sizeof(struct response);
        if (!reset && !ret)
        {
            mctrl_print("%s", resp->stats);
        }
        goto exit;
    }

    if (!reset && !ret)
    {
        uint8_t *buf = (uint8_t *)resp->stats;

        switch (format_val)
        {
            case FORMAT_REGULAR:
            {
                table = stats_format_regular_get_formatter_table();
                break;
            }
            case FORMAT_JSON_PPRINT:
            {
                stats_format_json_set_pprint(true);
                /* fall through */
            }
            case FORMAT_JSON:
            {
                table = stats_format_json_get_formatter_table();
                break;
            }
            default:
                ret = -1;
                goto exit;
        }

        while (resp_sz > STATS_TLV_OVERHEAD )
        {
            stats_tlv_tag_t tag =  *((stats_tlv_tag_t *)buf);
            buf += sizeof(stats_tlv_tag_t);

            stats_tlv_len_t len =  *((stats_tlv_len_t *)buf);
            buf += sizeof(stats_tlv_len_t);

            if ((len > resp_sz) || (len == 0))
            {
                mctrl_err("error: malformed TLV (tag %d/0x%x, len %u/0x%x, size %u)\n",
                        tag, tag, len, len, resp_sz);
                break;
            }

            struct statistics_offchip_data *offchip = get_stats_offchip(mors, tag);
            if (offchip)
            {
                if ((offchip->format == MORSE_STATS_FMT_DEC) &&
                        !strncmp(offchip->type_str, "uint", 4))
                {
                    offchip->format = MORSE_STATS_FMT_U_DEC;
                }

                if (!filter_string || !strcmp(offchip->key, filter_string))
                {
                    if (format_val == FORMAT_JSON || format_val == FORMAT_JSON_PPRINT)
                    {
                        stats_format_json_init();
                    }

                    if (offchip->format > MORSE_STATS_FMT_LAST)
                    {
                        offchip->format = MORSE_STATS_FMT_LAST;
                    }

                    table->format_func[offchip->format]((const char *) offchip->key,
                                                                (const uint8_t *)buf, len);
                }
            }
            else
            {
                mctrl_err("UNKOWN KEY for tag %d: ", tag);
                hexdump(buf, len);
                mctrl_err("\n");
            }
            buf += len;

            resp_sz -= (STATS_TLV_OVERHEAD + len);
        }
    }
exit:
    morsectrl_transport_buff_free(cmd_tbuff);
    morsectrl_transport_buff_free(rsp_tbuff);
    return ret;
}

static void dump_stats_types(struct morsectrl *mors)
{
    int ii;

    mctrl_print("Stats types\n");
    for (ii = 0; ii < mors->n_stats; ii++)
    {
        mctrl_print("Type: %s\n", mors->stats[ii].type_str);
        mctrl_print("Name: %s\n", mors->stats[ii].name);
        mctrl_print("Key: %s\n\n", mors->stats[ii].key);
    }
}

int stats(struct morsectrl *mors, int argc, char *argv[])
{
    int option;
    int ret = 0;
    bool reset = false, app_c = false, mac_c = false, uph_c = false;
    const char *filter_string = NULL;
    const char *firmware_path = NULL;
    enum format_type format = FORMAT_REGULAR;

    if (argc == 0)
    {
        usage(mors);
        return 0;
    }

    while ((option = getopt(argc, argv, "amurjpf:s:")) != -1)
    {
        switch (option)
        {
            case 'a' :
                app_c = true;
                break;
            case 'm' :
                mac_c = true;
                break;
            case 'u' :
                uph_c = true;
                break;
            case 'r' :
                reset = true;
                break;
            case 'j' :
                format = FORMAT_JSON;
                break;
            case 'p' :
                format = FORMAT_JSON_PPRINT;
                break;
            case 'f' :
                filter_string = optarg;
                break;
            case 's' :
                firmware_path = optarg;
                break;
            default :
                usage(mors);
                return -1;
        }
    }
    if (argc > optind)
    {
        mctrl_err("Invalid argument %s\n", argv[optind]);
        usage(mors);
        return -1;
    }

    ret = load_offchip_statistics(mors, firmware_path);

    if (ret)
    {
        goto exit_stats;
    }

    if (mors->debug)
        dump_stats_types(mors);

    /* If no core selected then enable all. */
    if ((!app_c) && (!mac_c) && (!uph_c))
    {
        app_c = true;
        mac_c = true;
        uph_c = true;
    }

    if (format == FORMAT_JSON)
    {
        mctrl_print("{");
    }
    else if (format == FORMAT_JSON_PPRINT)
    {
        mctrl_print("{\n");
    }

    if (app_c)
    {
        ret = morsectrl_stats_cmd(mors, MORSE_COMMAND_APP_STATS_LOG, reset,
            filter_string, format);
        if (ret) goto exit_stats;
    }
    if (mac_c)
    {
        ret = morsectrl_stats_cmd(mors, MORSE_COMMAND_MAC_STATS_LOG, reset,
            filter_string, format);
        if (ret) goto exit_stats;
    }
    if (uph_c)
    {
        ret = morsectrl_stats_cmd(mors, MORSE_COMMAND_UPHY_STATS_LOG, reset,
            filter_string, format);
        if (ret) goto exit_stats;
    }

    if (format == FORMAT_JSON)
    {
        mctrl_print("}\n");
    }
    else if (format == FORMAT_JSON_PPRINT)
    {
        mctrl_print("\n}\n");
    }

exit_stats:
    if (ret < 0)
    {
        mctrl_err("Command stats error (%d)\n", ret);
    }

    return ret;
}
