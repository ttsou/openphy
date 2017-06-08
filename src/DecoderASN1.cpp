#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include "DecoderASN1.h"

extern "C" {
#include "lte/log.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
}

#define MAC_LTE_START_STRING_LEN	7
#define MAC_MAX_LEN			1024

/* Radio type */
#define FDD_RADIO 1
#define TDD_RADIO 2

/* Direction */
#define DIRECTION_UPLINK   0
#define DIRECTION_DOWNLINK 1

/* RNTI type */
#define NO_RNTI  0
#define P_RNTI   1
#define RA_RNTI  2
#define C_RNTI   3
#define SI_RNTI  4
#define SPS_RNTI 5
#define M_RNTI   6

#define MAC_LTE_START_STRING		"mac-lte"
#define MAC_LTE_RNTI_TAG		0x02
#define MAC_LTE_UEID_TAG		0x03
#define MAC_LTE_SUBFRAME_TAG		0x04
#define MAC_LTE_PREDEFINED_DATA_TAG	0x05
#define MAC_LTE_RETX_TAG		0x06
#define MAC_LTE_CRC_STATUS_TAG		0x07
#define MAC_LTE_EXT_BSR_SIZES_TAG	0x08
#define MAC_LTE_PAYLOAD_TAG		0x01

struct mac_frame {
	char start[MAC_LTE_START_STRING_LEN];
	uint8_t radio_type;
	uint8_t dir;
	uint8_t rnti_type;
	uint8_t rnti_tag;
	uint16_t rnti;
	uint8_t payload_tag;
	char payload[MAC_MAX_LEN];
} __attribute__((packed));

using namespace std;

bool DecoderASN1::open(int port)
{
    _sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_sock < 0) {
        ostringstream ostr;
        ostr << "ASN1  : Socket creating failed " << _sock;
        LOG_ERR(ostr.str().c_str());
        return false;
    }

    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);

    if (inet_aton("localhost", (struct in_addr *) &_addr.sin_addr) < 0) {
        ostringstream ostr;
        ostr << "ASN1  : Socket address conversion failed";
        LOG_ERR(ostr.str().c_str());
        return false;
    }

    return true;
}

bool DecoderASN1::send(const char *data, int len, uint16_t rnti)
{
    if (len < 0)
        throw out_of_range("");

    struct mac_frame hdr;
    string id(MAC_LTE_START_STRING);

    copy_n(begin(id), MAC_LTE_START_STRING_LEN, hdr.start);
    hdr.radio_type = FDD_RADIO;
    hdr.dir = DIRECTION_DOWNLINK;
    switch (rnti) {
    case 0xffff:
        hdr.rnti_type = SI_RNTI;
        break;
    case 0xfffe:
        hdr.rnti_type = P_RNTI;
        break;
    default:
        if (rnti <= 10) hdr.rnti_type = RA_RNTI;
        else hdr.rnti_type = C_RNTI;
    }
    hdr.rnti_tag = MAC_LTE_RNTI_TAG;
    hdr.rnti = htons(rnti);
    hdr.payload_tag = MAC_LTE_PAYLOAD_TAG;
    copy_n(data, len > MAC_MAX_LEN ? MAC_MAX_LEN : len, hdr.payload);

    int rc = sendto(_sock, &hdr, sizeof(hdr) - (MAC_MAX_LEN - len),
                    0, (const struct sockaddr *) &_addr, sizeof(_addr));
    if (rc < 0) {
        ostringstream ostr;
        ostr << "ASN1  : Socket send error " << -rc;
        LOG_ERR(ostr.str().c_str());
        return false;
    }

    return true;
}
