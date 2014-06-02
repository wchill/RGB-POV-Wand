CXX = g++
WARNINGS = -Wchar-subscripts -Wparentheses -Wreturn-type -Wmissing-braces -Wundef -Wshadow
CXX_OPTS = -c -g -O3 -Wfatal-errors -Werror $(WARNINGS)

.PHONY: all clean

all: convert 

clean:
	-rm *.o convert 2>/dev/null

tidy:
	-rm -rf doc

convert: main.cpp png.o rgbapixel.o
	$(CXX) $^ -lpng -o $@

png.o: png.cpp png.h
	$(CXX) $(CXX_OPTS) $<

rgbapixel.o: rgbapixel.cpp rgbapixel.h
	$(CXX) $(CXX_OPTS) $<
