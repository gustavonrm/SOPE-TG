#!/bin/bash 

clear
if [ "$1" = "clean" ]
then 
    cd Server 
    make clean 
    cd ../User 
    make clean
    cd ..
    rm server
    rm user
fi


if [ "$1" = "make" ]
then 
    cd Server 
    make
    mv server ..
    cd ../User 
    make 
    mv user ..
fi

#$ ./build.sh clean ---- Limpa os ficheiros 
#$ ./build.sh make  ---- Gera os ficheiros 

