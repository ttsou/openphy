#ifndef CONVERT_H
#define CONVERT_H

void convert_float_short(short *out, float *in, float scale, int len);
void convert_short_float(float *out, short *in, int len, float scale);

#endif /* CONVERT_H */
