CPPFLAGS= -I/usr/local/opt/opencv3/include
LDFLAGS= -L/usr/local/opt/opencv3/lib

% : %.cpp
	clang++ -std=c++11 -Wall `pkg-config --cflags --libs opencv3` $(CPPFLAGS) -o $@ $<

