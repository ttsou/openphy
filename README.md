About OpenPHY
==============

OpenPHY is a LTE UE receiver implementation for real-time test, decoding,
and network diagnostic purposes. Alternatively the implementation can serve as
the basis for a full software UE implementation when combined with uplink and
MAC/RRC layer functionality.

Implemented Functionality
=========================

General:
  * LTE Release 8
  * UE Category 3 (75 Mbps downlink)
  * FDD mode
  
LTE specifications:
  * 3GPP TS 36.211 *"Physical channels and modulation"*
  * 3GPP TS 36.212 *"Multiplexing and channel coding"*
  * 3GPP TS 36.213 *"Physical layer procedures"*

Physical Layer:
  * Bandwidth: 1.4, 3, 5, 10, and 20 MHz (automatic detection)
  * Channels: PSS, SSS, RS, PBCH, PCFICH, PDCCH, PDSCH
  * PDCCH formats (DCI): 0, 1, 1A, 1C, 1D, 2, 2A
  * Modulation: QPSK, QAM-16, QAM-64
  * MIMO: 2x2 or 2x1 transmit mode 2 (diversity)

Decoding (TurboFEC):
  * Turbo decoding for PDSCH (SIMD optimized)
  * Convolutional decoding for PBCH and PDCCH (SIMD optimized)
  * Turbo and convolutional rate matching

RF device support:
  * Ettus Research USRP B200/B210
  * Ettus Research USRP X300/X310

Processor Support:
  * Intel SSE3, SSE4, and AVX2 instruction support is automatically detected and
enabled at build time if available.

Dependencies
============

The following dependencies are required:

FFTW for computing the discrete Fourier transform (DFT).

  http://www.fftw.org

USRP Hardware Driver for Ettus radio device support

  http://uhd.ettus.com

Reference Clock
===============

OpenPHY will resolve the RF frequency offsets between the local device and the
remote eNodeB within a range of approximately +/-2.5 kHz.

*Use of GPSDO module or external frequency reference is recommended for RF
frequencies above 1 GHz.*

Wireshark
=========

Wireshark output of decoded LTE MAC frames is available on port 6666.

Compile
=======
```
$ autoreconf -i
$ ./configure
$ make
```

Install
=======
```
$ sudo make install
```

Run
===

LTE signal decoding is command line driven.

```
$ lte_decode -h

Options:
  -h    This text
  -a    UHD device args
  -c    Number of receive channels (1 or 2)
  -f    Downlink frequency
  -g    RF receive gain
  -j    Number of PDSCH decoding threads (default = 1)
  -b    Number of LTE resource blocks (default = auto)
  -r    LTE RNTI (default = 0xFFFF)
  -x    Enable external device reference (default = off)
  -p    Enable GPSDO reference (default = off)
```

The following command will enable receive MIMO on RF frequency of 751 MHz with a
gain setting of 40 dB. Default RNTI (SI-RNTI) will be used to decode eNodeB
system information blocks (SIB) on the PDSCH channel.

```
$ lte_decode -c 2 -f 751e6 -g 40

21:55:05:212 PDCCH : DCI Length 27, CRC 65535, Agg. Level 8, Block 0
21:55:05:212 PDCCH : DCI Format 1A:
                     Format 0/1A differentiation              1
                     Local/distributed VRB assignment         0
                     Resource block assignment                100
                     Modulation and coding                    7
                     HARQ process number                      0
                     New data indicator                       0
                     Redundancy version                       1
                     TPC command for PUCCH                    0
21:55:05:212 FRAME : SFN 0414, Ns 05
21:55:05:212 PDSCH : RIV 100, start 0, N_step -1, L_crbs 3
21:55:05:212 PDSCH : Transport block size A=224, Physical bits G=720
21:55:05:212 PDSCH : RNTI 65535, Modulation 2, Redundancy version 1
21:55:05:213 PDSCH : CRC Segmentation block r=0
21:55:05:213 PDSCH :     Rate match length E=720
21:55:05:213 PDSCH :     Rate match length D=252
21:55:05:213 PDSCH :     Turbo decode length K=248
21:55:05:213 PDSCH : Decoded transport block:
                     48 4c 46 90 11 f0 15 0d 77 18 92 98 23 21 03 10 8a c2 16 26 
                     36 fd 80 d9 00 00 00 0d 
21:55:05:226 PDCCH : DCI Length 27, CRC 65535, Agg. Level 8, Block 0
21:55:05:226 PDCCH : DCI Format 1A:
                     Format 0/1A differentiation              1
                     Local/distributed VRB assignment         0
                     Resource block assignment                1000
                     Modulation and coding                    8
                     HARQ process number                      0
                     New data indicator                       0
                     Redundancy version                       0
                     TPC command for PUCCH                    0
21:55:05:227 FRAME : SFN 0416, Ns 00
21:55:05:227 PDSCH : RIV 1000, start 0, N_step -1, L_crbs 21
21:55:05:227 PDSCH : Transport block size A=256, Physical bits G=5040
21:55:05:227 PDSCH : RNTI 65535, Modulation 2, Redundancy version 0
21:55:05:227 PDSCH : CRC Segmentation block r=0
21:55:05:227 PDSCH :     Rate match length E=5040
21:55:05:227 PDSCH :     Rate match length D=284
21:55:05:227 PDSCH :     Turbo decode length K=280
21:55:05:227 PDSCH : Decoded transport block:
                     00 81 01 f2 b7 ec 93 74 ab 42 53 40 04 00 04 01 60 29 dc aa 
                     f1 dd c8 01 c2 14 63 c4 50 a4 00 06 
```

Authors
=======

OpenPHY was written by Tom Tsou <tom.tsou@ettus.com>.
