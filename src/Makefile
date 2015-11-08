DESTDIR?=""

INCLUDE= `sdl-config --cflags`
LIBS= `sdl-config --libs` -lusb-1.0 -lSDL_gfx

OBJS= main.o DLC300.o AutoWhiteBalance.o

EXEC= dlc300

COMPILER_FLAGS+= -Wall -O3

$(EXEC): $(OBJS) $(wildcard *.h)
	$(CXX) $(COMPILER_FLAGS) -o $(EXEC) $(OBJS) $(LIBS)

%.o:	%.cc
	$(CXX) -c $(COMPILER_FLAGS) -o $@ $< $(INCLUDE)

all:	$(EXEC)

install: $(EXEC)
	install $(EXEC) "$(DESTDIR)"/usr/bin
	install -m 644 70-dlc300_camera.rules "$(DESTDIR)"/etc/udev/rules.d/

uninstall:
	rm "$(DESTDIR)"/usr/bin/$(EXEC)
	rm "$(DESTDIR)"/etc/udev/rules.d/70-dlc300_camera.rules

clean:
	rm -f $(EXEC) $(OBJS)
