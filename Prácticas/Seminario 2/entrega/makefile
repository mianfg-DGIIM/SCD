.SUFFIXES:
.PHONY: message

compilador:=g++
opcionesc:= -std=c++11 -pthread -Wfatal-errors
hmonsrcs:= HoareMonitor.h HoareMonitor.cpp Semaphore.h Semaphore.cpp
compilethis:= prodcons_sc_exe prodcons_su_exe

message: $(compilethis)
	$(info Compilados archivos correctamente en:)
	$(info - ./prodcons_sc_exe: versiones FIFO y LIFO, semántica SC)
	$(info - ./prodcons_su_exe: versiones FIFO y LIFO, semántica SU)
	$(info Haga make sc ó make su para ejecutar cada versión)
	$(info  )
	$(info Alumno: Miguel Ángel Fernández Gutiérrez)

sc: prodcons_sc_exe
	./$<

su: prodcons_su_exe
	./$<

prodcons_sc_exe: prodcons_sc.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)	-o $@ $<

prodcons_su_exe: prodcons_su.cpp $(hmonsrcs)
	$(compilador) $(opcionesc)	-o $@ $<  HoareMonitor.cpp Semaphore.cpp

clean:
	rm -f *_exe
