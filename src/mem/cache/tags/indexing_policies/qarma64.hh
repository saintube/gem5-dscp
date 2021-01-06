// author: Zhejiang University ICSR Phantom0308
// modified by saintube

#ifndef __MEM_CACHE_INDEXING_POLICIES_QARMA64_HH__
#define __MEM_CACHE_INDEXING_POLICIES_QARMA64_HH__

#include "base/types.hh"

#define MAX_LENGTH 64
#define subcells sbox[sbox_use]
#define subcells_inv sbox_inv[sbox_use]

typedef unsigned char          cell_t;

// Qarma64 is a lightweight tweakable block cipher.
class Qarma64
{
  public:
    // Constructor.
    Qarma64(Addr w, Addr k);

    // Destructor.
    ~Qarma64() {};

    // QARMA-64 encryption
    Addr qarma64_enc(Addr plaintext, Addr tweak, int rounds);

    // QARMA-64 decryption
    Addr qarma64_dec(Addr plaintext, Addr tweak, int rounds);

  protected:
    Addr w0;
    Addr k0;

    // int sbox_use = 0;
    // Addr check_box[3] = { 0x3ee99a6c82af0c38, 0x9f5c41ec525603c9,
    //     0xbcaf6c89de930765 };
    // int sbox_use = 1;
    // Addr check_box[3] = { 0x544b0ab95bda7c3a, 0xa512dd1e4e3ec582,
    //     0xedf67ff370a483f2 };
    int sbox_use = 2;
    Addr check_box[3] = { 0xc003b93999b33765, 0x270a787275c48d10,
        0x5c06a7501b63b2fd };

    int m = MAX_LENGTH / 16;

    Addr alpha = 0xC0AC29B7C97C50DD;
    Addr c[8] = { 0x0000000000000000, 0x13198A2E03707344, 0xA4093822299F31D0,
        0x082EFA98EC4E6C89, 0x452821E638D01377, 0xBE5466CF34E90C6C,
        0x3F84D5B5B5470917, 0x9216D5D98979FB1B };

    // sbox 0: lightest version, fixed points at 0, 2.
    // sbox 1: no fixed points.
    // sbox 2: lightweight sbox from prince family.
    int sbox[3][16] = {
        { 0, 14, 2, 10, 9, 15, 8, 11, 6, 4, 3, 7, 13, 12, 1, 5},
        {10, 13, 14, 6, 15, 7, 3, 5, 9, 8, 0, 12, 11, 1, 2, 4},
        {11, 6, 8, 15, 12, 0, 9, 14, 3, 7, 4, 5, 13, 2, 1, 10}};

    int sbox_inv[3][16] = {
        { 0, 14, 2, 10, 9, 15, 8, 11,
            6, 4, 3, 7, 13, 12, 1, 5},
        {10, 13, 14, 6, 15, 7, 3, 5,
            9, 8, 0, 12, 11, 1, 2, 4},
        { 5, 14, 13, 8, 10, 11, 1, 9,
            2, 6, 15, 0, 4, 12, 7, 3}};

    int t[16] = { 0, 11, 6, 13, 10, 1, 12, 7, 5, 14, 3, 8, 15, 4,  9, 2 };
    int t_inv[16] = { 0, 5, 15, 10, 13, 8, 2, 7, 11, 14, 4, 1, 6, 3, 9, 12 };
    int h[16] = { 6, 5, 14, 15, 0, 1, 2, 3, 7, 12, 13, 4, 8, 9, 10, 11 };
    int h_inv[16] = { 4, 5, 6, 7, 11, 1, 0, 8, 12, 13, 14, 15, 9, 10, 2, 3 };

    cell_t M[16] = { 0, 1, 2, 1,
        1, 0, 1, 2,
        2, 1, 0, 1,
        1, 2, 1, 0 };

    void text2cell(cell_t* cell, Addr is);

    Addr cell2text(cell_t* cell);

    Addr pseudo_reflect(Addr is, Addr tk);

    Addr forward(Addr is, Addr tk, int r);

    Addr backward(Addr is, Addr tk, int r);

    cell_t LFSR(cell_t x);

    cell_t LFSR_inv(cell_t x);

    Addr forward_update_key(Addr T);

    Addr backward_update_key(Addr T);
};

#endif // __MEM_CACHE_INDEXING_POLICIES_QARMA64_HH__
