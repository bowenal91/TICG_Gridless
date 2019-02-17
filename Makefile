FAST= -fast
CCINTEL=icpc
CCMPI=mpicxx
CCMPIINTEL=mpiicpc
MPIFAST=-0fast

FLAGS= -O3 
DEBUG= -g -p
FAST = -fast
CC = g++
SRCS = main.cpp mersenne.cpp  

OBJS = $(SRCS)

main: $(OBJS) 
	$(CC)  $(OBJS) $(FLAGS) -o run -w -std=c++11 

mpi: $(OBJS)1
	$(CCMPIINTEL) $(OBJS) $(MPIFAST) -lm -w -o dsa

mpi_debug: $(OBJS)
	$(CCMPI)	$(OBJS)	$(DEBUG) -o dsa

intelfast: $(OBJS)
	$(CCINTEL)  $(OBJS) $(FAST) -o dsa

intelstatic: $(OBJS)
	$(CCINTEL)  $(OBJS) $(FAST) -ipo -o dsa

fast: $(OBJS)
	$(CC)  $(OBJS) $(FAST) -o dsa

debug: $(OBJS)
	$(CC)  $(OBJS) $(DEBUG) -w -o debug

%.cpp.o: %.cpp
	$(CC) $(FLAGS) $(DEBUG) -c -o $@ $< 
clean:
	rm *.cpp.o

