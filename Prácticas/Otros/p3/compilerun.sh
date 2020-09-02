#!/bin/bash

mpicxx -std=c++11 $1.cpp -o $1
mpirun -np $2 $1
