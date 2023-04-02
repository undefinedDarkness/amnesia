#ifndef C2CB3CD7_D2FD_4F9E_B362_7E7B98FB9DD8
#define C2CB3CD7_D2FD_4F9E_B362_7E7B98FB9DD8

typedef unsigned char pixelv __attribute__((vector_size(4)));
typedef char spixelv __attribute((vector_size(4)));
typedef int calcv __attribute((vector_size(4 * sizeof(int))));


void ditherPass(long start_h, long height, long width, pixelv*);
pixelv lumaInvert(pixelv &c);
pixelv findNearest(pixelv &find);
void loadPalette();

#endif /* C2CB3CD7_D2FD_4F9E_B362_7E7B98FB9DD8 */
