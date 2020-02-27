.PHONY: all

all: lidl_temperature

lidl_temperature: lidl_temperature.o jeedom_report.o
	$(CC) -o $@ $^ $(shell pkg-config --cflags --libs libcurl libgpiod)
