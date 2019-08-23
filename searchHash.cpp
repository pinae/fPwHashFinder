#include <iostream>
#include <cstring>
#include "searchHelpers.h"
#include "printHelpers.h"
#include "vectorclassLibrary/vectorclass.h"
using namespace std;

//#define DEBUG
//#define DEBUG_DETAILED

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
        double pwHashAsDbl = md5ToDouble(pwHash, 0);

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
        double rangeStartHashDbl = md5ToDouble(rangeHash, 0);
        double rangeEndHashDbl = md5ToDouble(rangeHash, 2);
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
                            rangeStartHashDbl = md5ToDouble(rangeHash, 0);
                        } else {
                            #ifdef DEBUG
                                printf("Selecting middle range.\n");
                            #endif
                            rangeStart = piv1Pos;
                            rangeEnd = piv2Pos;
                            free(rangeHash);
                            rangeHash = pivHash;
                            rangeStartHashDbl = md5ToDouble(rangeHash, 0);
                            rangeEndHashDbl = md5ToDouble(rangeHash, 2);
                        }
                    } else {
                        #ifdef DEBUG
                            printf("Selecting lower range.\n");
                        #endif
                        rangeEnd = piv1Pos;
                        memcpy((char*) rangeHash, (char*) pivHash, 16);
                        free(pivHash);
                        rangeEndHashDbl = md5ToDouble(rangeHash, 2);
                    }
                } else {
                    if (sigleGt(pwHash, pivHash)) {
                        #ifdef DEBUG
                            printf("piv2 is out of range. Selecting upper range with piv1 only.\n");
                        #endif
                        rangeStart = piv1Pos;
                        memcpy((char*) rangeHash, (char*) pivHash, 16);
                        free(pivHash);
                        rangeStartHashDbl = md5ToDouble(rangeHash, 0);
                    } else {
                        #ifdef DEBUG
                            printf("piv2 is out of range. Selecting lower range with piv1 only.\n");
                        #endif
                        rangeEnd = piv1Pos;
                        memcpy((char*) rangeHash + 16, (char*) pivHash, 16);
                        free(pivHash);
                        rangeEndHashDbl = md5ToDouble(rangeHash, 2);
                    }
                }
            } else {
                #ifdef DEBUG
                    printf("Expectation failed. Range is %ld - %ld | size: %ld\n", rangeStart, rangeEnd, rangeSize);
                    printf("Switching to binary search.\n");
                #endif
                piv1Pos = rangeStart + rangeSize / 2;
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
                    rangeStartHashDbl = md5ToDouble(rangeHash, 0);
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
                    rangeEndHashDbl = md5ToDouble(rangeHash, 2);
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
