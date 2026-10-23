/* SPDX-License-Identifier: BSD-3-Clause */

static void r8_zx_packed_render(uint16_t* dest,const uint8_t* fb,const uint16_t colors[16]) {
    static uint32_t pairs[256];
    for(unsigned i=0;i<256;i++)pairs[i]=(uint32_t)colors[i&15]|((uint32_t)colors[i>>4]<<16);
    for(unsigned y=0;y<240;y++) {
        const uint8_t* src=fb+(y+8)*512;
        uint8_t* out=(uint8_t*)(dest+y*320);
        for(unsigned x=0;x<320;x+=4) {
            uint32_t v;memcpy(&v,src+x,sizeof v);
            v &= UINT32_C(0x0f0f0f0f);
            v |= v >> 4;
            unsigned lo=v&255;
            unsigned hi=(v>>16)&255;
            memcpy(out+x*2,&pairs[lo],sizeof(uint32_t));
            memcpy(out+x*2+4,&pairs[hi],sizeof(uint32_t));
        }
    }
}
