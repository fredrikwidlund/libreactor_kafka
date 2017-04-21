ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = -std=gnu11 -g -O3 -flto -fuse-linker-plugin -I$(srcdir)/src
AM_LDFLAGS = -static

EXTRA_DIST = \
CHANGES \
LICENSE \
README.rst

SOURCE_FILES = \
src/reactor_kafka/reactor_kafka.c \
src/reactor_kafka/reactor_kafka_producer.c \
src/reactor_kafka/reactor_kafka_consumer.c

HEADER_FILES = \
src/reactor_kafka/reactor_kafka.h \
src/reactor_kafka/reactor_kafka_producer.h \
src/reactor_kafka/reactor_kafka_consumer.h

AUTOMAKE_OPTIONS = subdir-objects
lib_LTLIBRARIES = libreactor_kafka.la
libreactor_kafka_la_SOURCES = $(SOURCE_FILES) $(HEADER_FILES)

headerfilesdir = $(includedir)/reactor_kafka
headerfiles_HEADERS = $(HEADER_FILES)

mainheaderdir = $(includedir)
mainheader_HEADERS = src/reactor_kafka.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libreactor_kafka.pc

## examples ###

bin_PROGRAMS = \
example/subscriber \
example/publisher

example_subscriber_LDADD = -lreactor_kafka -lreactor -ldynamic -lrdkafka
example_publisher_LDADD = -lreactor_kafka -lreactor -ldynamic -lrdkafka

MAINTAINERCLEANFILES = aclocal.m4 config.h.in configure Makefile.in docs/Makefile.in libreactor_kafka-?.?.?.tar.gz
maintainer-clean-local:; rm -rf autotools m4 libreactor_kafka-?.?.?

CLEANFILES = {.,src/reactor_kafka}/*.{gcno,gcda,gcov}
