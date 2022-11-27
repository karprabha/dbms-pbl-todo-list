CFLAGS = -std=c11
CPPFLAGS = -std=c++14
OBJPATH = ./obj/
SRCPATH = ./src/

task: main.o sqlite3.o
	g++ $(CPPFLAGS) $(OBJPATH)main.o $(OBJPATH)sqlite3.o -o task

main.o: $(SRCPATH)main.cpp
	g++ $(CPPFLAGS) -c $(SRCPATH)main.cpp -o $(OBJPATH)main.o

sqlite3.o: $(SRCPATH)sqlite3.c
	gcc $(CFLAGS) -c $(SRCPATH)sqlite3.c -o $(OBJPATH)sqlite3.o

clean:
	-del task.exe *.db
	-cd $(OBJPATH) && del *.o