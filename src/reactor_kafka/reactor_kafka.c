#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <librdkafka/rdkafka.h>

#include <dynamic.h>
#include <reactor.h>

#include "reactor_kafka.h"

rd_kafka_conf_t *reactor_kafka_configure(char *param[], char *error, size_t error_size)
{
  rd_kafka_conf_t *conf;
  int status, i;

  conf = rd_kafka_conf_new();
  for (i = 0; param[i]; i += 2)
    {
      status = rd_kafka_conf_set(conf, param[i], param[i + 1], error, error_size);
      if (status != RD_KAFKA_CONF_OK)
        {
          rd_kafka_conf_destroy(conf);
          return NULL;
        }
    }
  
  return conf;
}
