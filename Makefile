#
# Copyright 2018, Jim Studt jim@studt.net
# SPDX-License-Identifier: BSD-3-Clause
#

BINDIR=/usr/local/bin

all : dht11-pigpio dht11-json

dht11 : LDLIBS += -lwiringPi

dht11-pigpio : LDLIBS += -lpigpio -lpthread
dht11-pigpio : CFLAGS += -std=c11 -Wall -Werror

dht11-json : LDLIBS += -lpigpio -lpthread
dht11-json : CFLAGS += -std=c11 -Wall -Werror


install: ${BINDIR}/dht11-json

${BINDIR}/dht11-json: dht11-json
	install $< ${BINDIR}
