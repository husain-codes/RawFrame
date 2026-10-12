#include <img/img_processing.h>

void unpack_10bit_mipi(const uint8_t *packed_data, uint16_t *out_data,
                       uint32_t width, uint32_t height, size_t stride) {
    for(int y=0; y<height; y++) {
        const uint8_t* row_in = packed_data + y*stride;
        uint16_t* row_out = out_data + y*width;
        for(int x=0; x<width; x+=4) {
           int idx =  (x/4)*5;
            uint8_t b0 = row_in[idx+0];
            uint8_t b1 = row_in[idx+1];
            uint8_t b2 = row_in[idx+2];
            uint8_t b3 = row_in[idx+3];
            uint8_t b4 = row_in[idx+4];

            row_out[x+0] = (b0<<2) | (b4 & 0x03);
            row_out[x+1] = (b1<<2) | ((b4>>2) & 0x03);
            row_out[x+2] = (b2<<2) | ((b4>>4) & 0x03);
            row_out[x+3] = (b3<<2) | ((b4>>6) & 0x03);

        }
    }    
}