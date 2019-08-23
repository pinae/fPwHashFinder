#include <openssl/md5.h>
#include <cstring>
#include <fstream>
#include <limits>
#include "vectorclassLibrary/vectorclass.h"
using namespace std;

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
