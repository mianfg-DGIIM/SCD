#!/bin/bash
# Uso: ./compilerun.sh <nº de procesos (subvectores, p)> <nº de iteraciones (M)>

mpicxx -std=c++11 iteracion_sincrona.cpp -o iteracion_sincrona
mpirun -np $1 iteracion_sincrona $1 $2
