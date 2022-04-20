



kernel void BlurWPI16(
    const global uchar* source_pixel_data,
    global uchar* dest_pixel_data,
    const int width,
    const int height){
    
    // first byte (not pixel) col id of current block
    const size_t col_id=BYTES_PER_PX+get_global_id(0)*WPI16;
    // this is pixel or byte (actually same!) row id
    const size_t row_id=get_global_id(1);

    const size_t bytes_per_row=((width*BYTES_PER_PX-1)/4+1)*4;
    const size_t data_byte_ubound=width*BYTES_PER_PX-BYTES_PER_PX;

    if(row_id>0&&row_id<height-1&&col_id+WPI16-1<data_byte_ubound){

        size_t total_offset=row_id*bytes_per_row+col_id;
        size_t upper_total_offset=total_offset-bytes_per_row;
        size_t lower_total_offset=total_offset+bytes_per_row;

        /*
                [************  ****] <-UPPER PIXELS (16)
            [****************][********] <-CENTER PIXELS (16), CENTER PIXELS1 (8)
                [************  ****] <-LOWER PIXELS (16)
        */

        // already upper pixels
        uint16 upper_pixels=convert_uint16(vload16(0,source_pixel_data+ upper_total_offset));
        uint16 center_pixels=convert_uint16(vload16(0,source_pixel_data+ total_offset-4));
        uint8 center_pixels_1=convert_uint8(vload8(0,source_pixel_data+ total_offset+WPI16-4));
        uint16 lower_pixels=convert_uint16(vload16(0,source_pixel_data+ lower_total_offset));


        uchar16 avg_pixels=convert_uchar16((
            upper_pixels
            +(uint16)(center_pixels.yzw,center_pixels.lo.hi,center_pixels.hi,center_pixels_1.x)
            +(uint16)(center_pixels.lo.hi.w,center_pixels.hi,center_pixels_1.lo,center_pixels_1.hi.xyz)
            +lower_pixels)>>2);

        vstore16(avg_pixels,0,dest_pixel_data+ total_offset);
    }
}

