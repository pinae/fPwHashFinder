#include <iostream>
#include <fstream>
#include <openssl/md5.h>
#include <cstring>
#include <limits>
#include "vectorclass.h"
using namespace std;

//#define DEBUG
//#define DEBUG_DETAILED

unsigned char* switchEndian(unsigned char* a, int size = 16) {
    unsigned char* switchBuf;
    switchBuf = (unsigned char*) malloc(size);
    for (int i=0; i<8; i++) {
        switchBuf[i] = a[7-i];
        switchBuf[i+8] = a[7-i+8];
    }
    free(a);
    return switchBuf;
}

unsigned char* calcMd5(char* string, int mallocSize = 16) {
    unsigned char* digest;
    digest = (unsigned char*) malloc(mallocSize);
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, string, strlen(string));
    MD5_Final(digest, &ctx);
    return digest;
}

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

void dualGt(Vec4uq* a, Vec4uq* b, bool* gt1, bool* gt2) {
    Vec4qb a_gt_b = *a > *b;
    Vec4qb a_eq_b = *a == *b;
    *gt1 = a_gt_b.extract(0) || (a_eq_b.extract(0) && a_gt_b.extract(1));
    *gt2 = a_gt_b.extract(2) || (a_eq_b.extract(2) && a_gt_b.extract(3));
}

bool sigleGt(Vec4uq* a, Vec4uq* b) {
    Vec4qb a_gt_b = *a > *b;
    Vec4qb a_eq_b = *a == *b;
    return a_gt_b.extract(0) || (a_eq_b.extract(0) && a_gt_b.extract(1));
}

void readHash(Vec4uq* v, unsigned int p, ifstream* s, streampos begin, bool lower=true) {
    auto* rb = (unsigned char*) malloc(16);
    s->seekg(begin + (streampos) p * 20);
    s->read((char*) rb, 16);
    #ifdef DEBUG_DETAILED
        char* hash_hex = getHex((unsigned char *) rb, 16, 1);
        printf("Read bytes (%10ld): %s\n", (long) begin + (streampos) p * 20, hash_hex);
    #endif
    auto* vb = (unsigned char*) v;
    if (lower) {
        unsigned char buf[32] = {rb[7], rb[6], rb[5], rb[4], rb[3], rb[2], rb[1], rb[0],
                                 rb[15], rb[14], rb[13], rb[12], rb[11], rb[10], rb[9], rb[8],
                                 vb[16], vb[17], vb[18], vb[19], vb[20], vb[21], vb[22], vb[23],
                                 vb[24], vb[25], vb[26], vb[27], vb[28], vb[29], vb[30], vb[31]};
        #ifdef DEBUG_DETAILED
            hash_hex = getHex((unsigned char *) buf, 32, 1);
            printf("                 Buffer: %s\n", hash_hex);
            free(hash_hex);
        #endif
        v->load(buf);
    } else {
        unsigned char buf[32] = {vb[0], vb[1], vb[2], vb[3], vb[4], vb[5], vb[6], vb[7],
                                 vb[8], vb[9], vb[10], vb[11], vb[12], vb[13], vb[14], vb[15],
                                 rb[7], rb[6], rb[5], rb[4], rb[3], rb[2], rb[1], rb[0],
                                 rb[15], rb[14], rb[13], rb[12], rb[11], rb[10], rb[9], rb[8]};
        #ifdef DEBUG_DETAILED
            hash_hex = getHex((unsigned char *) buf, 32, 1);
            printf("                  Buffer: %s\n", hash_hex);
            free(hash_hex);
        #endif
        v->load(buf);
    }
}

unsigned int* readCount(unsigned int p, ifstream* s, streampos begin) {
    auto* count = (unsigned int*) malloc(4);
    s->seekg(begin + (streampos) p * 20 + (streampos) 16);
    s->read((char*) count, 4);
    return count;
}

