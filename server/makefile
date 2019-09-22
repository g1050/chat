.SUFFIXES:.c .o

CC=g++
SRCS=server.cpp\
		account.c\
			common.c\
				filechat.c\
					grpchat.c\
						pvachat.c
OBJS=$(SRCS:.c=.o)
EXEC=server

start:$(OBJS)
	@$(CC) -o  $(EXEC) $(SRCS) `mysql_config --cflags --libs` -w
	@echo "----------OK"
.c.o:
	@$(CC) -o  $@ -c $< `mysql_config --cflags --libs` -w
clean:
	@rm -rf $(OBJS) $(EXEC)


