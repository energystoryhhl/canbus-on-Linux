#最终可执行文件
BIN = canbus 
#制定头文件的目录
INC = -Iinclude/
#指定源文件，wildcard为函数，作用为列出所有.c结尾的文件
SRC = $(wildcard *.c)
#指定obj文件，patsubst为函数，三个参数，作用为将所有SRC中的.c结尾的文件替换为.o结尾
OBJS = $(patsubst %.c,%.o,$(SRC))
#制定编译器
CC = gcc
#编译选项
CFLAGS = $(INC) -g
#目标依赖于所有的.o文件
$(BIN):$(OBJS)
		$(CC) -lpthread $^ -o $@ 	
clean:
		rm $(OBJS) $(BIN)