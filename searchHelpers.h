#include <fstream>
#include "vectorclass.h"
using namespace std;

unsigned char* switchEndian(unsigned char* a, int size = 16);
unsigned char* calcMd5(char* string, int mallocSize = 16);
void dualGt(Vec4uq* a, Vec4uq* b, bool* gt1, bool* gt2);
bool sigleGt(Vec4uq* a, Vec4uq* b);
void readHash(Vec4uq* v, unsigned int p, ifstream* s, streampos begin, bool lower=true);
unsigned int* readCount(unsigned int p, ifstream* s, streampos begin);
double hashToDouble(Vec4uq* v, int vecOffset= 0);
