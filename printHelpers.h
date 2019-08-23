#include <iostream>
#include "vectorclassLibrary/vectorclass.h"
using namespace std;

char* getHex(unsigned char* bytes, int wordsCount=2, int wordSize=8);
void printPwAndHash(char* pw, unsigned char* pwHash);
void printHash(Vec4uq* hash, int wordsCount = 4, int wordSize = 8, int offset = 0);
