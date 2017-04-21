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
#include "reactor_kafka_consumer.h"

static void reactor_kafka_consumer_error(reactor_kafka_consumer *k, const char *reason)
{
  reactor_user_dispatch(&k->user, REACTOR_KAFKA_CONSUMER_EVENT_ERROR, (void *) reason);
}

static void reactor_kafka_consumer_read(reactor_kafka_consumer *k)
{
  rd_kafka_event_t *event;
  const rd_kafka_message_t *rkmessage;

  while(1)
    {
      event = rd_kafka_queue_poll(k->queue, 0);
      if (!event)
        break;

      switch (rd_kafka_event_type(event))
        {
        case RD_KAFKA_EVENT_FETCH:
        case RD_KAFKA_EVENT_DR:
          while (1)
            {
              rkmessage = rd_kafka_event_message_next(event);
              if (!rkmessage)
                break;
              if (rkmessage->err)
                {
                  rd_kafka_event_destroy(event);
                  reactor_kafka_consumer_error(k, "event error");
                  return;
                }
              else
                reactor_user_dispatch(&k->user, REACTOR_KAFKA_CONSUMER_EVENT_MESSAGE, (reactor_kafka_message[]) {
                    (char *) rd_kafka_topic_name(rkmessage->rkt),
                    rkmessage->offset,
                    (reactor_memory) {rkmessage->payload, rkmessage->len},
                    (reactor_memory) {rkmessage->key, rkmessage->key_len}
                  });
            }
          break;
        case RD_KAFKA_EVENT_ERROR:
          reactor_kafka_consumer_error(k, rd_kafka_event_error_string(event));;
          return;
        default:
          break;
        }
      rd_kafka_event_destroy(event);
    }
}

static void reactor_kafka_consumer_pipe_event(void *state, int type, void *data)
{
  reactor_kafka_consumer *k = state;
  struct pollfd *pollfd = data;
  char token;
  size_t n;

  (void) type;
  if (pollfd->revents != POLLIN)
    {
      reactor_kafka_consumer_error(k, "unexpected consumer event");
      return;
    }

  n = read(pollfd->fd, &token, 1);
  if (n != 1)
    {
      reactor_kafka_consumer_error(k, "unexpected read");
      return;
    }

  reactor_kafka_consumer_read(k);
}

void reactor_kafka_consumer_hold(reactor_kafka_consumer *k)
{
  k->ref ++;
}

void reactor_kafka_consumer_release(reactor_kafka_consumer *k)
{
  k->ref --;
  if (!k->ref)
    reactor_user_dispatch(&k->user, REACTOR_KAFKA_CONSUMER_EVENT_CLOSE, NULL);
}

static void reactor_kafka_consumer_connect(reactor_kafka_consumer *k, char *brokers, char *group)
{
  rd_kafka_conf_t *conf;
  char error[4096];

  conf = reactor_kafka_configure((char *[]) {
      "bootstrap.servers", brokers,
      "group.id", group ? group : "reactor_kafka_consumer",
      "enable.partition.eof", "false",
      NULL}, error, sizeof error);
  if (!conf)
    {
      reactor_kafka_consumer_error(k, error);
      return;
    }

  k->kafka = rd_kafka_new(RD_KAFKA_CONSUMER, conf, error, sizeof error );
  if (!k->kafka)
   {
     reactor_kafka_consumer_error(k, error);
     return;
   }

  k->queue = rd_kafka_queue_get_consumer(k->kafka);
  if (!k->queue)
    {
      reactor_kafka_consumer_error(k, rd_kafka_err2str(rd_kafka_last_error()));
      return;
    }

  (void) pipe(k->fd);
  reactor_core_fd_register(k->fd[0], reactor_kafka_consumer_pipe_event, k, POLLIN);
  rd_kafka_queue_io_event_enable(k->queue, k->fd[1], ".", 1);
}

void reactor_kafka_consumer_open(reactor_kafka_consumer *k, reactor_user_callback *callback, void *state,
                                 char *brokers, char *group)
{
  *k = (reactor_kafka_consumer) {0};
  k->ref = 0;
  k->state = REACTOR_KAFKA_CONSUMER_STATE_OPEN;
  reactor_user_construct(&k->user, callback, state);
  reactor_kafka_consumer_hold(k);
  reactor_kafka_consumer_connect(k, brokers, group);
}

void reactor_kafka_consumer_close(reactor_kafka_consumer *k)
{
  if (k->state & REACTOR_KAFKA_CONSUMER_STATE_CLOSED)
    return;

  k->state = REACTOR_KAFKA_CONSUMER_STATE_CLOSED;
  reactor_kafka_consumer_release(k);
}

void reactor_kafka_consumer_subscribe(reactor_kafka_consumer *k, char *topic)
{
  rd_kafka_topic_partition_list_t *topics;
  int status;

  topics = rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(topics, topic, -1);
  status = rd_kafka_subscribe(k->kafka, topics);
  rd_kafka_topic_partition_list_destroy(topics);
  if (status)
    {
      reactor_kafka_consumer_error(k, rd_kafka_err2str(status));
      return;
    }
}
