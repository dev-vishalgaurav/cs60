#include "const.h"
/**
* this is a method to print and error and exit the program
*/

void showErrorAndExit(char *msg){
    perror(msg);
    exit(0);
}