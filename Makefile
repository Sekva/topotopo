CC = gcc -std=gnu99 -Wall -Wextra -pedantic -g3 -s -O3
LIBFLAGS= -lX11 -lm
SRC_DIR = ./src/
OBJETOS_DIR = ./obj/
BUILD_DIR = ./build/
SRCS = main.c
BIN_NOME = topotopo
OBJS = $(SRCS:.c=.o)
OBJS_FINAL = $(OBJS:%.o=$(OBJETOS_DIR)%.o)
BIN = $(BUILD_DIR)$(BIN_NOME)

all : dirs $(BIN)

dirs:
	@mkdir -pv $(BUILD_DIR) $(OBJETOS_DIR)

$(BIN) : $(OBJS_FINAL)
	$(CC) $(OBJS_FINAL) -o $(BIN) $(LIBFLAGS)

$(OBJS_FINAL) : $(OBJETOS_DIR)%.o : $(SRC_DIR)%.c
	$(CC) -c $< -o $@

clean :
	rm  $(BIN) $(OBJS_FINAL)

run: all
	$(BIN)
