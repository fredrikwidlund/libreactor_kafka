#ifndef REACTOR_KAFKA_CONSUMER_H_INCLUDED
#define REACTOR_KAFKA_CONSUMER_H_INCLUDED

enum reactor_kafka_consumer_state
{
  REACTOR_KAFKA_CONSUMER_STATE_CLOSED     = 0x01,
  REACTOR_KAFKA_CONSUMER_STATE_OPEN       = 0x02,
  REACTOR_KAFKA_CONSUMER_STATE_ERROR      = 0x04
};

enum reactor_kafka_consumer_event
{
  REACTOR_KAFKA_CONSUMER_EVENT_ERROR,
  REACTOR_KAFKA_CONSUMER_EVENT_MESSAGE,
  REACTOR_KAFKA_CONSUMER_EVENT_CLOSE
};

typedef struct reactor_kafka_consumer reactor_kafka_consumer;
struct reactor_kafka_consumer
{
  size_t            ref;
  size_t            state;
  reactor_user      user;
  rd_kafka_t       *kafka;
  rd_kafka_queue_t *queue;
  int               fd[2];
};

void reactor_kafka_consumer_hold(reactor_kafka_consumer *);
void reactor_kafka_consumer_release(reactor_kafka_consumer *);
void reactor_kafka_consumer_open(reactor_kafka_consumer *, reactor_user_callback *, void *, char *, char *);
void reactor_kafka_consumer_close(reactor_kafka_consumer *);
void reactor_kafka_consumer_subscribe(reactor_kafka_consumer *, char *);

#endif /* REACTOR_KAFKA_CONSUMER_H_INCLUDED */
