CC = gcc

BINDIR = ../bin
OBJDIR = ../obj

CFLAGS = -c -I/usr/include/mysql -I/usr/include/pcre
LDFLAGS = -L/usr/lib/mysql -pthread -Wl,-Bstatic -lconfig -Wl,-Bdynamic -lmysqlclient -lpcre

ifeq ($(BUILD),release)
	PROG = $(BINDIR)/serenity
	OBJDIR += /release
	CFLAGS += -DNDEBUG -DUNIX -DPARANOID -DNVEDEXT -O3 -fno-exceptions
	LDFLAGS += -O3 -fno-exceptions
	EXTRA = strip $(PROG)
else
	PROG = $(BINDIR)/serenity-debug
	OBJDIR += /debug
	CFLAGS += -g -Wall -DUNIX -DPARANOID -DLOG_ERRORS -D_DEBUG -DNVEDEXT -DSUPERDEBUG
	LDFLAGS += -g
	EXTRA =
endif

OBJS = $(OBJDIR)/serenity.o $(OBJDIR)/sockets.o $(OBJDIR)/bf2.o $(OBJDIR)/bf2rcon.o $(OBJDIR)/src.o $(OBJDIR)/srcrcon.o $(OBJDIR)/global.o $(OBJDIR)/sqlquery.o $(OBJDIR)/util.o $(OBJDIR)/md5.o

all: serenity

serenity: $(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

$(OBJDIR)/serenity.o: serenity.c serenity.h bf2rcon.h
	$(CC) $(CFLAGS) -DSHOW_STATUS serenity.c -o $(OBJDIR)/serenity.o

$(OBJDIR)/sockets.o: sockets.c sockets.h util.h
	$(CC) $(CFLAGS) sockets.c -o $(OBJDIR)/sockets.o

$(OBJDIR)/bf2rcon.o: bf2rcon.c bf2rcon.h sockets.h
	$(CC) $(CFLAGS) bf2rcon.c -o $(OBJDIR)/bf2rcon.o

$(OBJDIR)/bf2.o: bf2.c bf2.h bf2rcon.c bf2rcon.h
	$(CC) $(CFLAGS) bf2.c -o $(OBJDIR)/bf2.o

$(OBJDIR)/srcrcon.o: srcrcon.c srcrcon.h sockets.h
	$(CC) $(CFLAGS) srcrcon.c -o $(OBJDIR)/srcrcon.o

$(OBJDIR)/src.o: src.c src.h srcrcon.c srcrcon.h
	$(CC) $(CFLAGS) src.c -o $(OBJDIR)/src.o

$(OBJDIR)/global.o: global.c global.h
	$(CC) $(CFLAGS) global.c -o $(OBJDIR)/global.o

$(OBJDIR)/util.o: util.c util.h
	$(CC) $(CFLAGS) util.c -o $(OBJDIR)/util.o

$(OBJDIR)/md5.o: md5.c md5.h
	$(CC) $(CFLAGS) md5.c -o $(OBJDIR)/md5.o

clean:
	rm $(OBJDIR)/*.o $(PROG)

