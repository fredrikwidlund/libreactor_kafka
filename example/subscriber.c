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
  (void) fprintf(stderr, "%p %d %p\n", state, type, data);
  switch (type)
    {
    case REACTOR_KAFKA_EVENT_ERROR:
      (void) fprintf(stderr, "error: %s\n", (char *) data);
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
  reactor_kafka k;

  if (argc != 3)
    usage();

  reactor_core_construct();
  reactor_kafka_open(&k, event, &k, argv[1]);
  reactor_kafka_subscribe(&k, argv[2]);
  reactor_core_run();
  reactor_core_destruct();
}
