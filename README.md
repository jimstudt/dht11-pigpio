# DHT11 code for Raspberry Pi

A search of the internet will find DHT11 code for a raspberry pi which
polls the GPIO pin to read the data stream. This doesn't work well, or
at all, depending on your cpu load. Your reader process gets unschduled
and you can't resolve when the pin changed.

This code uses pigpio to sample the pins with DMA and become immune to
scheduling anomolies.

**Note:** pigpio means you need to run as root. Maybe there is a finer
grained capability, but I didn't worry about it.

## Usage

````
$ make install
$ sudo dht11-pigpio   # print about 20 samples as fast as they come
got 40 digits 46.0 hum, 21.0 temp, OK    # notice the first one is stale
got 40 digits 58.0 hum, 20.0 temp, OK
got 40 digits 55.0 hum, 20.0 temp, OK
got 40 digits 53.0 hum, 20.0 temp, OK
...

$ sudo dht11-json     # make a json record
{"temperature":21.0, "humidity":40.0}
````

## License

SPDX-License-Identifier: BSD-3-Clause

