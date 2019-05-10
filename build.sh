#!/bin/bash 

clear
if [ "$1" = "clean" ]
then 
    cd server 
    make clean 
    cd ..
    cd user 
    make clean 
    cd ..
fi


if [ "$1" = "make" ]
then 
    cd server 
    make
    cd ..
    cd user 
    make 
    cd ..
fi

#$ ./build.sh clean ---- Limpa os ficheiros 
#$ ./build.sh make  ---- Gera os ficheiros 

