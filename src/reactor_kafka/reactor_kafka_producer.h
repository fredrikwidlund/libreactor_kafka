#ifndef REACTOR_KAFKA_PRODUCER_H_INCLUDED
#define REACTOR_KAFKA_PRODUCER_H_INCLUDED

enum reactor_kafka_producer_state
{
  REACTOR_KAFKA_PRODUCER_STATE_CLOSED     = 0x01,
  REACTOR_KAFKA_PRODUCER_STATE_OPEN       = 0x02,
  REACTOR_KAFKA_PRODUCER_STATE_ERROR      = 0x04
};

enum reactor_kafka_producer_event
{
  REACTOR_KAFKA_PRODUCER_EVENT_ERROR,
  REACTOR_KAFKA_PRODUCER_EVENT_MESSAGE,
  REACTOR_KAFKA_PRODUCER_EVENT_CLOSE
};

typedef struct reactor_kafka_producer reactor_kafka_producer;

struct reactor_kafka_producer
{
  size_t            ref;
  size_t            state;
  reactor_user      user;
  reactor_timer     timer;
  vector            topics;
  rd_kafka_t       *kafka;
};

void reactor_kafka_producer_hold(reactor_kafka_producer *);
void reactor_kafka_producer_release(reactor_kafka_producer *);
void reactor_kafka_producer_open(reactor_kafka_producer *, reactor_user_callback *, void *, char *);
void reactor_kafka_producer_close(reactor_kafka_producer *);
void reactor_kafka_producer_publish(reactor_kafka_producer *, char *, char *, size_t);

#endif /* REACTOR_KAFKA_PRODUCER_H_INCLUDED */
