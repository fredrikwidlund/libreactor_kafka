#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <librdkafka/rdkafka.h>

#include <dynamic.h>
#include <reactor.h>

#include "reactor_kafka.h"

typedef struct app app;
struct app
{
  int                     beat;
  char                   *topic;
  reactor_timer           timer;
  reactor_kafka_producer  producer;
};

void timer(void *state, int type, void *data)
{
  app *app = state;
  char buffer[4096];
  
  (void) data;
  switch (type)
    {
    case REACTOR_TIMER_EVENT_CALL:
      snprintf(buffer, sizeof buffer, "%d", app->beat);
      app->beat ++;
      reactor_kafka_producer_publish(&app->producer, app->topic, buffer, strlen(buffer));
      (void) fprintf(stderr, "beat %s\n", buffer);
      break;
    }
}

void kafka(void *state, int type, void *data)
{
  (void) state;
  switch (type)
    {
    case REACTOR_KAFKA_PRODUCER_EVENT_ERROR:
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
  app app = {0};

  if (argc != 3)
    usage();

  app.topic = argv[2];
  reactor_core_construct();
  reactor_timer_open(&app.timer, timer, &app, 1000000000, 1000000000);
  reactor_kafka_producer_open(&app.producer, kafka, &app, argv[1]);
  reactor_core_run();
  reactor_core_destruct();
}
