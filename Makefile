CC=cc
FILES=main.c
OBJS=$(FILES:.c=.o)
NAME=main
all: $(NAME)
$(NAME): $(OBJS)
	$(CC) $(OBJS) -g -o $(NAME)
clean:
	rm -f $(OBJS)

default: clean all
