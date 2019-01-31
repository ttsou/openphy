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
#include <iomanip>
#include <map>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>

#include "BufferQueue.h"
#include "SynchronizerPBCH.h"
#include "SynchronizerPDSCH.h"
#include "DecoderPDSCH.h"
#include "DecoderASN1.h"
#include "FreqAverager.h"
#include "UHDDevice.h"

extern "C" {
#include "lte/log.h"
}

/*
 * Number of LTE subframe buffers passed between PDSCH processing threads
 */
#define NUM_RECV_SUBFRAMES        128

struct Config {
    std::string args = "";
    double freq      = 1e9;
    double gain      = 50;
    unsigned chans   = 1;
    unsigned rbs     = 6;
    unsigned threads = 1;
    uint16_t port    = 7878;
    uint16_t rnti    = 0xffff;
    UHDDevice<>::ReferenceType ref = UHDDevice<>::REF_INTERNAL;
};

static void print_help()
{
    fprintf(stdout, "\nOptions:\n"
        "  -h  --help     This text\n"
        "  -a  --args     UHD device args\n"
        "  -c  --chans    Number of receive channels (1 or 2)\n"
        "  -f  --freq     Downlink frequency\n"
        "  -g  --gain     RF receive gain\n"
        "  -r, --ref      Frequency reference (%s)\n"
        "  -j  --threads  Number of PDSCH decoding threads (default = 1)\n"
        "  -b  --rb       Number of LTE resource blocks (default = auto)\n"
        "  -n  --rnti     LTE RNTI (default = 0xFFFF)\n"
        "  -p  --port     Wireshark port\n\n",
        "'internal', 'external', 'gps'"
    );
}

static void print_config(Config *config)
{
    const std::map<UHDDevice<>::ReferenceType, std::string> refMap = {
        { UHDDevice<>::REF_INTERNAL, "Internal" },
        { UHDDevice<>::REF_EXTERNAL, "External" },
        { UHDDevice<>::REF_GPS,      "GPS"      },
    };

    auto rntiString = [](uint16_t rnti) {
        std::stringstream ss;
        ss << "0x" << std::setfill('0') << std::setw(4) << std::hex << rnti;
        switch (rnti) {
        case 0xffff:
            ss << " (SI-RNTI)";
            break;
        case 0xfffe:
            ss << " (P-RNTI)";
            break;
        }
        return ss.str();
    };

    fprintf(stdout,
        "Config:\n"
        "    Device args.............. \"%s\"\n"
        "    Downlink frequency....... %.6f GHz\n"
        "    Receive gain............. %.1f dB\n"
        "    Receive antennas......... %u\n"
        "    Frequency reference...... %s\n"
        "    PDSCH decoding threads... %u\n"
        "    LTE resource blocks...... %u\n"
        "    LTE RNTI................. %s\n"
        "\n",
        config->args.c_str(),
        config->freq / 1e9,
        config->gain,
        config->chans,
        refMap.at(config->ref).c_str(),
        config->threads,
        config->rbs,
        rntiString(config->rnti).c_str()
    );
}

static bool handle_options(int argc, char **argv, Config &config)
{
    const std::map<std::string, UHDDevice<>::ReferenceType> refMap = {
      { "internal", UHDDevice<>::REF_INTERNAL },
      { "external", UHDDevice<>::REF_EXTERNAL },
      { "gpsdo",    UHDDevice<>::REF_GPS      },
      { "gps",      UHDDevice<>::REF_GPS      },
    };

    auto setParam = [](const auto &m, auto arg, auto &val) {
      auto mi = m.find(arg);
      if (mi == m.end()) {
        printf("Invalid parameter '%s'\n\n", arg);
        return false;
      }
      val = mi->second;
      return true;
    };

    const struct option longopts[] = {
        { "args",    1, nullptr, 'a' },
        { "help",    0, nullptr, 'h' },
        { "chans",   1, nullptr, 'c' },
        { "freq",    1, nullptr, 'f' },
        { "gain",    1, nullptr, 'g' },
        { "threads", 1, nullptr, 'j' },
        { "rb",      1, nullptr, 'b' },
        { "rnti",    1, nullptr, 'n' },
        { "ref" ,    1, nullptr, 'r' },
        { "port" ,   1, nullptr, 'p' },
    };

    int option;
    while ((option = getopt_long(argc, argv, "ha:c:f:g:j:b:n:r:", longopts, nullptr)) != -1) {
        switch (option) {
        case 'a':
            config.args = optarg;
            break;
        case 'c':
            config.chans = atoi(optarg);
            if (config.chans > 2) {
                printf("Invalid number of channels\n");
                return false;
            }
            break;
        case 'f':
            config.freq = atof(optarg);
            break;
        case 'g':
            config.gain = atof(optarg);
            break;
        case 'j':
            config.threads = atoi(optarg);
            break;
        case 'b':
            config.rbs = atoi(optarg);
            break;
        case 'n':
            config.rnti = std::stoi(optarg, nullptr, 0);
            break;
        case 'r':
            if (!setParam(refMap, optarg, config.ref)) return false;
            break;
        case 'p':
            config.port = atoi(optarg);
            break;
        case 'h':
        default:
            return false;
        }
    }

    auto validRB = [](auto rbs) {
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
    };

    if (!validRB(config.rbs)) {
        printf("\nPlease specify valid number of resource blocks\n\n");
        printf("    LTE bandwidth      Resource Blocks\n");
        printf("          Auto                 0\n");
        printf("       1.4 MHz                 6\n");
        printf("         3 MHz                15\n");
        printf("         5 MHz                25\n");
        printf("        10 MHz                50\n");
        printf("        15 MHz                75\n");
        printf("        20 MHz               100\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    Config config;
    std::vector<std::thread> threads;

    if (!handle_options(argc, argv, config)) {
        print_help();
        return -EINVAL;
    }

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

    asn1->open(config.port);
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
