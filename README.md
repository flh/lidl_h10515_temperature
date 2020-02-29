# H10515/DCF external temperature sensor

This repository provides code to read temperature measurements sent by
a H10515/DCF wireless sensor. The sensor uses the 433 Mhz frequency. It
was sold at least in France by LIDL in around 2006.

The code reads data from a 433 Mhz receiver plugged on a GPIO input line
(such as a RXB6 receiver on a Raspberry Pi)

## Protocol description

The sensor sends regularly a message containing seven repetitions of the
same frame. Each frame consists of 36 bits which are encoded as follows:
* each bit transmission starts with a short high-state pulse lasting
  about 500 ms,
* 0 is encoded as a low-state signal lasting about 2000 ms,
* 1 is encoded as a low-state signal lasting about 4000 ms,
* frame end is marked by a long low-state signal lasting about 8000 ms.

After the last frame in the transmission, the sensor sends a longer
low signal.

Least significant bit is sent first.

The message payload consists of the first 32 bits. Last 4 bits contain
the message checksum.

### Checksum verification

The checksum is obtained by:
* splitting the message payload in a list of 4 bits integers,
* summing those integers and truncate the result to 4 bits,
* take the bitwise negation.

### Temperature reading

The temperature is encoded as a 12-bit signed integer in bits number 12
to 23 (22 is the most significant bit of the integer, 23 is the sign
bit). The integer represents tenth of degrees reading (i.e., integer 193
is sent when the temperature is 19.3°C).

### Other parts

Other parts of the message seem to be constant during my experiments. It
is yet unknown which bits encode the following information:
* sensor ID (which can be set manually on the sensor as 1, 2 or 3),
* battery level (which is reported on the sensor screen and on the
  weather station receiver).
