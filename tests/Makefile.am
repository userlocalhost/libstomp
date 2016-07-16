check_PROGRAMS = stomp_test

stomp_test_SOURCES = unit/test_frame.c \
                     unit/test_conn.c \
                     test.c
stomp_test_LDADD = -lcunit -lstomp -lpthread
stomp_test_CFLAGS = -I../include/ -I../src/.libs
stomp_test_LDFLAGS = -L../src/.libs/

check:
	./stomp_test
