INCLUDE= `sdl-config --cflags`
LIBS= `sdl-config --libs` -lusb-1.0 -lSDL_gfx

OBJS= main.o DLC300.o AutoWhiteBalance.o

EXEC= dlc300

COMPILER_FLAGS+= -Wall

$(EXEC): clean $(OBJS)
	$(CXX) $(COMPILER_FLAGS) -o $(EXEC) $(OBJS) $(LIBS)

%.o:	%.cc
	$(CXX) -c $(COMPILER_FLAGS) -o $@ $< $(INCLUDE)

all:	$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
