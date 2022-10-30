#include "path.h"

#import <Cocoa/Cocoa.h>

const char* Path::GetCurrentPath() {
    return [[[NSBundle mainBundle] bundlePath] UTF8String];
}
