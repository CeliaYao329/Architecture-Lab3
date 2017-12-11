# build Simulator
all: main.cpp cache.cc memory.cc advancedCache.cpp
	g++ -g main.cpp cache.cc memory.cc -o sim
	g++ -g advancedMain.cpp advancedCache.cpp memory.cc -o advancedSim
clean:
	$(RM) sim main.o cache.o memory.o
	$(RM) advancedSim advancedMain.o advancedCache.o memory.o 