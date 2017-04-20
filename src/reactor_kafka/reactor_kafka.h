#ifndef REACTOR_KAFKA_H_INCLUDED
#define REACTOR_KAFKA_H_INCLUDED

enum reactor_kafka_state
{
  REACTOR_KAFKA_STATE_CLOSED     = 0x01,
  REACTOR_KAFKA_STATE_OPEN       = 0x02,
  REACTOR_KAFKA_STATE_ERROR      = 0x04
};

enum reactor_kafka_event
{
  REACTOR_KAFKA_EVENT_ERROR,
  REACTOR_KAFKA_EVENT_CLOSE
};

typedef struct reactor_kafka reactor_kafka;

struct reactor_kafka
{
  size_t                           ref;
  size_t                           state;
  reactor_user                     user;
  reactor_timer                    timer;
  char                            *brokers;
  char                            *group;
  rd_kafka_t                      *consumer;
  rd_kafka_topic_partition_list_t *consumer_topics;
};

void reactor_kafka_hold(reactor_kafka *);
void reactor_kafka_release(reactor_kafka *);
void reactor_kafka_open(reactor_kafka *, reactor_user_callback *, void *, char *);
void reactor_kafka_close(reactor_kafka *);
void reactor_kafka_group(reactor_kafka *, char *);
void reactor_kafka_subscribe(reactor_kafka *, char *);

#endif /* REACTOR_KAFKA_H_INCLUDED */
