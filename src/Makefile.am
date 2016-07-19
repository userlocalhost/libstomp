AM_CPPFLAGS = -Wall -Werror
lib_LTLIBRARIES = libstomp.la
libstomp_la_LDFLAGS = -module -shared -version-info 0:0
libstomp_la_CFLAGS = -I../include $(CFLAGS) -g
libstomp_la_SOURCES = frame.c \
                      connection.c \
                      receiving_worker.c \
                      stomp.c
