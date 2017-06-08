#ifndef _DECODER_ASN1_
#define _DECODER_ASN1_

#include <netinet/in.h>
#include <vector>

class DecoderASN1 {
public:
    DecoderASN1() = default;
    DecoderASN1(const DecoderASN1 &d) = default;
    DecoderASN1(DecoderASN1 &&d) = default;
    ~DecoderASN1() = default;

    DecoderASN1 &operator=(const DecoderASN1 &d) = default;
    DecoderASN1 &operator=(DecoderASN1 &&d) = default;

    bool open(uint16_t port);
    bool send(const char *data, int len, uint16_t rnti);

private:
    int _sock;
    struct sockaddr_in _addr;
};

#endif /* _DECODER_ASN1_ */
