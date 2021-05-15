TARGET  = mysql-honeypotd

SOURCES = \
	main.c \
	globals.c \
	eventloop.c \
	connection.c \
	utils.c \
	protocol.c \
	dfa.c \
	pidfile.c \
	cmdline.c \
	daemon.c \
	log.c

OBJS        = $(patsubst %.c,%.o,$(SOURCES))
DEPS        = $(patsubst %.o,%.dep,$(OBJS))
COV_GCDA    = $(patsubst %.o,%.gcda,$(OBJS))
COV_GCNO    = $(patsubst %.o,%.gcno,$(OBJS))

all: $(TARGET)

TARGET_LDLIBS = -lev -lm

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(LDFLAGS_EXTRA) $^ $(TARGET_LDLIBS) $(LDLIBS) -o "$@"

%.o: %.c
	$(CC) $(CPPFLAGS) $(TARGET_CPPFLAGS) $(CPPFLAGS_EXTRA) $(CFLAGS) $(TARGET_CFLAGS) $(CFLAGS_EXTRA) -c "$<" -MMD -MP -MF"$(@:%.o=%.dep)" -MT"$(@:%.o=%.dep)" -MT"$@" -o "$@"

ifeq (,$(findstring clean,$(MAKECMDGOALS)))
-include $(DEPS)
endif

clean:
	-rm -f $(TARGET)
	-rm -f $(OBJS) $(DEPS)
	-rm -f $(COV_GCDA) $(COV_GCNO)
