/**
  @file GImage_png.cpp
  @author Morgan McGuire, http://graphics.cs.williams.edu
  @created 2002-05-27
  @edited  2009-04-20
 */
#include "G3D/platform.h"
#include "G3D/GImage.h"
#include <png.h>
#include <sstream>
#include "G3D/BinaryOutput.h"

namespace G3D {

//libpng required function signature
static void png_write_data(png_structp png_ptr,
    png_bytep data,
    png_size_t length) {

    debugAssert( png_ptr->io_ptr != NULL );
    debugAssert( data != NULL );

    ((BinaryOutput*)png_ptr->io_ptr)->writeBytes(data, length);
}

//libpng required function signature
static void png_flush_data(
    png_structp png_ptr) {
    (void)png_ptr;
    //Do nothing.
}

//libpng required function signature
static void png_error(
    png_structp png_ptr,
    png_const_charp error_msg) {
    
    (void)png_ptr;
    debugAssert( error_msg != NULL );
    
    png_ptr->error_ptr = const_cast<png_charp>(error_msg);
    longjmp(png_ptr->jmpbuf, 1);
}


//libpng required function signature
void png_warning(
    png_structp png_ptr,
    png_const_charp warning_msg) 
{
}

//libpng required function signature
static void png_read_data(
    png_structp png_ptr,
    png_bytep data,
    png_size_t length) {


        debugAssert( png_ptr->io_ptr != NULL );
        debugAssert( length >= 0 );
        debugAssert( data != NULL );

        int64 pos = ((BinaryInput*)png_ptr->io_ptr)->getPosition();
        int64 binaryOutputLen = ((BinaryInput*)png_ptr->io_ptr)->getLength();

        if (pos + length > binaryOutputLen)
        {
            length = binaryOutputLen - pos;
            G3D::png_error(png_ptr, "Trying to load incomplete image");
        }

        ((BinaryInput*)png_ptr->io_ptr)->readBytes(data, length);
}


void GImage::encodePNG(
    BinaryOutput&           out) const {

    if (! (m_channels == 1 || m_channels == 2 || m_channels == 3 || m_channels == 4)) {
        throw GImage::Error(format("Illegal channels for PNG: %d", m_channels));
    }
    if (m_width <= 0) {
        throw GImage::Error(format("Illegal width for PNG: %d", m_width));
    }
    if (m_height <= 0) {
        throw GImage::Error(format("Illegal height for PNG: %d", m_height));
    }

    // PNG library requires that the height * pointer size fit within an int
    if (png_uint_32(m_height) * png_sizeof(png_bytep) > PNG_UINT_32_MAX) {
        throw GImage::Error("Unsupported PNG height.");
    }

    out.setEndian(G3D_LITTLE_ENDIAN);

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, png_error, png_warning);
    if (! png_ptr) {
        throw GImage::Error("Unable to initialize PNG encoder." );
    }
    
    if (setjmp(png_ptr->jmpbuf)) {
        const char* message = png_ptr->error_ptr ? static_cast<const char*>(png_ptr->error_ptr) : "Unknown PNG error";
        
        throw GImage::Error(message);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (! info_ptr) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw GImage::Error("Unable to initialize PNG encoder.");
    }

    //setup libpng write handler so can use BinaryOutput
    png_set_write_fn(png_ptr, (void*)&out, png_write_data, png_flush_data);
    png_color_8_struct sig_bit;

    switch (m_channels) {
    case 1:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_GRAY,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        sig_bit.red = 0;
        sig_bit.green = 0;
        sig_bit.blue = 0;
        sig_bit.alpha = 0;
        sig_bit.gray = 8;
        break;

    case 2:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_GRAY_ALPHA,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        sig_bit.red = 0;
        sig_bit.green = 0;
        sig_bit.blue = 0;
        sig_bit.alpha = 8;
        sig_bit.gray = 8;
        break;

    case 3:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        sig_bit.red = 8;
        sig_bit.green = 8;
        sig_bit.blue = 8;
        sig_bit.alpha = 0;
        sig_bit.gray = 0;
        break;

    case 4:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 8, PNG_COLOR_TYPE_RGBA,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        sig_bit.red = 8;
        sig_bit.green = 8;
        sig_bit.blue = 8;
        sig_bit.alpha = 8;
        sig_bit.gray = 0;
        break;

    default:
        png_destroy_write_struct(&png_ptr, &info_ptr);
        throw GImage::Error("Unsupported number of channels for PNG.");
    }


    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    //write the png header
    png_write_info(png_ptr, info_ptr);

