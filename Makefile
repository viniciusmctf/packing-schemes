CPPC=g++-8
CC=gcc-8
CFLAGS=-I. -pthread -lm -std=c++14
TESTFLAGS= 
DEPS = NaivePack.h PackBase.h
WUTESTOBJ = TestNaiveWorkUnit.o
# PACKTESTOBJ = ./tests/.o
WUOBJ = NaivePack.o
BIN = wutest
MAINTEST=MainTests.cpp

.PHONY: all clean gtest-checkout
FORCE: ;


test:
	$(CPPC) -o $@ $(MAINTEST) $(CFLAGS) $(TESTFLAGS)

%.o: %.cpp $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS) $(TESTFLAGS)

%.o: %.C $(DEPS)
	$(CPPC) -c -o $@ $< $(CFLAGS) $(TESTFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(TESTFLAGS)

wutest: $(OBJ) $(WUTESTOBJ)
	$(CPPC) -o $@ $^ $(CFLAGS) $(TESTFLAGS)

clean:
	rm $(BIN) *.o *.out *.log
