
compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp

all: prodconsN_sc_lifo_exe prodconsN_sc_fifo_exe prodconsN_su_lifo_exe prodconsN_su_fifo_exe

prodconsN_sc_lifo_exe: prodconsN_sc_lifo.cpp
	$(compilador) $(opcionesc) $(hmonsrcs) -o $@ $<

prodconsN_sc_fifo_exe: prodconsN_sc_fifo.cpp
	$(compilador) $(opcionesc) $(hmonsrcs) -o $@ $<

prodconsN_su_lifo_exe: prodconsN_su_lifo.cpp
	$(compilador) $(opcionesc) $(hmonsrcs) -o $@ $<

prodconsN_su_fifo_exe: prodconsN_su_fifo.cpp
	$(compilador) $(opcionesc) $(hmonsrcs) -o $@ $<

clean:
	rm -f *_exe
