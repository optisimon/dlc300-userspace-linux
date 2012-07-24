INCLUDE= `sdl-config --cflags`
LIBS= `sdl-config --libs` -lusb-1.0 -lSDL_gfx

OBJS= main.o DLC300.o

EXEC= dlc300

$(EXEC): $(OBJS)
	$(CXX) $(COMPILER_FLAGS) -o $(EXEC) $(OBJS) $(LIBS)

%.o:    %.cpp ../../video/MyV4L2.hh
	$(CXX) -c $(COMPILER_FLAGS) -o $@ $< $(INCLUDE)

all:	$(EXEC)

clean:
	rm -f $(EXEC) $(OBJS)
