override MORSECTRL_VERSION_STRING = "rel_1_9_4_2023_Nov_02"
DEFAULT_INTERFACE_NAME ?= "wlan0"

MORSECTRL_CFLAGS = $(CFLAGS)
MORSECTRL_CFLAGS += -Wall -Werror
MORSECTRL_CFLAGS += -DMORSECTRL_VERSION_STRING="\"$(MORSECTRL_VERSION_STRING)\""
MORSECTRL_CFLAGS += -DDEFAULT_INTERFACE_NAME="\"$(DEFAULT_INTERFACE_NAME)\""
MORSECTRL_LDFLAGS = $(LDFLAGS)

DEPS := $(wildcard *.h)
DEPS += $(wildcard */*.h)

SRCS := morsectrl.c
SRCS += config_file.c
SRCS += elf_file.c
SRCS += offchip_statistics.c
SRCS += command.c
SRCS += version.c
SRCS += hw_version.c
SRCS += stats.c
SRCS += channel.c
SRCS += bsscolor.c
SRCS += utilities.c
SRCS += ampdu.c
SRCS += raw.c
SRCS += health.c
SRCS += cts_self_ps.c
SRCS += long_sleep.c
SRCS += duty_cycle.c
SRCS += stats_format_regular.c
SRCS += stats_format_json.c
SRCS += coredump.c
SRCS += opclass.c
SRCS += tx_pkt_lifetime_us.c
SRCS += maxampdulen.c
SRCS += macaddr.c
SRCS += reset.c
SRCS += wakeaction.c
SRCS += standby.c
SRCS += mpsw.c
SRCS += dhcpc.c
SRCS += keep_alive.c
SRCS += vendor_ie.c
SRCS += twt.c
SRCS += cac.c
SRCS += ecsa.c
SRCS += mbssid.c
SRCS += mesh_config.c
SRCS += mbca.c
SRCS += params.c
SRCS += uapsd.c

SRCS += transport/transport.c

WIN_LIB_SRCS += win/strsep.c
LINUX_SRCS += gpioctrl.c

LINUX_LDFLAGS += -lm
ifeq ($(CONFIG_MORSE_STATIC),1)
	MORSECTRL_LDFLAGS += -static
endif

ifeq ($(CONFIG_MORSE_TRANS_NL80211),1)
	ifeq ($(CFLAGS),)
		LINUX_CFLAGS += -I${SYSROOT}/usr/include/libnl3
	endif

	LINUX_LDFLAGS += -lnl-3 -lnl-genl-3 -lpthread
	LINUX_CFLAGS += -DENABLE_TRANS_NL80211
	LINUX_SRCS += transport/nl80211.c
endif

ifeq ($(CONFIG_MORSE_TRANS_FTDI_SPI),1)
	SRCS += transport/ftdi_spi.c
	SRCS += transport/sdio_over_spi.c
	LIB_SRCS += transport/libmpsse/source/ftdi_i2c.c
	LIB_SRCS += transport/libmpsse/source/ftdi_infra.c
	LIB_SRCS += transport/libmpsse/source/ftdi_mid.c
	LIB_SRCS += transport/libmpsse/source/ftdi_spi.c
	LIB_SRCS += transport/libmpsse/source/memcpy.c

	MAJOR_VERSION = 1
	MINOR_VERSION = 0
	BUILD_VERSION = 3
	MORSECTRL_CFLAGS += -DFTDIMPSSE_STATIC
	MORSECTRL_CFLAGS += -DENABLE_TRANS_FTDI_SPI
	MORSECTRL_CFLAGS += -DFT_VER_MAJOR=$(MAJOR_VERSION)
	MORSECTRL_CFLAGS += -DFT_VER_MINOR=$(MINOR_VERSION)
	MORSECTRL_CFLAGS += -DFT_VER_BUILD=$(BUILD_VERSION)
	MORSECTRL_CFLAGS += -Itransport/libmpsse/include
	MORSECTRL_CFLAGS += -Itransport/libmpsse/libftd2xx
	MORSECTRL_CFLAGS += -Itransport/libmpsse/source
	MORSECTRL_LDFLAGS += -Wl,--wrap=memcpy
	WIN_LDFLAGS += -lws2_32

	LINUX_CFLAGS += -D_GNU_SOURCE
	LINUX_LDFLAGS += -lpthread -lrt -ldl
endif

MORSE_CLI_CFLAGS = $(MORSECTRL_CFLAGS)
MORSE_CLI_LDFLAGS = $(MORSECTRL_LDFLAGS)

MORSE_CLI_CFLAGS += -DMORSE_CLIENT

# Set Windows Vista as the minimum supported windows version
WIN_CFLAGS += -DMORSE_WIN_BUILD -D__USE_MINGW_ANSI_STDIO -D_WIN32_WINNT=0x0600
WIN_CC ?= x86_64-w64-mingw32-gcc

# Commands to be enabled in morse_cli
# eg.: MORSE_CLI_CFLAGS += -DENABLE_CMD_BW
# That will enable BW command

MORSE_CLI_CFLAGS += -DENABLE_CMD_VERSION
MORSE_CLI_CFLAGS += -DENABLE_CMD_RESET
MORSE_CLI_CFLAGS += -DENABLE_CMD_STATS
MORSE_CLI_CFLAGS += -DENABLE_CMD_CHANNEL
MORSE_CLI_CFLAGS += -DENABLE_CMD_BSSCOLOR
MORSE_CLI_CFLAGS += -DENABLE_CMD_AMPDU
MORSE_CLI_CFLAGS += -DENABLE_CMD_RAW
MORSE_CLI_CFLAGS += -DENABLE_CMD_HEALTHCHECK
MORSE_CLI_CFLAGS += -DENABLE_CMD_CTS_SELF_PS
MORSE_CLI_CFLAGS += -DENABLE_CMD_LONG_SLEEP
MORSE_CLI_CFLAGS += -DENABLE_CMD_DUTY_CYCLE
MORSE_CLI_CFLAGS += -DENABLE_CMD_COREDUMP
MORSE_CLI_CFLAGS += -DENABLE_CMD_OPCLASS
MORSE_CLI_CFLAGS += -DENABLE_CMD_TX_PKT_LIFETIME_US
MORSE_CLI_CFLAGS += -DENABLE_CMD_SET_MAX_AMPDU_LENGTH
MORSE_CLI_CFLAGS += -DENABLE_CMD_MACADDR
MORSE_CLI_CFLAGS += -DENABLE_CMD_SEND_WAKE_ACTION_FRAME
MORSE_CLI_CFLAGS += -DENABLE_CMD_MPSW
MORSE_CLI_CFLAGS += -DENABLE_CMD_STANDBY
MORSE_CLI_CFLAGS += -DENABLE_CMD_DHCPC
MORSE_CLI_CFLAGS += -DENABLE_CMD_KEEPALIVE_OFFLOAD
MORSE_CLI_CFLAGS += -DENABLE_CMD_VENDOR_IE
MORSE_CLI_CFLAGS += -DENABLE_CMD_TWT
MORSE_CLI_CFLAGS += -DENABLE_CMD_CAC
MORSE_CLI_CFLAGS += -DENABLE_CMD_ECSA_INFO
MORSE_CLI_CFLAGS += -DENABLE_CMD_MBSSID_IE_INFO
MORSE_CLI_CFLAGS += -DENABLE_CMD_MESH_CONFIG
MORSE_CLI_CFLAGS += -DENABLE_CMD_MBCA
MORSE_CLI_CFLAGS += -DENABLE_CMD_PARAM_GET_SET
MORSE_CLI_CFLAGS += -DENABLE_CMD_UAPSD_CONFIG

SRCS += $(LIB_SRCS)
WIN_SRCS += $(WIN_LIB_SRCS)
LINUX_SRCS += $(LINUX_LIB_SRCS)

all: morse_cli

clean:
	rm -rf morsectrl morse_cli *.exe output
	find . -iname '*.o' -exec rm {} \;

include morsectrl.mk

CLIENT_OBJS = $(patsubst %.c, %_cli.o, $(SRCS) $(LINUX_SRCS))
CLIENT_OBJS_WIN = $(patsubst %.c, %_cli_win.o, $(SRCS) $(WIN_SRCS))

%_cli.o: %.c $(DEPS)
	$(CC) $(MORSE_CLI_CFLAGS) $(LINUX_CFLAGS) -c -o $@ $<

morse_cli: $(CLIENT_OBJS)
	$(CC) $(MORSE_CLI_CFLAGS) $(LINUX_CFLAGS) -o $@ $^ \
	$(MORSE_CLI_LDFLAGS) $(LINUX_LDFLAGS)


%_cli_win.o: %.c $(DEPS)
	$(WIN_CC) $(MORSE_CLI_CFLAGS) $(WIN_CFLAGS) -c -o $@ $<

morse_cli_win: $(CLIENT_OBJS_WIN)
	$(WIN_CC) $(MORSE_CLI_CFLAGS) $(WIN_CFLAGS) -o morse_cli $^ \
	$(MORSE_CLI_LDFLAGS) $(WIN_LDFLAGS)

install_cli:
	cp morse_cli /usr/bin
