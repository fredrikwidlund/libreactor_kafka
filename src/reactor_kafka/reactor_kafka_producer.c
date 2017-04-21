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
#include "reactor_kafka_producer.h"

static void reactor_kafka_producer_error(reactor_kafka_producer *k, const char *reason)
{
  reactor_user_dispatch(&k->user, REACTOR_KAFKA_PRODUCER_EVENT_ERROR, (void *) reason);
}

void reactor_kafka_producer_hold(reactor_kafka_producer *k)
{
  k->ref ++;
}

void reactor_kafka_producer_release(reactor_kafka_producer *k)
{
  k->ref --;
  if (!k->ref)
    reactor_user_dispatch(&k->user, REACTOR_KAFKA_PRODUCER_EVENT_CLOSE, NULL);
}

static void reactor_kafka_producer_connect(reactor_kafka_producer *k, char *brokers)
{
  rd_kafka_conf_t *conf;
  char error[4096];

  conf = reactor_kafka_configure((char *[]) {
      "bootstrap.servers", brokers,
      NULL}, error, sizeof error);
  if (!conf)
    {
      reactor_kafka_producer_error(k, error);
      return;
    }

  k->kafka = rd_kafka_new(RD_KAFKA_PRODUCER, conf, error, sizeof error);
  if (!k->kafka)
   {
     reactor_kafka_producer_error(k, error);
     return;
   }
}

static void reactor_kafka_producer_event(void *state, int type, void *data)
{
  reactor_kafka_producer *k = state;

  (void) data;
  switch (type)
    {
    case REACTOR_TIMER_EVENT_CALL:
      rd_kafka_poll(k->kafka, 0);
      break;
    }
}

void reactor_kafka_producer_open(reactor_kafka_producer *k, reactor_user_callback *callback, void *state, char *brokers)
{
  *k = (reactor_kafka_producer) {0};
  k->ref = 0;
  k->state = REACTOR_KAFKA_PRODUCER_STATE_OPEN;
  reactor_user_construct(&k->user, callback, state);
  vector_construct(&k->topics, sizeof (rd_kafka_topic_t *));
  reactor_kafka_producer_hold(k);
  reactor_timer_open(&k->timer, reactor_kafka_producer_event, k, 100000000, 100000000);
  reactor_kafka_producer_connect(k, brokers);
}

void reactor_kafka_producer_close(reactor_kafka_producer *k)
{
  if (k->state & REACTOR_KAFKA_PRODUCER_STATE_CLOSED)
    return;

  k->state = REACTOR_KAFKA_PRODUCER_STATE_CLOSED;
  reactor_kafka_producer_release(k);
}

void reactor_kafka_producer_publish(reactor_kafka_producer *k, char *topic, char *data, size_t size)
{
  rd_kafka_topic_t *t;
  int status;
  size_t i;

  t = vector_data(&k->topics);
  for (i = 0; i < vector_size(&k->topics); i ++)
    {
      t = *(rd_kafka_topic_t **) vector_at(&k->topics, i);
      if (strcmp(rd_kafka_topic_name(t), topic) == 0)
        break;
    }
  
  if (!t)
    {
      t = rd_kafka_topic_new(k->kafka, topic, NULL);
      if (!t)
        {
          reactor_kafka_producer_error(k, rd_kafka_err2str(rd_kafka_last_error()));
          return;
        }
      vector_push_back(&k->topics, &t);
    }

  status = rd_kafka_produce(t, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, data, size, NULL, 0, NULL);
  if (status == -1)
    {
      reactor_kafka_producer_error(k, rd_kafka_err2str(rd_kafka_last_error()));
      return;
    }
}
