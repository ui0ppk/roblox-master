//
//  MachOBaseAddr.m
//  App
//
//  Created on 11/10/14.
//
//

#import <Foundation/Foundation.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>

uint32_t staticBaseAddress(void)
{
    const struct segment_command* command = getsegbyname("__TEXT");
    uint32_t addr = command->vmaddr;
    return addr;
}

intptr_t imageSlide(void)
{
    int image_count = _dyld_image_count();
    const struct mach_header *header;
    
    for (int index = 0; index < image_count; ++index)
    {
        header = _dyld_get_image_header(index);
        if (header->filetype == MH_EXECUTE)
        {
            return _dyld_get_image_vmaddr_slide(index);
        }
    }
    return 0;
}

uint32_t machODynamicBaseAddress(void)
{
    return staticBaseAddress() + imageSlide();
}

uint32_t machOTextSize(void)
{
    const struct segment_command* command = getsegbyname("__TEXT");
    return command->vmsize;
}