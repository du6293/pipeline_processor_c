CC = gcc
TARGET = intro_exe

$(TARGET) : main.o rv32i_pipe.o
        $(CC) -o $(TARGET) rv32i_pipe.o main.o

rv32i.o : rv32i_pipe.c
        $(CC) -c -o rv32i_pipe.o rv32i_pipe.c

main.o : main.c
        $(CC) -c -o main.o main.c

clean :
        rm *.o intro_exe
