#ifndef REACTOR_KAFKA_H_INCLUDED
#define REACTOR_KAFKA_H_INCLUDED

typedef struct reactor_kafka_message reactor_kafka_message;
struct reactor_kafka_message
{
  char           *topic;
  size_t          offset;
  reactor_memory  data;
  reactor_memory  key;
};

rd_kafka_conf_t *reactor_kafka_configure(char *[], char *, size_t);

#endif /* REACTOR_KAFKA_H_INCLUDED */
