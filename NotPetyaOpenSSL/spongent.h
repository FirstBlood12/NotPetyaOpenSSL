#include <stdint.h>

typedef unsigned char uchar;

//globals
static uint8_t S[16] = {0xe, 0xd, 0xb, 0x0, 0x2, 0x1, 0x4, 0xf, 0x7, 0xa, 0x8, 0x5, 0x9, 0xc, 0x3, 0x6};
static const uint32_t b = 272;
static const uint32_t B = 272 / 8;
static const uint32_t n = 256;
uchar state[272 / 8];


uint8_t nextValueForLfsr(uint8_t lfsr)
{
    return (lfsr << 1) | (((lfsr >> 1) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 7)) & 1);
}

/*
    option 1, use:
    * 6 ANDs
    * 3 ORs
    * 2 assignments
    * 7 shifts left
    * 7 shifts right
*/
uint8_t reverse(uint8_t b)
{
   b =    (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b =    (b & 0xCC) >> 2 | (b & 0x33) << 2;
   return (b & 0xAA) >> 1 | (b & 0x55) << 1;
}

void pLayer()
{
    uchar tmp[B];
    memset(tmp, 0, B);

    for (unsigned long idx = 0; idx < (b - 1); ++idx)
    {
        uchar bit = (state[idx / 8] >> (idx % 8)) & 0x1;
        // the array is initialized to zero, so if the bit isn't set
        // it doesn't matter where it'd go
        if (bit)
        {
            unsigned long dest = (idx * b / 4) % (b - 1);
            tmp[dest / 8] |= 1 << dest % 8;
        }
    }
    //the very last bit stays in place since 
    // ((b-1)*b/4)mod(b-1) and (0*b/4)mod(b-1) would both be 0
    tmp[B-1] |= state[B - 1] & 0x80;
    memcpy(state, tmp, B);
}

void permute()
{
    uint8_t lfsr = 0x9E;
    do
    {
        state[0] ^= lfsr;
        state[B - 1] ^= reverse(lfsr);

        lfsr = nextValueForLfsr(lfsr);

        // in asm we might turn this into a subroutine?
        // Also it looks like we should be able to squeeze some bytes out of this
        // Shifting and lookup is done per nibble.
        for (unsigned long idx = 0; idx < B; ++idx)
            state[idx] = S[state[idx] >> 4] << 4 | S[state[idx] & 0xF];

        pLayer();
    } while (lfsr != 0xFF);
}

void spongent(uchar *input, unsigned long length, uchar *output)
{
    for (unsigned long idx = 0; idx < length; idx++)
    {
        state[idx&1] ^= input[idx];
        if(idx&1)permute();
    }

    state[0] ^= 0x80;
    permute();

    //do until n bits of data are extracted:
    for (unsigned long idx = 0; idx < n/8; idx++)  //one permute per r bits
    {
        //concatenate r bits of STATE with output
        output[idx] = state[idx&1];
        if(idx&1)permute();
    }
}