comp_exo1 :
    gcc ./exo1.1/exo1daniel.c sha256.c -o main -g -Wall -Werror -Wextra
exec_exo1 : 
    ./exo1.1/main toto



    
Programme : main.o fonctions.o
    gcc main.o fonctions.o -o Programme
 
main.o : main.c fonctions.c
    gcc -c main.c -o main.o
 
fonctions.o : fonctions.c
    gcc -c fonctions.c -o fonctions.o
