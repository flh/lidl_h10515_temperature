#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <argp.h>
#include <gpiod.h>

#include "jeedom_report.h"

#define LIDL_STATE_NOISE 0
#define LIDL_STATE_PULSE 1
#define LIDL_STATE_SHORT 2
#define LIDL_STATE_LONG 3
#define LIDL_STATE_END 4

#define GPIO_CONSUMER "lidltemp"

uint_least64_t lidl_poll_signal(struct gpiod_chip *gpiochip, struct gpiod_line *gpioline) {
  gpiod_line_request_input(gpioline, GPIO_CONSUMER);
  gpiod_line_request_both_edges_events(gpioline, GPIO_CONSUMER);
  
  int signal_state = LIDL_STATE_NOISE;
  struct timespec prev_ts;
  uint_least64_t signal = 0;
  unsigned int signal_length = 0;
  
  for(;;) {
    struct gpiod_line_event gpioevent;
    gpiod_line_event_wait(gpioline, NULL);
    gpiod_line_event_read(gpioline, &gpioevent);
    long duration = gpioevent.ts.tv_nsec - prev_ts.tv_nsec;
    prev_ts = gpioevent.ts;
  
    if(gpioevent.event_type == GPIOD_LINE_EVENT_RISING_EDGE && signal_state == LIDL_STATE_PULSE) {
      if(duration > 1700000 && duration < 2500000) {
        signal_state = LIDL_STATE_SHORT;
        signal *= 2;
        signal_length++;
      }
      if(duration > 3800000 && duration < 4200000) {
        signal_state = LIDL_STATE_LONG;
        signal = signal * 2 + 1;
        signal_length++;
      }
      if(duration > 7000000 && duration < 9100000) {
        signal_state = LIDL_STATE_END;
        if(signal_length == 36) {
          return signal;
        }
      }
      continue;
    }
    if(gpioevent.event_type == GPIOD_LINE_EVENT_FALLING_EDGE &&
        duration > 480000) {
      signal_state = LIDL_STATE_PULSE;
      continue;
    }

    signal_state = LIDL_STATE_NOISE;
    signal_length = 0;
    signal = 0;
  }
}

float lidl_decode_temperature(uint_least64_t signal) {
  unsigned int raw_temperature = 0;
  for(char i = 0; i < 11; i++) {
    raw_temperature = raw_temperature * 2 + ((signal >> (i + 12)) & 1);
  }
  if(signal >> 24 & 1 == 0) {
    return (float)raw_temperature / 10.;
  }
  else {
    return (float)~raw_temperature / (-10.);
  }
}

bool lidl_checksum(uint_least64_t signal) {
  unsigned char sum = 0;
  for(int i = 0; i < 35; i += 4) {
    unsigned char segment = 0;
    for(int j = i + 3; j >= i; j--) {
      segment = segment * 2 + ((signal >> j) & 1);
    }
    sum = (sum + segment) & 0xF;
  }
  return sum == 0;
}

static struct argp_option options[] = {
  {0, 0, 0, 0, "Wireless access settings:", 1},
    {"device", 'd', "GPIO", 0, "GPIO device to use.", 1},
    {"line", 'l', "LINE", 0, "Line to read from the GPIO device.", 1},
  {0, 0, 0, 0, "Jeedom reporting settings:", 2},
    {"report_url", 'r', "URL", 0, "Jeedom base report URL.", 2},
    {"apikey", 777, "KEY", 0, "Jeedom API key", 2},
    {"command", 778, "CMD_ID", 0, "Jeedom command ID", 2},
    {0}
};
const char *argp_program_version = "1";

struct arguments {
  char *gpio_device;
  int gpio_line;
  char *jeedom_url;
  char *jeedom_apikey;
  int jeedom_cmdid;
};

static int parse_option(int key, char *arg, struct argp_state *state) {
  struct arguments *a = state->input;
  switch(key) {
    case 'd':
      a->gpio_device = arg;
      break;
    case 'l':
      a->gpio_line = atoi(arg);
      break;
    case 'r':
      a->jeedom_url = arg;
      break;
    case 777:
      a->jeedom_apikey = arg;
      break;
    case 778:
      a->jeedom_cmdid = atoi(arg);
      break;
    case ARGP_KEY_END:
      if(a->jeedom_url && (!a->jeedom_apikey || a->jeedom_cmdid < 0)) {
        argp_error(state, "both API key and command-id must be specified to enable Jeedom reporting\n");
        return EINVAL;
          }
      break;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  struct argp argp = { options, parse_option };
  struct arguments arguments = { "/dev/gpiochip0", 5, NULL, NULL, -1 };
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  struct gpiod_chip *gpiochip = gpiod_chip_open(arguments.gpio_device);
  struct gpiod_line *gpioline = gpiod_chip_get_line(gpiochip, arguments.gpio_line);

  for(;;) {
    uint_least64_t signal = lidl_poll_signal(gpiochip, gpioline);
    if(lidl_checksum(signal)) {
      float temperature = lidl_decode_temperature(signal);

      if(arguments.jeedom_url) {
        lidl_report_to_jeedom(arguments.jeedom_url, arguments.jeedom_apikey, arguments.jeedom_cmdid, temperature);
      }
    }
  }
}
