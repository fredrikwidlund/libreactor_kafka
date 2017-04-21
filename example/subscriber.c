#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <librdkafka/rdkafka.h>

#include <dynamic.h>
#include <reactor.h>

#include "reactor_kafka.h"

void event(void *state, int type, void *data)
{
  reactor_kafka_message *message;

  (void) state;
  switch (type)
    {
    case REACTOR_KAFKA_CONSUMER_EVENT_ERROR:
      (void) fprintf(stderr, "error: %s\n", (char *) data);
      break;
    case REACTOR_KAFKA_CONSUMER_EVENT_MESSAGE:
      message = data;
      (void) fprintf(stderr, "[message %s %lu]\n%.*s\n", message->topic, message->offset,
                     (int) message->data.size, message->data.base);
      break;
    }
}

void usage()
{
  extern char *__progname;

  (void) fprintf(stderr, "usage: %s <brokers> <topic>\n", __progname);
  exit(1);
}

int main(int argc, char **argv)
{
  reactor_kafka_consumer k;

  if (argc != 3)
    usage();

  reactor_core_construct();
  reactor_kafka_consumer_open(&k, event, &k, argv[1], NULL);
  reactor_kafka_consumer_subscribe(&k, argv[2]);
  reactor_core_run();
  reactor_core_destruct();
}
