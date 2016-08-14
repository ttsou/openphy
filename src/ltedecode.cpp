/*
 * Copyright (C) 2016 Ettus Research LLC
 * Author Tom Tsou <tom.tsou@ettus.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <thread>
#include <cstdio>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "BufferQueue.h"
#include "SynchronizerPBCH.h"
#include "SynchronizerPDSCH.h"
#include "DecoderPDSCH.h"
#include "DecoderASN1.h"
#include "FreqAverager.h"

extern "C" {
#include "lte/log.h"
}

typedef complex<short> type;

/*
 * Number of LTE subframe buffers passed between PDSCH processing threads
 */
#define NUM_RECV_SUBFRAMES        128

struct lte_config {
    std::string args;
    double freq;
    double gain;
    int chans;
    int rbs;
    int threads;
    uint16_t rnti;
    enum dev_ref_type ref;
};

static void print_help()
{
        fprintf(stdout, "\nOptions:\n"
        "  -h    This text\n"
        "  -a    UHD device args\n"
        "  -c    Number of receive channels (1 or 2)\n"
        "  -f    Downlink frequency\n"
        "  -g    RF receive gain\n"
        "  -j    Number of PDSCH decoding threads (default = 1)\n"
        "  -b    Number of LTE resource blocks (default = auto)\n"
        "  -r    LTE RNTI (default = 0xFFFF)\n"
        "  -x    Enable external device reference (default = off)\n"
        "  -p    Enable GPSDO reference (default = off)\n\n");
}

static void print_config(struct lte_config *config)
{
    std::string refstr;

    switch (config->ref) {
    case REF_INTERNAL:
        refstr = "Internal";
        break;
    case REF_EXTERNAL:
        refstr = "External";
        break;
    case REF_GPSDO:
        refstr = "GPSDO";
        break;
    default:
        refstr = "Unknown";
    }

    fprintf(stdout,
        "Config:\n"
        "    Device args.............. \"%s\"\n"
        "    Downlink frequency....... %.3f MHz\n"
        "    Receive gain............. %.2f dB\n"
        "    Receive antennas......... %i\n"
        "    Clock reference.......... %s\n"
        "    PDSCH decoding threads... %i\n"
        "    LTE resource blocks...... %i\n"
        "    LTE RNTI................. 0x%04x\n"
        "\n",
        config->args.c_str(),
        config->freq / 1e6,
        config->gain,
        config->chans,
        refstr.c_str(),
        config->threads,
        config->rbs,
        config->rnti);
}

static bool valid_rbs(int rbs)
{
    switch (rbs) {
    case 6:
    case 15:
    case 25:
    case 50:
    case 75:
    case 100:
        return true;
    }

    return false;
}

static int handle_options(int argc, char **argv, struct lte_config *config)
{
    int option;

    config->freq = -1.0;
    config->gain = 0.0;
    config->chans = 1;
    config->rbs = 6;
    config->threads = 1;
    config->rnti = 0xffff;
    config->ref = REF_INTERNAL;

    while ((option = getopt(argc, argv, "ha:c:f:g:j:b:r:xp")) != -1) {
        switch (option) {
        case 'h':
            print_help();
            return -1;
        case 'a':
            config->args = optarg;
            break;
        case 'c':
            config->chans = atoi(optarg);
            if ((config->chans != 1) && (config->chans != 2)) {
                printf("Invalid number of channels\n");
                return -1;
            }
            break;
        case 'f':
            config->freq = atof(optarg);
            break;
        case 'g':
            config->gain = atof(optarg);
            break;
        case 'j':
            config->threads = atoi(optarg);
            break;
        case 'b':
            config->rbs = atoi(optarg);
            break;
        case 'r':
            config->rnti = atoi(optarg);
            break;
        case 'x':
            config->ref = REF_EXTERNAL;
            break;
        case 'p':
            config->ref = REF_GPSDO;
            break;
        default:
            print_help();
            return -1;
        }
    }

    if (config->threads < 1) {
        printf("\nInvalid number of PDSCH decoding threads %i\n",
               config->threads);
        return -1;
    }

    if (config->freq < 0.0) {
        print_help();
        printf("\nPlease specify downlink frequency\n");
        return -1;
    }

    if (config->rbs && !valid_rbs(config->rbs)) {
        print_help();
        printf("\nPlease specify valid number of resource blocks\n\n");
        printf("    LTE bandwidth      Resource Blocks\n");
        printf("       1.4 MHz                 6\n");
        printf("         3 MHz                15\n");
        printf("         5 MHz                25\n");
        printf("        10 MHz                50\n");
        printf("        15 MHz                75\n");
        printf("        20 MHz               100\n\n");

        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct lte_config config;
    struct lte_buffer *buf;
    std::vector<std::thread> threads;

    if (handle_options(argc, argv, &config) < 0)
        return -1;

    print_config(&config);

    auto pdschQueue = std::make_shared<BufferQueue>();
    auto pdschReturnQueue = std::make_shared<BufferQueue>();
    auto asn1 = std::make_shared<DecoderASN1>();
    SynchronizerPDSCH sync(config.chans);

    sync.attachInboundQueue(pdschReturnQueue);
    sync.attachOutboundQueue(pdschQueue);

    /* Prime the queue */
    for (int i = 0; i < NUM_RECV_SUBFRAMES; i++)
        pdschReturnQueue->write(std::make_shared<LteBuffer>(config.chans));

    asn1->open();
    std::vector<DecoderPDSCH> decoders(config.threads,
                                       DecoderPDSCH(config.chans));
    for (auto &d : decoders) {
        d.addRNTI(config.rnti);
        d.attachInboundQueue(pdschQueue);
        d.attachOutboundQueue(pdschReturnQueue);
        d.attachDecoderASN1(asn1);
        threads.push_back(std::thread(&DecoderPDSCH::start, &d));
    }

    if (!sync.open(config.rbs, config.ref, config.args)) {
        fprintf(stderr, "Radio: Failed to initialize\n");
        return -1;
    }
    sync.setFreq(config.freq);
    sync.setGain(config.gain);
    sync.start();

    for (auto &t : threads)
        t.join();

    return 0;
}