double hashToDouble(Vec4uq* v, int vecOffset= 0) {
    auto dv = (double) v->extract(vecOffset);
    return dv * (double) numeric_limits<unsigned long>::max() + (double) v->extract(1 + vecOffset);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Error: Please supply at least one password as a command line argument.");
        return 0;
    }

    double relativeSearchRange = 0.02;

    streampos fileBegin, fileEnd;
    ifstream file_stream ("collection4.md5", ios::binary);
    fileBegin = file_stream.tellg();
    file_stream.seekg(0, ios::end);
    fileEnd = file_stream.tellg();
    #ifdef DEBUG
        printf("File begin: %08x -- File end: %08x \n", (unsigned int) fileBegin, (unsigned int) fileEnd);
    #endif
    unsigned long initRangeStart = 0;
    unsigned long initRangeEnd = (fileEnd - fileBegin - 20) / 20;
    unsigned long rangeStart, rangeEnd;
    Vec4uq* rangeHash;
    Vec4uq* pivHash;
    Vec4uq* pwHash;
    rangeHash = (Vec4uq*) malloc(32);
    pivHash = (Vec4uq*) malloc(32);
    pwHash = (Vec4uq*) malloc(32);

    for (int pwNo = 0; pwNo < argc - 1; pwNo++) {
        char *pw = argv[pwNo + 1];
        unsigned char *pwHashStr = calcMd5(pw, 32);
        #ifdef DEBUG
            printPwAndHash(pw, pwHashStr);
        #endif
        pwHashStr = switchEndian(pwHashStr, 16);
        memcpy(pwHashStr + 16, pwHashStr, 16);
        memcpy(pwHash, pwHashStr, 32);
        free(pwHashStr);
        #ifdef DEBUG
            printf("pwHash vector: ");
            printHash(pwHash, 2, 8, 0);
            printf(" ");
            printHash(pwHash, 2, 8, 16);
            printf("\n");
        #endif
        double pwHashAsDbl = hashToDouble(pwHash, 0);

        // Calculate
        rangeStart = initRangeStart;
        rangeEnd = initRangeEnd;
        readHash(rangeHash, rangeStart, &file_stream, fileBegin, true);
        readHash(rangeHash, rangeEnd, &file_stream, fileBegin, false);
        #ifdef DEBUG
            printf("range (%9ld - %9ld): ", rangeStart, rangeEnd);
            printHash(rangeHash, 2, 8, 0); printf(" ");
            printHash(rangeHash, 2, 8, 16); printf("\n");
        #endif
        double rangeStartHashDbl = hashToDouble(rangeHash, 0);
        double rangeEndHashDbl = hashToDouble(rangeHash, 2);
        #ifdef DEBUG
            printf("Double range is %f - %f\n", rangeStartHashDbl, rangeEndHashDbl);
        #endif
        unsigned long rangeSize = rangeEnd - rangeStart;
        bool piv1Gt, piv2Gt;

        while (rangeSize > 1) {
            double expectedRelativePos = (pwHashAsDbl - rangeStartHashDbl) /
                                         (rangeEndHashDbl - rangeStartHashDbl);
            #ifdef DEBUG
                printf("Expected relative position: %f\n", expectedRelativePos);
                printf("range (%9ld - %9ld): ", rangeStart, rangeEnd);
                printHash(rangeHash, 2, 8, 0); printf(" ");
                printHash(rangeHash, 2, 8, 16); printf("\n");
                printf("                pwHash vector: ");
                printHash(pwHash, 2, 8, 0); printf(" ");
                printHash(pwHash, 2, 8, 16);printf("\n");
            #endif
            unsigned long piv1Pos = (unsigned long) ((expectedRelativePos - relativeSearchRange) * (double) rangeSize) + rangeStart;
            unsigned long piv2Pos = (unsigned long) ((expectedRelativePos + relativeSearchRange) * (double) rangeSize) + rangeStart;
            //int expectedPos = (int) (expectedRelativePos * (rangeEnd - rangeStart)) + rangeStart;
            if (piv1Pos > rangeStart && piv1Pos < rangeEnd) {
                pivHash = (Vec4uq*) malloc(32);
                readHash(pivHash, piv1Pos, &file_stream, fileBegin, true);
                #ifdef DEBUG
                    printf("     hash at piv1 (%9ld): ", piv1Pos);
                    printHash(pivHash, 2, 8, 0); printf("\n");
                #endif
                if (piv2Pos > piv1Pos && piv2Pos < rangeEnd) {
                    readHash(pivHash, piv2Pos, &file_stream, fileBegin, false);
                    #ifdef DEBUG
                        printf("     hash at piv2 (%9ld): ", piv2Pos);
                        printHash(pivHash, 2, 8, 16); printf("\n");
                    #endif
                    dualGt(pwHash, pivHash, &piv1Gt, &piv2Gt);
                    #ifdef DEBUG
                        printf("dualGt() pwHash > pivHash: %d %d\n", piv1Gt, piv2Gt);
                    #endif
                    if (piv1Gt) {
                        if (piv2Gt) {
                            #ifdef DEBUG
                                printf("Selecting upper range.\n");
                            #endif
                            rangeStart = piv2Pos;
                            memcpy((char*) rangeHash, (char*) pivHash + 16, 16);
                            free(pivHash);
                            rangeStartHashDbl = hashToDouble(rangeHash, 0);
                        } else {
                            #ifdef DEBUG
                                printf("Selecting middle range.\n");
                            #endif
                            rangeStart = piv1Pos;
                            rangeEnd = piv2Pos;
                            free(rangeHash);
                            rangeHash = pivHash;
                            rangeStartHashDbl = hashToDouble(rangeHash, 0);
                            rangeEndHashDbl = hashToDouble(rangeHash, 2);
                        }
                    } else {
                        #ifdef DEBUG
                            printf("Selecting lower range.\n");
                        #endif
                        rangeEnd = piv1Pos;
                        memcpy((char*) rangeHash, (char*) pivHash, 16);
                        free(pivHash);
                        rangeEndHashDbl = hashToDouble(rangeHash, 2);
                    }
                } else {
                    if (sigleGt(pwHash, pivHash)) {
                        #ifdef DEBUG
                            printf("piv2 is out of range. Selecting upper range with piv1 only.\n");
                        #endif
                        rangeStart = piv1Pos;
                        memcpy((char*) rangeHash, (char*) pivHash, 16);
                        free(pivHash);
                        rangeStartHashDbl = hashToDouble(rangeHash, 0);
                    } else {
                        #ifdef DEBUG
                            printf("piv2 is out of range. Selecting lower range with piv1 only.\n");
                        #endif
                        rangeEnd = piv1Pos;
                        memcpy((char*) rangeHash + 16, (char*) pivHash, 16);
                        free(pivHash);
                        rangeEndHashDbl = hashToDouble(rangeHash, 2);
                    }
                }
            } else {
                #ifdef DEBUG
                    printf("Expectation failed. Range is %ld - %ld | size: %ld\n", rangeStart, rangeEnd, rangeSize);
                    printf("Switching to binary search.\n");
                #endif
                unsigned long piv1Pos = rangeStart + rangeSize / 2;
                pivHash = (Vec4uq*) malloc(32);
                readHash(pivHash, piv1Pos, &file_stream, fileBegin, true);
                #ifdef DEBUG
                    printf("hash at piv1 (%ld): ", piv1Pos);
                    printHash(pivHash, 2, 8, 0); printf("\n");
                #endif
                if (sigleGt(pwHash, pivHash)) {
                    #ifdef DEBUG
                        printf("Binary search. Selecting upper range with piv1 only.\n");
                    #endif
                    rangeStart = piv1Pos + 1;
                    readHash(rangeHash, rangeStart, &file_stream, fileBegin, true);
                    free(pivHash);
                    rangeStartHashDbl = hashToDouble(rangeHash, 0);
                    #ifdef DEBUG_DETAILED
                        printf("range (%ld - %ld): ", rangeStart, rangeEnd);
                        printHash(rangeHash, 2, 8, 0); printf(" ");
                        printHash(rangeHash, 2, 8, 16); printf("\n");
                    #endif
                } else {
                    #ifdef DEBUG
                        printf("Binary search. Selecting lower range with piv1 only.\n");
                    #endif
                    rangeEnd = piv1Pos;
                    memcpy((char*) rangeHash + 16, (char*) pivHash, 16);
                    free(pivHash);
                    rangeEndHashDbl = hashToDouble(rangeHash, 2);
                    #ifdef DEBUG_DETAILED
                        printf("range (%ld - %ld): ", rangeStart, rangeEnd);
                        printHash(rangeHash, 2, 8, 0); printf(" ");
                        printHash(rangeHash, 2, 8, 16); printf("\n");
                    #endif
                }
            }
            rangeSize = rangeEnd - rangeStart;
        }
        #ifdef DEBUG
            printf("Finished\n");
            printf("pwHash vector:            ");
            printHash(pwHash, 2, 8, 0); printf(" ");
            printHash(pwHash, 2, 8, 16);printf("\n");
            printf("range start  (%ld): ", rangeStart);
            printHash(rangeHash, 2, 8, 0); printf("\n");
            printf("range end    (%ld): ", rangeEnd);
            printHash(rangeHash, 2, 8, 16); printf("\n");
        #endif
        Vec4qb pw_eq_range = *pwHash == *rangeHash;
        if (pw_eq_range.extract(0) && pw_eq_range.extract(1)) {
            unsigned int* count = readCount(rangeStart, &file_stream, fileBegin);
            printf("Your password \"%s\" was found %d times in hacks. Change it!\n", pw, *count);
            free(count);
        } else {
            if (pw_eq_range.extract(2) && pw_eq_range.extract(3)) {
                unsigned int* count = readCount(rangeEnd, &file_stream, fileBegin);
                printf("Your password \"%s\" was found %d times in hacks. Change it!\n", pw, *count);
                free(count);
            } else {
                printf("Your password \"%s\" is safe.\n", pw);
            }
        }
    }
    free(rangeHash);
    free(pivHash);
    free(pwHash);
    file_stream.close();
    return 0;
}
