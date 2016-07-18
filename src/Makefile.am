AM_CPPFLAGS = -Wall -Werror
lib_LTLIBRARIES = libstomp.la
libstomp_la_LDFLAGS = -module -avoid-version -shared
libstomp_la_CFLAGS = -I../include $(CFLAGS) -g
libstomp_la_SOURCES = frame.c \
                      connection.c \
                      receiving_worker.c \
                      stomp.c
