/*
 * coapTask.c
 *
 *  Created on: Dec 4, 2024
 *      Author: stred
 */

#include "main.h"
#include "logging_levels.h"
#define LOG_LEVEL    LOG_INFO
#include "FreeRTOS.h"
#include "task.h"

#include "lwip/sockets.h"

#include <stdio.h>
#include <stdbool.h>
#include <strings.h>

#include "coap.h"

#define PORT 5683
#define DEBUG

void vCoApTask(void *pvParameters)
{
  int fd;
#ifdef IPV6
    struct sockaddr_in6 servaddr, cliaddr;
#else /* IPV6 */
  struct sockaddr_in servaddr, cliaddr;
#endif /* IPV6 */
  uint8_t buf[4096];
  uint8_t scratch_raw[4096];
  coap_rw_buffer_t scratch_buf = { scratch_raw, sizeof(scratch_raw) };

#ifdef IPV6
    fd = socket(AF_INET6,SOCK_DGRAM,0);
#else /* IPV6 */
  fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif /* IPV6 */

  bzero(&servaddr, sizeof(servaddr));
#ifdef IPV6
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(PORT);
#else /* IPV6 */
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(PORT);
#endif /* IPV6 */
  bind(fd, (struct sockaddr*) &servaddr, sizeof(servaddr));

  endpoint_setup();

  while (1)
  {
    int n, rc;
    socklen_t len = sizeof(cliaddr);
    coap_packet_t pkt;

    n = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr*) &cliaddr, &len);

    LogInfo("Received: ");
    coap_dump(buf, n, true);

    if (0 != (rc = coap_parse(&pkt, buf, n)))
    {
      LogInfo("Bad packet rc=%d\n", rc);
    }
    else
    {
      size_t rsplen = sizeof(buf);
      coap_packet_t rsppkt;
#ifdef DEBUG
      coap_dumpPacket(&pkt);
#endif
      coap_handle_req(&scratch_buf, &pkt, &rsppkt);

      if (0 != (rc = coap_build(buf, &rsplen, &rsppkt)))
        LogInfo("coap_build failed rc=%d\n", rc);
      else
      {
#ifdef DEBUG
        LogInfo("Sending: ");
        coap_dump(buf, rsplen, true);
        LogInfo("\n");
#endif
#ifdef DEBUG
        coap_dumpPacket(&rsppkt);
#endif

        sendto(fd, buf, rsplen, 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr));
      }
    }
  }
}

