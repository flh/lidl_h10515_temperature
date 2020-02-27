#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void lidl_report_to_jeedom(char *base_url, char *apikey, int command_id, float temperature) {
  CURL* curl = curl_easy_init();

  char report_url_tpl[] = "%s?plugin=virtual&apikey=%s&type=virtual&id=%d&value=%f";
  // Assume command_id can be encoded with only ten digits. Temperature
  // is sent as XX.X (decimals truncated after the first one).
  if(command_id >= 1e10 || temperature >= 100) {
    // TODO log
    return;
  }
  size_t report_url_len = strlen(base_url) + strlen(apikey) + strlen(report_url_tpl) + 10 + 4;
  char *report_url = malloc(sizeof(char) * (report_url_len + 1));
  if(snprintf(report_url, report_url_len, report_url_tpl, apikey, command_id, temperature) < 0) {
    // TODO report
    return;
  }
  curl_easy_setopt(curl, CURLOPT_URL, report_url);

  curl_easy_perform(curl);
  curl_easy_cleanup(curl);
}
