#!/bin/bash
# Uso: ./compilerun.sh <nº de procesos (subvectores, p)> <nº de iteraciones (M)>

mpicxx -std=c++11 iteracion_sincrona_dos_vectores.cpp -o iteracion_sincrona_dos_vectores
mpirun -np $1 iteracion_sincrona_dos_vectores $1 $2
