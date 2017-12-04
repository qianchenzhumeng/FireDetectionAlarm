CONTIKI = /home/user/contiki
CONTIKI_PROJECT = sender sink
#APPS = powertrace collect-view
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\" -DDEBUG
all: $(CONTIKI_PROJECT)
#PROJECT_SOURCEFILES += address-verify.c get-address.c collect-common.c
PROJECT_SOURCEFILES += address-verify.c get-address.c fire-detect.c
CONTIKI_WITH_RIME = 1
#NETSTACK_CONF_WITH_IPV6 = 0
UIP_CONF_IPV6_RPL = 0
#CONTIKI_WITH_IPV6 = 0
#WITH_UIP = 0
WITH_SLIP = 0
include $(CONTIKI)/Makefile.include

