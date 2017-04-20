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

static void reactor_kafka_error(reactor_kafka *k, const char *reason)
{
  reactor_user_dispatch(&k->user, REACTOR_KAFKA_EVENT_ERROR, (void *) reason);
}

static rd_kafka_conf_t *reactor_kafka_configure(reactor_kafka *k, char *params[])
{
  rd_kafka_conf_t *conf;
  int status, i;
  char error[4096];

  conf = rd_kafka_conf_new();
  for (i = 0; params[i]; i += 2)
    {
      status = rd_kafka_conf_set(conf, params[i], params[i + 1], error, sizeof(error));
      if (status != RD_KAFKA_CONF_OK)
        {
          rd_kafka_conf_destroy(conf);
          reactor_kafka_error(k, error);
          return NULL;
        }
    }

  return conf;
}

static void reactor_kafka_read(reactor_kafka *k)
{
  rd_kafka_queue_t *queue;
  rd_kafka_event_t *event;

  queue = rd_kafka_queue_get_consumer(k->consumer);
  while(1)
    {
      event = rd_kafka_queue_poll(queue, 0);
      if (!event)
        break;

      switch (rd_kafka_event_type(event))
        {
        case RD_KAFKA_EVENT_REBALANCE:
          printf("rebalance\n");
          break;
        case RD_KAFKA_EVENT_FETCH:
          printf("fetch\n");
          break;
        case RD_KAFKA_EVENT_ERROR:
          printf("error\n");
          break;
        default:
          printf("default\n");
          break;
        }
      rd_kafka_event_destroy(event);
    }
}

static void reactor_kafka_event(void *state, int type, void *data)
{
  reactor_kafka *k = state;
  struct pollfd *pollfd = data;
  char token;
  size_t n;

  (void) fprintf(stderr, "%p %d %p\n", state, type, data);
  (void) type;

  if (pollfd->revents & POLLIN)
    {
      n = read(pollfd->fd, &token, 1);
      if (n == 1)
        reactor_kafka_read(k);
    }
}

void reactor_kafka_hold(reactor_kafka *k)
{
  k->ref ++;
}

void reactor_kafka_release(reactor_kafka *k)
{
  k->ref --;
  if (!k->ref)
    {
      reactor_user_dispatch(&k->user, REACTOR_KAFKA_EVENT_CLOSE, NULL);
    }
}

void reactor_kafka_open(reactor_kafka *k, reactor_user_callback *callback, void *state, char *brokers)
{
  *k = (reactor_kafka) {0};
  k->ref = 0;
  k->state = REACTOR_KAFKA_STATE_OPEN;
  reactor_user_construct(&k->user, callback, state);
  k->brokers = strdup(brokers);
  reactor_kafka_hold(k);
}

void reactor_kafka_close(reactor_kafka *k)
{
  if (k->state & REACTOR_KAFKA_STATE_CLOSED)
    return;

  k->state = REACTOR_KAFKA_STATE_CLOSED;
  reactor_kafka_release(k);
}

void reactor_kafka_group(reactor_kafka *k, char *id)
{
  k->group = id;
}

void reactor_kafka_subscribe(reactor_kafka *k, char *topic)
{
  char error[4096];
  rd_kafka_conf_t *conf;
  int status, fds[2];

  conf = reactor_kafka_configure(k, (char *[]) {
      "bootstrap.servers", k->brokers,
      "compression.codec", "snappy",
      "group.id", k->group ? k->group : "libreactor_kafka",
      "enable.partition.eof", "false",
      NULL});
  if (!conf)
    return;
  
  k->consumer = rd_kafka_new(RD_KAFKA_CONSUMER, conf, error, sizeof(error));
  if (!k->consumer)
   {
     reactor_kafka_error(k, error);
     return;
   }

  /*
  t = reactor_kafka_lookup(&k->consumer_topics, topic);
  if (t)
    return;
  */

  printf("ok\n");
  
  //rd_kafka_message_t *rkmessage;
  rd_kafka_topic_partition_list_t *topics;
  rd_kafka_message_t *message;

  
  topics = rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(topics, topic, -1);

  status = rd_kafka_subscribe(k->consumer, topics);
  if (status)
    {
      reactor_kafka_error(k, rd_kafka_err2str(status));
      return;
    }

  pipe(fds);
  reactor_core_fd_register(fds[0], reactor_kafka_event, k, POLLIN);
  rd_kafka_queue_t *queue = rd_kafka_queue_get_consumer(k->consumer);
  rd_kafka_queue_io_event_enable(queue, fds[1], "1", 1);
  //rd_kafka_queue_destroy(queue);
  /*
  message = rd_kafka_consumer_poll(k->consumer, 1000000);
  if (!message)
    {
      reactor_kafka_error(k, rd_kafka_err2str(rd_kafka_last_error()));
      return;
    }
  
  if (message->err)
    {
      if (message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
        {
          fprintf(stderr,
                  "Consumer reached end of %s [%"PRId32"] "
                  "message queue at offset %"PRId64"\n",
                  rd_kafka_topic_name(message->rkt),
                  message->partition, message->offset);
        }

      printf("%% Consume error for topic \"%s\" [%"PRId32"] "
             "offset %"PRId64": %s\n",
             message->rkt ? rd_kafka_topic_name(message->rkt):"",
             message->partition,
             message->offset,
             rd_kafka_message_errstr(message));
    }
  
  rd_kafka_message_destroy(message);
  printf("done\n");
  */
}