    png_bytepp row_pointers = new png_bytep[m_height];

    for (int i=0; i < m_height; ++i) {
        row_pointers[i] = (png_bytep)&m_byte[m_width * m_channels * i];
    }

    png_write_image(png_ptr, row_pointers);

    png_write_end(png_ptr, info_ptr);

    delete[] row_pointers;

    png_destroy_write_struct(&png_ptr, &info_ptr);
}


void GImage::decodePNG(
    BinaryInput&        input) {

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, png_error, png_warning);
    if (png_ptr == NULL) {
        throw GImage::Error("Unable to initialize PNG decoder.");
    }
    
    if (setjmp(png_ptr->jmpbuf)) {
        const char* message = png_ptr->error_ptr ? static_cast<const char*>(png_ptr->error_ptr) : "Unknown PNG error";
        
        throw GImage::Error(message);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        throw GImage::Error("Unable to initialize PNG decoder.");
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (end_info == NULL) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        throw GImage::Error("Unable to initialize PNG decoder.");
    }

    // now that the libpng structures are setup, change the error handlers and read routines
    // to use G3D functions so that BinaryInput can be used.

    png_set_read_fn(png_ptr, (png_voidp)&input, png_read_data);
    
    // read in sequentially so that three copies of the file are not in memory at once
    png_read_info(png_ptr, info_ptr);

    png_uint_32 png_width, png_height;
    int bit_depth, color_type, interlace_type;
    // this will validate the data it extracts from info_ptr
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bit_depth, &color_type,
       &interlace_type, NULL, NULL);

    m_width  = static_cast<uint32>(png_width);
    m_height = static_cast<uint32>(png_height);

    //swap bytes of 16 bit files to least significant byte first
    png_set_swap(png_ptr);

    png_set_strip_16(png_ptr);

    //Expand paletted colors into true RGB triplets
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }

    //Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand(png_ptr);
    }

    //Expand paletted or RGB images with transparency to full alpha channels
    //so the data will be available as RGBA quartets.
    bool has1BitAlpha = png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS);
    if (has1BitAlpha) {
        png_set_tRNS_to_alpha(png_ptr);
    }

    // Fix sub-8 bit_depth to 8bit
    if (bit_depth < 8) {
        png_set_packing(png_ptr);
    }

    size_t bytesToAlloc = 0;

    if ((color_type == PNG_COLOR_TYPE_RGBA) || ((color_type == PNG_COLOR_TYPE_PALETTE) && (png_ptr->num_trans > 0)) || (color_type ==  PNG_COLOR_TYPE_RGB && has1BitAlpha)) 
    {
        m_channels = 4;
    }
    else if ((color_type == PNG_COLOR_TYPE_RGB) || (color_type == PNG_COLOR_TYPE_PALETTE)) 
    {
        m_channels = 3;
    } 
    else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA || (color_type ==  PNG_COLOR_TYPE_GRAY && has1BitAlpha)) 
    {
        m_channels = 2;
    }
    else if (color_type == PNG_COLOR_TYPE_GRAY) 
    {
        m_channels = 1;
	} 
    else 
    {
        throw GImage::Error("Unsupported PNG bit-depth or type.");
    }

    bytesToAlloc = m_width * m_height * m_channels;
    m_byte = (uint8*)m_memMan->alloc(bytesToAlloc);

    if (!m_byte)
    {
        std::stringstream ss;
        ss << "Out of memory while allocating " << bytesToAlloc << " bytes";
        throw GImage::Error(ss.str());
    }

    //since we are reading row by row, required to handle interlacing
    uint32 number_passes = png_set_interlace_handling(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    for (uint32 pass = 0; pass < number_passes; ++pass) {
        for (uint32 y = 0; y < (uint32)m_height; ++y) {
            png_bytep rowPointer = &m_byte[m_width * m_channels * y]; 
            png_read_rows(png_ptr, &rowPointer, NULL, 1);
        }
    }

    png_read_end(png_ptr, info_ptr);

    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
}

}
