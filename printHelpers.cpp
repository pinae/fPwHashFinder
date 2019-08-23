#include <iostream>
#include "vectorclass.h"
using namespace std;

char* getHex(unsigned char* bytes, int wordsCount=2, int wordSize=8) {
    char* hex_str;
    hex_str = (char*) malloc(2 * wordsCount * wordSize + 1);
    for (int i = 0; i < wordsCount; i++)
        for (int j = 0; j < wordSize; j++)
            sprintf(&hex_str[(i*wordSize + j) * 2], "%02x", (unsigned int) bytes[i*wordSize + wordSize-1-j]);
    return hex_str;
}

void printPwAndHash(char* pw, unsigned char* pwHash) {
    char* hash_hex = getHex(pwHash, 16, 1);
    printf("Password: %s - Hash(md5): %s\n", pw, hash_hex);
    free(hash_hex);
}

void printHash(Vec4uq* hash, int wordsCount = 4, int wordSize = 8, int offset = 0) {
    char* hash_hex = getHex((unsigned char *) hash + offset, wordsCount, wordSize);
    printf("%s", hash_hex);
    free(hash_hex);
}
