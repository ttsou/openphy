#ifndef _SIGNALVECTOR_
#define _SIGNALVECTOR_

#include <stdlib.h>
#include <vector>
#include <complex>

using namespace std;

struct cxvec;

class SignalVector {
public:
    SignalVector();
    SignalVector(const SignalVector &s);
    SignalVector(SignalVector &&s);
    SignalVector(size_t size, size_t head = 0);
    ~SignalVector();

    SignalVector& operator=(const SignalVector &s);
    SignalVector& operator=(SignalVector &&s);

    size_t size() const;
    void reverse();

    const complex<float> *cbegin() const;
    const complex<float> *cend() const;
    const complex<float> *chead() const;

    complex<float> *begin();
    complex<float> *end();
    complex<float> *head();
    complex<float>& operator[](int i);

    /* Dangerous calls */
    SignalVector(struct cxvec *v);
    struct cxvec *cv();
    static void translateVectors(vector<SignalVector> &v, struct cxvec **c);

private:
    bool _local;
    size_t _head;
    struct cxvec *_cv;
};
#endif /* _SIGNALVECTOR_ */
