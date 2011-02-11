/*
 * QEMU Cocoa CG display driver
 *
 * Copyright (c) 2008 Mike Kronenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#import <Cocoa/Cocoa.h>
#include <pthread.h>

#include "qemu-common.h"
#include "console.h"
#include "sysemu.h"

#ifndef MAC_OS_X_VERSION_10_4
#define MAC_OS_X_VERSION_10_4 1040
#endif
#ifndef MAC_OS_X_VERSION_10_5
#define MAC_OS_X_VERSION_10_5 1050
#endif
#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

//#define DEBUG
//#define QEMU_COCOA_THREADED

#ifdef DEBUG
#define COCOA_DEBUG(fmt, ...) { (void) fprintf (stdout, "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__); }
#else
#define COCOA_DEBUG(...)  ((void) 0)
#endif

#define cgrect(nsrect) (*(CGRect *)&(nsrect))

typedef struct {
    int width;
    int height;
    int bitsPerComponent;
    int bitsPerPixel;
} QEMUScreen;

int qemu_main(int argc, char **argv); // main defined in qemu/vl.c
static NSWindow *normalWindow;
static id cocoaView;
static DisplayChangeListener *dcl;
static int last_vm_running;

#ifdef CONFIG_GLES2
static int caption_update_requested;
#endif

static int gArgc;
static char **gArgv;

// keymap conversion
static const int keymap[] =
{
//  SdlI    macI    macH    SdlH    104xtH  104xtC  sdl
    30, //  0       0x00    0x1e            A       QZ_a
    31, //  1       0x01    0x1f            S       QZ_s
    32, //  2       0x02    0x20            D       QZ_d
    33, //  3       0x03    0x21            F       QZ_f
    35, //  4       0x04    0x23            H       QZ_h
    34, //  5       0x05    0x22            G       QZ_g
    44, //  6       0x06    0x2c            Z       QZ_z
    45, //  7       0x07    0x2d            X       QZ_x
    46, //  8       0x08    0x2e            C       QZ_c
    47, //  9       0x09    0x2f            V       QZ_v
    0,  //  10      0x0A    Undefined       (paragraph)
    48, //  11      0x0B    0x30            B       QZ_b
    16, //  12      0x0C    0x10            Q       QZ_q
    17, //  13      0x0D    0x11            W       QZ_w
    18, //  14      0x0E    0x12            E       QZ_e
    19, //  15      0x0F    0x13            R       QZ_r
    21, //  16      0x10    0x15            Y       QZ_y
    20, //  17      0x11    0x14            T       QZ_t
    2,  //  18      0x12    0x02            1       QZ_1
    3,  //  19      0x13    0x03            2       QZ_2
    4,  //  20      0x14    0x04            3       QZ_3
    5,  //  21      0x15    0x05            4       QZ_4
    7,  //  22      0x16    0x07            6       QZ_6
    6,  //  23      0x17    0x06            5       QZ_5
    13, //  24      0x18    0x0d            =       QZ_EQUALS
    10, //  25      0x19    0x0a            9       QZ_9
    8,  //  26      0x1A    0x08            7       QZ_7
    12, //  27      0x1B    0x0c            -       QZ_MINUS
    9,  //  28      0x1C    0x09            8       QZ_8
    11, //  29      0x1D    0x0b            0       QZ_0
    27, //  30      0x1E    0x1b            ]       QZ_RIGHTBRACKET
    24, //  31      0x1F    0x18            O       QZ_o
    22, //  32      0x20    0x16            U       QZ_u
    26, //  33      0x21    0x1a            [       QZ_LEFTBRACKET
    23, //  34      0x22    0x17            I       QZ_i
    25, //  35      0x23    0x19            P       QZ_p
    28, //  36      0x24    0x1c            ENTER   QZ_RETURN
    38, //  37      0x25    0x26            L       QZ_l
    36, //  38      0x26    0x24            J       QZ_j
    40, //  39      0x27    0x28            '       QZ_QUOTE
    37, //  40      0x28    0x25            K       QZ_k
    39, //  41      0x29    0x27            ;       QZ_SEMICOLON
    43, //  42      0x2A    0x2b            \       QZ_BACKSLASH
    51, //  43      0x2B    0x33            ,       QZ_COMMA
    53, //  44      0x2C    0x35            /       QZ_SLASH
    49, //  45      0x2D    0x31            N       QZ_n
    50, //  46      0x2E    0x32            M       QZ_m
    52, //  47      0x2F    0x34            .       QZ_PERIOD
    15, //  48      0x30    0x0f            TAB     QZ_TAB
    57, //  49      0x31    0x39            SPACE   QZ_SPACE
    41, //  50      0x32    0x29            `       QZ_BACKQUOTE
    14, //  51      0x33    0x0e            BKSP    QZ_BACKSPACE
    0,  //  52      0x34    Undefined
    1,  //  53      0x35    0x01            ESC     QZ_ESCAPE
    220,//  54      0x36    0xdc    e0,5c   R GUI   QZ_RMETA
    219,//  55      0x37    0xdb    e0,5b   L GUI   QZ_LMETA
    42, //  56      0x38    0x2a            L SHFT  QZ_LSHIFT
    58, //  57      0x39    0x3a            CAPS    QZ_CAPSLOCK
    56, //  58      0x3A    0x38            L ALT   QZ_LALT
    29, //  59      0x3B    0x1d            L CTRL  QZ_LCTRL
    54, //  60      0x3C    0x36            R SHFT  QZ_RSHIFT
    184,//  61      0x3D    0xb8    E0,38   R ALT   QZ_RALT
    157,//  62      0x3E    0x9d    E0,1D   R CTRL  QZ_RCTRL
    0,  //  63      0x3F    Undefined       (fn)
    0,  //  64      0x40    Undefined       (f17)
    83, //  65      0x53    0x53            KP .
    0,  //  66      0x42    Undefined
    55, //  67      0x43    0x37            KP *    QZ_KP_MULTIPLY
    0,  //  68      0x44    Undefined
    78, //  69      0x45    0x4e            KP +    QZ_KP_PLUS
    0,  //  70      0x46    Undefined
    69, //  71      0x47    0x45            NUM     QZ_NUMLOCK
    0,  //  72      0x48    Undefined
    0,  //  73      0x49    Undefined
    0,  //  74      0x4A    Undefined
    181,//  75      0x4B    0xb5    E0,35   KP /    QZ_KP_DIVIDE
    152,//  76      0x4C    0x9c    E0,1C   KP EN   QZ_KP_ENTER
    0,  //  77      0x4D    undefined
    74, //  78      0x4E    0x4a            KP -    QZ_KP_MINUS
    0,  //  79      0x4F    Undefined       (f18)
    0,  //  80      0x50    Undefined       (f19)
    0,  //  81      0x51                    (kp =)  QZ_KP_EQUALS
    82, //  82      0x52    0x52            KP 0    QZ_KP0
    79, //  83      0x53    0x4f            KP 1    QZ_KP1
    80, //  84      0x54    0x50            KP 2    QZ_KP2
    81, //  85      0x55    0x51            KP 3    QZ_KP3
    75, //  86      0x56    0x4b            KP 4    QZ_KP4
    76, //  87      0x57    0x4c            KP 5    QZ_KP5
    77, //  88      0x58    0x4d            KP 6    QZ_KP6
    71, //  89      0x59    0x47            KP 7    QZ_KP7
    0,  //  90      0x5A    Undefined
    72, //  91      0x5B    0x48            KP 8    QZ_KP8
    73, //  92      0x5C    0x49            KP 9    QZ_KP9
    0,  //  93      0x5D    Undefined
    0,  //  94      0x5E    Undefined
    0,  //  95      0x5F    Undefined
    63, //  96      0x60    0x3f            F5      QZ_F5
    64, //  97      0x61    0x40            F6      QZ_F6
    65, //  98      0x62    0x41            F7      QZ_F7
    61, //  99      0x63    0x3d            F3      QZ_F3
    66, //  100     0x64    0x42            F8      QZ_F8
    67, //  101     0x65    0x43            F9      QZ_F9
    0,  //  102     0x66    Undefined
    87, //  103     0x67    0x57            F11     QZ_F11
    0,  //  104     0x68    Undefined
    183,//  105     0x69    0xb7            (f13)   QZ_PRINT
    0,  //  106     0x6A    Undefined
    70, //  107     0x6B    0x46            SCROLL(f14)  QZ_SCROLLOCK
    0,  //  108     0x6C    Undefined
    68, //  109     0x6D    0x44            F10     QZ_F10
    0,  //  110     0x6E    Undefined
    88, //  111     0x6F    0x58            F12     QZ_F12
    0,  //  112     0x70    Undefined
    110,//  113     0x71    0x0             (f15)   QZ_PAUSE
    210,//  114     0x72    0xd2    E0,52   INSERT  QZ_INSERT
    199,//  115     0x73    0xc7    E0,47   HOME    QZ_HOME
    201,//  116     0x74    0xc9    E0,49   PG UP   QZ_PAGEUP
    211,//  117     0x75    0xd3    E0,53   DELETE  QZ_DELETE
    62, //  118     0x76    0x3e            F4      QZ_F4
    207,//  119     0x77    0xcf    E0,4f   END     QZ_END
    60, //  120     0x78    0x3c            F2      QZ_F2
    209,//  121     0x79    0xd1    E0,51   PG DN   QZ_PAGEDOWN
    59, //  122     0x7A    0x3b            F1      QZ_F1
    203,//  123     0x7B    0xcb    e0,4B   L ARROW QZ_LEFT
    205,//  124     0x7C    0xcd    e0,4D   R ARROW QZ_RIGHT
    208,//  125     0x7D    0xd0    E0,50   D ARROW QZ_DOWN
    200,//  126     0x7E    0xc8    E0,48   U ARROW QZ_UP
/* completed according to http://www.libsdl.org/cgi/cvsweb.cgi/SDL12/src/video/quartz/SDL_QuartzKeys.h?rev=1.6&content-type=text/x-cvsweb-markup */

/* Aditional 104 Key XP-Keyboard Scancodes from http://www.computer-engineering.org/ps2keyboard/scancodes1.html */
/*
    221 //          0xdd            e0,5d   APPS
        //              E0,2A,E0,37         PRNT SCRN
        //              E1,1D,45,E1,9D,C5   PAUSE
// ACPI Scan Codes
    222 //          0xde            E0, 5E  Power
    223 //          0xdf            E0, 5F  Sleep
    227 //          0xe3            E0, 63  Wake
// Windows Multimedia Scan Codes
    153 //          0x99            E0, 19  Next Track
    144 //          0x90            E0, 10  Previous Track
    164 //          0xa4            E0, 24  Stop
    162 //          0xa2            E0, 22  Play/Pause
    160 //          0xa0            E0, 20  Mute
    176 //          0xb0            E0, 30  Volume Up
    174 //          0xae            E0, 2E  Volume Down
    237 //          0xed            E0, 6D  Media Select
    236 //          0xec            E0, 6C  E-Mail
    161 //          0xa1            E0, 21  Calculator
    235 //          0xeb            E0, 6B  My Computer
    229 //          0xe5            E0, 65  WWW Search
    178 //          0xb2            E0, 32  WWW Home
    234 //          0xea            E0, 6A  WWW Back
    233 //          0xe9            E0, 69  WWW Forward
    232 //          0xe8            E0, 68  WWW Stop
    231 //          0xe7            E0, 67  WWW Refresh
    230 //          0xe6            E0, 66  WWW Favorites
*/
};

static int cocoa_keycode_to_qemu(int keycode)
{
    if((sizeof(keymap)/sizeof(int)) <= keycode)
    {
        printf("(cocoa) warning unknow keycode 0x%x\n", keycode);
        return 0;
    }
    return keymap[keycode];
}


@interface QemuFullScreenWindow : NSWindow
{
}
- (BOOL) canBecomeKeyWindow;
@end

@implementation QemuFullScreenWindow
- (BOOL) canBecomeKeyWindow { return YES; }
@end

/*
 ------------------------------------------------------
    QemuCocoaView
 ------------------------------------------------------
*/
@interface QemuCocoaView : NSView
{
    QEMUScreen screen;
    NSWindow *fullScreenWindow;
    float cx,cy,cw,ch,cdx,cdy;
    CGDataProviderRef dataProviderRef;
    void *dataRawPtr;
    int mouseX, mouseY;
    int modifiers_state[256];
    BOOL isMouseGrabed;
    BOOL isFullscreen;
    BOOL isAbsoluteEnabled;
    BOOL isTabletEnabled;
    BOOL isShuttingDownGuest;
#ifdef CONFIG_SKINNING
    BOOL isZoomEnabled;
    int zoomwidth, zoomheight, realwidth, realheight;
#endif
}
- (void) resizeFrameTo:(int)w height:(int)h;
- (void) resizeContentToWidth:(id)arg;
- (void) grabMouse;
- (void) ungrabMouse;
- (void) toggleFullScreen:(id)sender;
- (void) qemuReportMouseEvent:(NSEvent *)event position:(NSPoint)p buttons:(int)buttons;
- (void) setAbsoluteEnabled:(BOOL)tIsAbsoluteEnabled;
- (void) setShuttingDownGuest;
- (BOOL) isMouseGrabed;
- (BOOL) isAbsoluteEnabled;
- (float) cdx;
- (float) cdy;
- (QEMUScreen) gscreen;
- (void) updateCaption;
- (void) checkAndDisplayAltCursor;
#ifdef CONFIG_SKINNING
- (BOOL) isFullscreen;
- (void) enableZooming:(int)w height:(int)h displayState:(DisplayState *)ds;
#endif
@end

@implementation QemuCocoaView
- (id)initWithFrame:(NSRect)frameRect
{
    COCOA_DEBUG("");

    self = [super initWithFrame:frameRect];
    if (self) {

        screen.bitsPerComponent = 8;
        screen.bitsPerPixel = 32;
        screen.width = frameRect.size.width;
        screen.height = frameRect.size.height;

    }
    return self;
}

- (void) dealloc
{
    COCOA_DEBUG("");

    if (dataProviderRef)
        CGDataProviderRelease(dataProviderRef);

    [super dealloc];
}

- (BOOL) isOpaque
{
    return YES;
}

- (void) checkAndDisplayAltCursor
{
    if (multitouch_enabled && !isMouseGrabed && !cursor_hide &&
        dataRawPtr && modifiers_state[56]) {
        unsigned char *p = (unsigned char *)dataRawPtr;
        int altX = screen.width - mouseX;
        int altY = mouseY;
        int x, y;
        int bytesperpixel = screen.bitsPerPixel / 8;
        p += (altY * screen.width) * bytesperpixel;
        for (y = 0; y < 8; y++) {
            if (y + altY > 0 && y + altY < screen.height) {
                unsigned char *q = p + altX * bytesperpixel;
                for (x = 0; x < 8; x++) {
                    if (x + altX > 0 && x + altX < screen.width) {
                        int i;
                        for (i = 0; i < bytesperpixel; i++) {
                            q[i] ^= 0xff;
                        }
                    }
                    q += bytesperpixel;
                }
            }
            p += screen.width * bytesperpixel;
        }
    }
}

- (void) drawRect:(NSRect) rect
{
    COCOA_DEBUG("");

    // get CoreGraphic context
    CGContextRef viewContextRef = [[NSGraphicsContext currentContext] graphicsPort];
#ifdef CONFIG_SKINNING
	// For skinning make it look better
    CGContextSetInterpolationQuality (viewContextRef, kCGInterpolationHigh);
#else
    CGContextSetInterpolationQuality (viewContextRef, kCGInterpolationNone);
#endif
    CGContextSetShouldAntialias (viewContextRef, NO);

    int width, height;
#ifdef CONFIG_SKINNING
    if (isZoomEnabled && !isFullscreen && is_graphic_console()) {
        width = realwidth;
        height = realheight;
    } else
#endif
	{
        width = screen.width;
        height = screen.height;
    }

    // draw screen bitmap directly to Core Graphics context
    if (dataProviderRef) {
        [self checkAndDisplayAltCursor];
        CGImageRef imageRef = CGImageCreate(
            width,
            height,
            screen.bitsPerComponent, //bitsPerComponent
            screen.bitsPerPixel, //bitsPerPixel
            (width * (screen.bitsPerComponent/2)), //bytesPerRow
#ifdef __LITTLE_ENDIAN__
            CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB), //colorspace for OS X >= 10.4
            kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
#else
            CGColorSpaceCreateDeviceRGB(), //colorspace for OS X < 10.4 (actually ppc)
            kCGImageAlphaNoneSkipFirst, //bitmapInfo
#endif
            dataProviderRef, //provider
            NULL, //decode
            TRUE, //interpolate
            kCGRenderingIntentDefault //intent
        );
// test if host supports "CGImageCreateWithImageInRect" at compile time
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
        if (CGImageCreateWithImageInRect == NULL) { // test if "CGImageCreateWithImageInRect" is supported on host at runtime
#endif
            // compatibility drawing code (draws everything) (OS X < 10.4)
            CGContextDrawImage (viewContextRef, CGRectMake(0, 0, [self bounds].size.width, [self bounds].size.height), imageRef);
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4)
        } else {
            // selective drawing code (draws only dirty rectangles) (OS X >= 10.4)
            const NSRect *rectList;
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
            NSInteger rectCount;
#else
            int rectCount;
#endif
            int i;
            CGImageRef clipImageRef;
            CGRect clipRect;

            [self getRectsBeingDrawn:&rectList count:&rectCount];
            for (i = 0; i < rectCount; i++) {
                clipRect.origin.x = floorf(rectList[i].origin.x / cdx);
                clipRect.origin.y = floorf((float)height - (rectList[i].origin.y + rectList[i].size.height) / cdy);
                clipRect.size.width = ceilf(rectList[i].size.width / cdx);
                clipRect.size.height = ceilf(rectList[i].size.height / cdy);
                clipImageRef = CGImageCreateWithImageInRect(
                    imageRef,
                    clipRect
                );
                CGContextDrawImage (viewContextRef, cgrect(rectList[i]), clipImageRef);
                CGImageRelease (clipImageRef);
            }
        }
#endif
        CGImageRelease (imageRef);
    }
}

- (void) setContentDimensions
{
    COCOA_DEBUG("");

    if (isFullscreen) {
        cdx = [[NSScreen mainScreen] frame].size.width / (float)screen.width;
        cdy = [[NSScreen mainScreen] frame].size.height / (float)screen.height;
        cw = screen.width * cdx;
        ch = screen.height * cdy;
        cx = ([[NSScreen mainScreen] frame].size.width - cw) / 2.0;
        cy = ([[NSScreen mainScreen] frame].size.height - ch) / 2.0;
    }
#ifdef CONFIG_SKINNING
	else if (isZoomEnabled && is_graphic_console()) {
        cdx = zoomwidth / (float)realwidth;
        cdy = zoomheight / (float)realheight;
        cw = zoomwidth;
        ch = zoomheight;
        cx = 0;
        cy = 0;
    }
#endif
    else {
        cx = 0;
        cy = 0;
        cw = screen.width;
        ch = screen.height;
        cdx = 1.0;
        cdy = 1.0;
    }
}

- (void) resizeFrameTo:(int)w height:(int)h
{
    // update windows
    if (isFullscreen) {
        [[fullScreenWindow contentView] setFrame:[[NSScreen mainScreen] frame]];
        COCOA_DEBUG("resizing window: %4.0f,%4.0f %dx%4.0f", [normalWindow frame].origin.x, [normalWindow frame].origin.y - h + screen.height, w, h + [normalWindow frame].size.height - screen.height);
        [normalWindow setFrame:NSMakeRect([normalWindow frame].origin.x, [normalWindow frame].origin.y - h + screen.height, w, h + [normalWindow frame].size.height - screen.height) display:NO animate:NO];
#ifdef CONFIG_SKINNING
    } else if (isZoomEnabled && is_graphic_console()) {
        [normalWindow setFrame:NSMakeRect([normalWindow frame].origin.x, [normalWindow frame].origin.y - zoomheight + screen.height, zoomwidth, zoomheight + [normalWindow frame].size.height - screen.height) display:NO animate:NO];
#endif
    } else {
        [normalWindow setFrame:NSMakeRect([normalWindow frame].origin.x, [normalWindow frame].origin.y - h + screen.height, w, h + [normalWindow frame].size.height - screen.height) display:YES animate:NO];
    }
    [self updateCaption];
#ifdef CONFIG_SKINNING
    if (isZoomEnabled && !isFullscreen && is_graphic_console()) {
        screen.width = zoomwidth;
        screen.height = zoomheight;
    } else
#endif
    {
        screen.width = w;
        screen.height = h;
    }
    //[normalWindow center];
    [self setContentDimensions];
    [self setFrame:NSMakeRect(cx, cy, cw, ch)];
}

- (void) resizeContentToWidth:(id)arg
{
    COCOA_DEBUG("");
    DisplayState *ds = get_displaystate();
    int w = ds_get_width(ds);
    int h = ds_get_height(ds);

    // update screenBuffer
    if (dataProviderRef)
        CGDataProviderRelease(dataProviderRef);

    //sync host window color space with guests
	screen.bitsPerPixel = ds_get_bits_per_pixel(ds);
	screen.bitsPerComponent = ds_get_bytes_per_pixel(ds) * 2;

    dataRawPtr = is_graphic_console() ? ds_get_data(ds) : 0;
    dataProviderRef = CGDataProviderCreateWithData(NULL, ds_get_data(ds), w * 4 * h, NULL);
    
    // resize windows
    [self resizeFrameTo:w height:h];
}

- (void) toggleFullScreen:(id)sender
{
    COCOA_DEBUG("");

    if (isFullscreen) { // switch from fullscreen to desktop
        isFullscreen = FALSE;
#ifdef CONFIG_SKINNING
        if (isZoomEnabled && is_graphic_console()) {
            [self resizeFrameTo:realwidth height:realheight];
        }
#endif
        if (isMouseGrabed) [self ungrabMouse];
        [self setContentDimensions];
// test if host supports "exitFullScreenModeWithOptions" at compile time
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        if ([NSView respondsToSelector:@selector(exitFullScreenModeWithOptions:)]) { // test if "exitFullScreenModeWithOptions" is supported on host at runtime
            [self exitFullScreenModeWithOptions:nil];
        } else {
#endif
            [fullScreenWindow close];
            [normalWindow setContentView: self];
            [NSMenu setMenuBarVisible:YES];
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        }
#endif
        [normalWindow makeKeyAndOrderFront:self];
    } else { // switch from desktop to fullscreen
        isFullscreen = TRUE;
        isTabletEnabled = TRUE;
#ifdef CONFIG_SKINNING
        if (isZoomEnabled && is_graphic_console()) {
            [self resizeFrameTo:realwidth height:realheight];
        }
#endif
        if (cursor_allow_grab) [self grabMouse];
        [self setContentDimensions];
        [normalWindow orderOut:self];
// test if host supports "enterFullScreenMode:withOptions" at compile time
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        if ([NSView respondsToSelector:@selector(enterFullScreenMode:withOptions:)]) { // test if "enterFullScreenMode:withOptions" is supported on host at runtime
            [self enterFullScreenMode:[NSScreen mainScreen]
                          withOptions:[NSDictionary dictionaryWithObjectsAndKeys:
                                       [NSNumber numberWithBool:NO], NSFullScreenModeAllScreens,
                                       [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], kCGDisplayModeIsStretched,
                                        nil], NSFullScreenModeSetting,
                                       nil]];
        } else {
#endif
            [NSMenu setMenuBarVisible:NO];
            fullScreenWindow = [[QemuFullScreenWindow alloc] initWithContentRect:[[NSScreen mainScreen] frame]
                                                           styleMask:NSBorderlessWindowMask
                                                             backing:NSBackingStoreBuffered
                                                               defer:NO];
            [fullScreenWindow setHasShadow:NO];
            [fullScreenWindow setContentView:self];
            [fullScreenWindow makeKeyAndOrderFront:self];
            [fullScreenWindow setAcceptsMouseMovedEvents:YES];
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        }
#endif
    }
}

#ifdef CONFIG_SKINNING
- (void) enableZooming:(int)w height:(int)h displayState:(DisplayState *)ds
{
    COCOA_DEBUG("%d x %d", w, h);
    isZoomEnabled = TRUE;
    zoomwidth = w;
    zoomheight = h;
    realwidth = ds_get_width(ds);
    realheight = ds_get_height(ds);
#ifdef QEMU_COCOA_THREADED
    [self performSelectorOnMainThread:@selector(resizeContentToWidth:)
                           withObject:nil waitUntilDone:YES];
#else
    [self resizeContentToWidth:self];
#endif
}
#endif

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (BOOL) canBecomeKeyView
{
    return YES;
}

- (void) flagsChanged:(NSEvent *)event
{
    int keycode = cocoa_keycode_to_qemu([event keyCode]);
    if (keycode) {
        if (keycode == 58 || keycode == 69) { // emulate caps lock and num lock keydown and keyup
            kbd_put_keycode(keycode);
            kbd_put_keycode(keycode | 0x80);
        } else if (is_graphic_console()) {
            if (keycode & 0x80) {
                kbd_put_keycode(0xe0);
            }
            if (modifiers_state[keycode] == 0) { // keydown
                kbd_put_keycode(keycode & 0x7f);
                modifiers_state[keycode] = 1;
            } else { // keyup
                kbd_put_keycode(keycode | 0x80);
                modifiers_state[keycode] = 0;
            }
        }
    }

    // release Mouse grab when pressing ctrl+alt
    if (!isFullscreen) {
        if ((ctrl_grab && [event keyCode] == 0x3e)
            || ((([event modifierFlags] & NSControlKeyMask) &&
                 ([event modifierFlags] & NSAlternateKeyMask))
                && (!alt_grab || ([event modifierFlags] & NSShiftKeyMask)))) {
            [self ungrabMouse];
        }
    }
}

- (void) keyDown:(NSEvent *)event
{
    int keycode = cocoa_keycode_to_qemu([event keyCode]);

    // handle control + alt Key Combos (ctrl+alt is reserved for QEMU)
    if (([event modifierFlags] & NSControlKeyMask) && ([event modifierFlags] & NSAlternateKeyMask)) {
        switch (keycode) {
        // enable graphic console
        case 0x02 ... 0x0a: // '1' to '9' keys
            console_select(keycode - 0x02);
            break;
        }
    } else if (is_graphic_console()) { // handle keys for graphic console
        if (keycode & 0x80) { //check bit for e0 in front
            kbd_put_keycode(0xe0);
        }
        kbd_put_keycode(keycode & 0x7f); //remove e0 bit in front
    } else { // handle keys for Monitor
        int keysym = 0;
        int ctrl = [event modifierFlags] & NSControlKeyMask;
        switch ([event keyCode]) {
        case 115:
            keysym = ctrl ? QEMU_KEY_CTRL_HOME : QEMU_KEY_HOME;
            break;
        case 116:
            keysym = ctrl ? QEMU_KEY_CTRL_PAGEUP : QEMU_KEY_PAGEUP;
            break;
        case 117:
            keysym = QEMU_KEY_DELETE;
            break;
        case 119:
            keysym = ctrl ? QEMU_KEY_CTRL_END : QEMU_KEY_END;
            break;
        case 121:
            keysym = ctrl ? QEMU_KEY_CTRL_PAGEDOWN : QEMU_KEY_PAGEDOWN;
            break;
        case 123:
            keysym = ctrl ? QEMU_KEY_CTRL_LEFT : QEMU_KEY_LEFT;
            break;
        case 124:
            keysym = ctrl ? QEMU_KEY_CTRL_RIGHT : QEMU_KEY_RIGHT;
            break;
        case 125:
            keysym = ctrl ? QEMU_KEY_CTRL_DOWN : QEMU_KEY_DOWN;
            break;
        case 126:
            keysym = ctrl ? QEMU_KEY_CTRL_UP : QEMU_KEY_UP;
            break;
        default:
            {
                NSString *ks = [event characters];
                if ([ks length] > 0) {
                    keysym = [ks characterAtIndex:0];
                }
            }
        }
        if (keysym) {
            kbd_put_keysym(keysym);
        }
    }
}

- (void) keyUp:(NSEvent *)event
{
    if (is_graphic_console()) {
        int keycode = cocoa_keycode_to_qemu([event keyCode]);
        if (keycode & 0x80) {
            kbd_put_keycode(0xe0);
        }
        kbd_put_keycode(keycode | 0x80); //add 128 to signal release of key
    }
}

- (void) mouseMoved:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    mouseX = p.x;
    mouseY = p.y;
    if (isAbsoluteEnabled && !isFullscreen) {
        if (p.x < 0 || p.x > screen.width || p.y < 0 || p.y > screen.height || ![[self window] isKeyWindow]) {
            if (isTabletEnabled) { // if we leave the window, deactivate the tablet
                if (cursor_hide) {
                    [NSCursor unhide];
                }
                isTabletEnabled = FALSE;
            }
        } else {
            if (!isTabletEnabled) { // if we enter the window, activate the tablet
                if (cursor_hide) {
                    [NSCursor hide];
                }
                isTabletEnabled = TRUE;
            }
        }
    }
    [self qemuReportMouseEvent:event position:p buttons:0];
}

- (void) mouseDown:(NSEvent *)event
{
    int buttons;
    if ([event modifierFlags] & NSCommandKeyMask) {
        buttons = MOUSE_EVENT_RBUTTON;
    } else if ([event modifierFlags] & NSAlternateKeyMask) {
        buttons = MOUSE_EVENT_MBUTTON << 1;
    } else {
        buttons = MOUSE_EVENT_LBUTTON;
    }
    [self qemuReportMouseEvent:event position:[event locationInWindow] buttons:buttons];
}

- (void) rightMouseDown:(NSEvent *)event
{
    [self qemuReportMouseEvent:event position:[event locationInWindow] buttons:MOUSE_EVENT_RBUTTON];
}

- (void) otherMouseDown:(NSEvent *)event
{
    [self qemuReportMouseEvent:event position:[event locationInWindow] buttons:MOUSE_EVENT_MBUTTON];
}

- (void) mouseDragged:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    mouseX = p.x;
    mouseY = p.y;
    [self mouseDown:event];
}

- (void) rightMouseDragged:(NSEvent *)event
{
    [self rightMouseDown:event];
}

- (void) otherMouseDragged:(NSEvent *)event
{
    [self otherMouseDown:event];
}

- (void) mouseUp:(NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    if (isTabletEnabled || isMouseGrabed) {
        [self qemuReportMouseEvent:event position:p buttons:0];
    } else if (!isMouseGrabed) {
        if (p.x > -1 && p.x < screen.width && p.y > -1 && p.y < screen.height) {
            if (cursor_allow_grab) {
                [self grabMouse];
            }
        }
    }
}

- (void) rightMouseUp:(NSEvent *)event
{
    [self qemuReportMouseEvent:event position:[event locationInWindow] buttons:0];
}

- (void) otherMouseUp:(NSEvent *)event
{
    [self qemuReportMouseEvent:event position:[event locationInWindow] buttons:0];
}

- (void) scrollWheel:(NSEvent *)event
{
    if (isTabletEnabled || isMouseGrabed) {
        kbd_mouse_event(0, 0, -[event deltaY], 0);
    }
}

- (void) qemuReportMouseEvent:(NSEvent *)event position:(NSPoint)p buttons:(int)buttons
{
    if (isTabletEnabled) {
        if (isFullscreen) {
            NSSize fs = [[NSScreen mainScreen] frame].size;
            kbd_mouse_event((int)(p.x * 0x7FFF / (fs.width - 1)),
                            (int)((fs.height - p.y) * 0x7FFF / (fs.height - 1)),
                            0, buttons);
        } else {
            kbd_mouse_event((int)(p.x * 0x7FFF / (screen.width - 1)),
                            (int)((screen.height - p.y) * 0x7FFF / (screen.height - 1)),
                            0, buttons);
        }
    } else if (isMouseGrabed) {
        kbd_mouse_event((int)[event deltaX], (int)[event deltaY], 0, buttons);
    } else {
        if (isFullscreen) {
            NSSize fs = [[NSScreen mainScreen] frame].size;
            kbd_mouse_event((int)(p.x * 0x7FFF / (fs.width - 1)),
                            (int)((fs.height - p.y) * 0x7FFF / (fs.height - 1)),
                            0, buttons);
        }
    }
}

- (void) grabMouse
{
    COCOA_DEBUG("");
    if (cursor_hide) [NSCursor hide];
    CGAssociateMouseAndMouseCursorPosition(FALSE);
    isMouseGrabed = TRUE; // while isMouseGrabed = TRUE, QemuCocoaApp sends all events to [cocoaView handleEvent:]
    [self updateCaption];
}

- (void) ungrabMouse
{
    COCOA_DEBUG("");

    if (cursor_hide) [NSCursor unhide];
    CGAssociateMouseAndMouseCursorPosition(TRUE);
    isMouseGrabed = FALSE;
    [self updateCaption];
}

- (void) updateCaption
{
    NSMutableString *caption = [NSMutableString stringWithCapacity:10];
    [caption setString:@"QEMU"];
    if (qemu_name) {
        [caption appendFormat:@" (%s)", qemu_name];
    }
#ifdef CONFIG_GLES2
    char *gles2_backend = getenv("DGLES2_BACKEND");
    if (gles2_backend && !strncmp(gles2_backend, "osmesa", 6)) {
        [caption appendString:@" (softGL)"];
    }
    caption_update_requested = 0;
#endif
    if (isShuttingDownGuest) {
        [caption appendString:@" [Shutting down]"];
    }
    if (!vm_running) {
        [caption appendString:@" [Stopped]"];
    } else if (isMouseGrabed) {
        if (ctrl_grab) {
            [caption appendString:@" - Press Right-Ctrl to release mouse"];
        } else if (alt_grab) {
            [caption appendString:@" - Press Ctrl-Alt-Shift to release mouse"];
        } else {
            [caption appendString:@" - Press Ctrl-Alt to release mouse"];
        }
    }
    [normalWindow setTitle:caption];
}

- (void) setAbsoluteEnabled:(BOOL)tIsAbsoluteEnabled {isAbsoluteEnabled = tIsAbsoluteEnabled;}
- (void) setShuttingDownGuest { isShuttingDownGuest = YES; [self updateCaption]; }
- (BOOL) isMouseGrabed {return isMouseGrabed;}
- (BOOL) isAbsoluteEnabled {return isAbsoluteEnabled;}
- (float) cdx {return cdx;}
- (float) cdy {return cdy;}
- (QEMUScreen) gscreen {return screen;}
#ifdef CONFIG_SKINNING
- (BOOL) isFullscreen {return isFullscreen;}
#endif
@end



/*
 ------------------------------------------------------
    QemuCocoaAppController
 ------------------------------------------------------
*/
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
@interface QemuCocoaAppController : NSObject <NSWindowDelegate, NSApplicationDelegate>
#else
@interface QemuCocoaAppController : NSObject
#endif
{
}
- (void)startEmulationWithArgc:(int)argc argv:(char**)argv;
- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
- (void)toggleFullScreen:(id)sender;
#ifdef QEMU_COCOA_THREADED
- (void)runQemuThread:(id)arg;
#endif
@end

@implementation QemuCocoaAppController
- (id) init
{
    COCOA_DEBUG("");

    self = [super init];
    if (self) {

        // create a view and add it to the window
        cocoaView = [[QemuCocoaView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 640.0, 480.0)];
        if(!cocoaView) {
            fprintf(stderr, "(cocoa) can't create a view\n");
            exit(1);
        }

        // create a window
        normalWindow = [[NSWindow alloc] initWithContentRect:[cocoaView frame]
            styleMask:NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask
            backing:NSBackingStoreBuffered defer:NO];
        if(!normalWindow) {
            fprintf(stderr, "(cocoa) can't create window\n");
            exit(1);
        }
        [normalWindow setAcceptsMouseMovedEvents:YES];
        [normalWindow setTitle:[NSString stringWithFormat:@"QEMU"]];
        [normalWindow setContentView:cocoaView];
        [normalWindow useOptimizedDrawing:YES];
        [normalWindow makeKeyAndOrderFront:self];
		[normalWindow center];
        [normalWindow setDelegate:self];
        [cocoaView updateCaption];
    }
    return self;
}

- (void) dealloc
{
    COCOA_DEBUG("");

    if (cocoaView)
        [cocoaView release];
    [super dealloc];
}

- (void)applicationDidFinishLaunching: (NSNotification *) note
{
    COCOA_DEBUG("");

    // Display an open dialog box if no argument were passed or
    // if qemu was launched from the finder ( the Finder passes "-psn" )
    if ( gArgc <= 1 || strncmp ((char *)gArgv[1], "-psn", 4) == 0) {
        NSOpenPanel *op = [[NSOpenPanel alloc] init];
        [op setPrompt:@"Boot image"];
        [op setMessage:@"Select the disk image you want to boot.\n\nHit the \"Cancel\" button to quit"];
        [op beginSheetForDirectory:nil file:nil types:[NSArray arrayWithObjects:@"img",@"iso",@"dmg",@"qcow",@"cow",@"cloop",@"vmdk",nil]
              modalForWindow:normalWindow modalDelegate:self
              didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
    } else {
        // or Launch Qemu, with the global args
        [self startEmulationWithArgc:gArgc argv:(char **)gArgv];
    }
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    COCOA_DEBUG("");

    qemu_system_shutdown_request();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication
{
    return YES;
}

- (void)applicationDidBecomeActive:(NSNotification *)aNotification
{
    if (dcl) {
        dcl->gui_timer_interval = 0;
        dcl->idle = 0;
    }
}

- (void)applicationDidResignActive:(NSNotification *)aNotification
{
    if (dcl) {
        dcl->gui_timer_interval = 500;
        dcl->idle = 1;
    }
}

- (void)startEmulationWithArgc:(int)argc argv:(char**)argv
{
    COCOA_DEBUG("");
    gArgc = argc;
    gArgv = argv;
#ifdef QEMU_COCOA_THREADED
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGIO);
    [NSThread detachNewThreadSelector:@selector(runQemuThread:) toTarget:self withObject:nil];
    pthread_sigmask(SIG_BLOCK, &set, NULL);
#else
    exit(qemu_main(gArgc, gArgv));
#endif
}

#ifdef QEMU_COCOA_THREADED
- (void)runQemuThread:(id)arg
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
    [[NSThread currentThread] setName:@"QEMU_MainThread"];
#endif
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
    pthread_setname_np("QEMU_MainThread");
#endif
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGWINCH);
    sigaddset(&set, SIGINFO);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    int status = qemu_main(gArgc, gArgv);
    [pool release];
    exit(status);
}
#endif

- (void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    COCOA_DEBUG("");

    if (returnCode == NSCancelButton) {
        exit(0);
    } else if (returnCode == NSOKButton) {
        const char *bin = "qemu";
        char *img = (char*)[ [ sheet filename ] cStringUsingEncoding:NSASCIIStringEncoding];

        char **argv = (char**)malloc( sizeof(char*)*3 );

        asprintf(&argv[0], "%s", bin);
        asprintf(&argv[1], "-hda");
        asprintf(&argv[2], "%s", img);

        COCOA_DEBUG("Using argc %d argv %s -hda %s", 3, bin, img);

        [self startEmulationWithArgc:3 argv:(char**)argv];
    }
}

- (void)toggleFullScreen:(id)sender
{
    COCOA_DEBUG("");

    [cocoaView toggleFullScreen:sender];
}

- (BOOL)windowShouldClose:(id)sender
{
    if (!no_quit) {
        if (qemu_run_display_close_handler()) {
            return YES;
        }
        [cocoaView setShuttingDownGuest];
    }
    return NO;
}

- (void)confirmQuit:(id)sender
{
    NSAlert *alert = [NSAlert alertWithMessageText:@"Are you sure you want to quit QEMU?"
                                     defaultButton:@"No"
                                   alternateButton:@"Yes"
                                       otherButton:@"Shutdown"
                         informativeTextWithFormat:@"If you quit QEMU without shutting down the emulated machine properly you risk losing data and possibly corrupting the virtual machine contents."];
    [alert setAlertStyle:NSCriticalAlertStyle];
    switch ([alert runModal]) {
    case NSAlertDefaultReturn:
        /* do nothing */
        break;
    case NSAlertAlternateReturn:
        [NSApp terminate:sender];
        break;
    case NSAlertOtherReturn:
        if (qemu_run_display_close_handler()) {
            qemu_system_shutdown_request();
        } else {
            [cocoaView setShuttingDownGuest];
        }
        break;
    default:
        break;
    }
}

- (void)aboutQEMU:(id)sender
{
    [NSApp orderFrontStandardAboutPanelWithOptions:[NSDictionary dictionaryWithObjectsAndKeys:
                                                    @ QEMU_VERSION, @"ApplicationVersion",
                                                    @ QEMU_PKGVERSION, @"Version",
                                                    nil]];
}
@end

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
// Dock Connection
typedef struct CPSProcessSerNum
{
        UInt32                lo;
        UInt32                hi;
} CPSProcessSerNum;

extern OSErr    CPSGetCurrentProcess( CPSProcessSerNum *psn);
extern OSErr    CPSEnableForegroundOperation( CPSProcessSerNum *psn, UInt32 _arg2, UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
extern OSErr    CPSSetFrontProcess( CPSProcessSerNum *psn);
#endif

int main (int argc, const char * argv[])
{
    gArgc = argc;
    gArgv = (char **)argv;
    int i;

    /* In case we don't need to display a window, let's not do that */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-vnc") ||
            !strcmp(argv[i], "-nographic") ||
            !strcmp(argv[i], "-curses")) {
                return qemu_main(gArgc, gArgv);
        }
    }

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];

#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_5)
    CPSProcessSerNum PSN;
    if (!CPSGetCurrentProcess(&PSN))
        if (!CPSEnableForegroundOperation(&PSN,0x03,0x3C,0x2C,0x1103))
            if (!CPSSetFrontProcess(&PSN))
#else
    ProcessSerialNumber PSN;
    if (!GetCurrentProcess(&PSN) &&
        !TransformProcessType(&PSN, kProcessTransformToForegroundApplication) &&
        !SetFrontProcess(&PSN))
#endif
        [NSApplication sharedApplication];

    // Add menus
    NSMenu      *menu;
    NSMenuItem  *menuItem;

    [NSApp setMainMenu:[[NSMenu alloc] init]];

    // Application menu
    menu = [[NSMenu alloc] initWithTitle:@""];
    [menu addItemWithTitle:@"About QEMU" action:@selector(aboutQEMU:) keyEquivalent:@""]; // About QEMU
    [menu addItem:[NSMenuItem separatorItem]]; //Separator
    [menu addItemWithTitle:@"Hide QEMU" action:@selector(hide:) keyEquivalent:@"h"]; //Hide QEMU
    menuItem = (NSMenuItem *)[menu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"]; // Hide Others
    [menuItem setKeyEquivalentModifierMask:(NSAlternateKeyMask|NSCommandKeyMask)];
    [menu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""]; // Show All
    [menu addItem:[NSMenuItem separatorItem]]; //Separator
    [menu addItemWithTitle:@"Quit QEMU" action:@selector(confirmQuit:) keyEquivalent:@"q"];
    menuItem = [[NSMenuItem alloc] initWithTitle:@"Apple" action:nil keyEquivalent:@""];
    [menuItem setSubmenu:menu];
    [[NSApp mainMenu] addItem:menuItem];
#if (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6)
    // Workaround (this method is private since 10.4+)
    // not needed for 10.6+ since first menu is always the app menu
    [NSApp performSelector:@selector(setAppleMenu:) withObject:menu];
#endif

    // View menu
    menu = [[NSMenu alloc] initWithTitle:@"View"];
    [menu addItem: [[[NSMenuItem alloc] initWithTitle:@"Enter Fullscreen" action:@selector(toggleFullScreen:) keyEquivalent:@"f"] autorelease]]; // Fullscreen
    menuItem = [[[NSMenuItem alloc] initWithTitle:@"View" action:nil keyEquivalent:@""] autorelease];
    [menuItem setSubmenu:menu];
    [[NSApp mainMenu] addItem:menuItem];

    // Window menu
    menu = [[NSMenu alloc] initWithTitle:@"Window"];
    [menu addItem: [[[NSMenuItem alloc] initWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"] autorelease]]; // Miniaturize
    menuItem = [[[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""] autorelease];
    [menuItem setSubmenu:menu];
    [[NSApp mainMenu] addItem:menuItem];
    [NSApp setWindowsMenu:menu];

    // Create an Application controller
    QemuCocoaAppController *appController = [[QemuCocoaAppController alloc] init];
    [NSApp setDelegate:appController];

    // Start the main event loop
    [NSApp run];

    [appController release];
    [pool release];

    return 0;
}

#pragma mark qemu
static void cocoa_update(DisplayState *ds, int x, int y, int w, int h)
{
    COCOA_DEBUG("");

    NSRect rect;
    if ([cocoaView cdx] == 1.0) {
        rect = NSMakeRect(x, [cocoaView gscreen].height - y - h, w, h);
#ifdef CONFIG_SKINNING
    } else if (![cocoaView isFullscreen]) {
        rect = NSMakeRect(
            x * [cocoaView cdx],
            ([cocoaView gscreen].height) - ((y + h) * [cocoaView cdy]),
            w * [cocoaView cdx],
            h * [cocoaView cdy]);
#endif
    } else {
        rect = NSMakeRect(
            x * [cocoaView cdx],
            ([cocoaView gscreen].height - y - h) * [cocoaView cdy],
            w * [cocoaView cdx],
            h * [cocoaView cdy]);
    }
    [cocoaView setNeedsDisplayInRect:rect];
}

#ifdef CONFIG_GLES2
static void cocoa_updatecaption(void)
{
    caption_update_requested = 1;
}
#endif

static void cocoa_resize(DisplayState *ds)
{
    COCOA_DEBUG("");

#ifdef QEMU_COCOA_THREADED
    [cocoaView performSelectorOnMainThread:@selector(resizeContentToWidth:)
                                withObject:nil waitUntilDone:YES];
#else
    [cocoaView resizeContentToWidth:cocoaView];
#endif
}

static void cocoa_refresh(DisplayState *ds)
{
    COCOA_DEBUG("");

    if (last_vm_running != vm_running
#ifdef CONFIG_GLES2
        || caption_update_requested
#endif
        ) {
        last_vm_running = vm_running;
        [cocoaView updateCaption];
    }

    if (kbd_mouse_is_absolute()) {
        if (![cocoaView isAbsoluteEnabled]) {
            if ([cocoaView isMouseGrabed]) {
                [cocoaView ungrabMouse];
            }
        }
        [cocoaView setAbsoluteEnabled:YES];
    }

#ifndef QEMU_COCOA_THREADED
    NSDate *distantPast;
    NSEvent *event;
    distantPast = [NSDate distantPast];
    do {
        event = [NSApp nextEventMatchingMask:NSAnyEventMask untilDate:distantPast
                                      inMode: NSDefaultRunLoopMode dequeue:YES];
        if (event != nil) {
            [NSApp sendEvent:event];
        }
    } while(event != nil);
#endif

    if (is_graphic_console()) {
        vga_hw_update();
    }
}

#ifdef CONFIG_SKINNING
static void cocoa_enablezoom(DisplayState *ds, int width, int height)
{
    [cocoaView enableZooming:width height:height displayState:ds];
}

static void cocoa_getresolution(int *width, int *height)
{
    NSRect screenrect = [[NSScreen mainScreen] frame];
    COCOA_DEBUG("Screen dim: %f x %f", screenrect.size.width, screenrect.size.height);
    *width = (int)screenrect.size.width;
    *height = (int)screenrect.size.height;
}

#endif

static void cocoa_cleanup(void)
{
    COCOA_DEBUG("");
	qemu_free(dcl);
}

void cocoa_display_init(DisplayState *ds, int full_screen)
{
    COCOA_DEBUG("");

	dcl = qemu_mallocz(sizeof(DisplayChangeListener));
	
    // register vga output callbacks
    dcl->dpy_update = cocoa_update;
    dcl->dpy_resize = cocoa_resize;
    dcl->dpy_refresh = cocoa_refresh;
#ifdef CONFIG_GLES2
    dcl->dpy_updatecaption = cocoa_updatecaption;
    caption_update_requested = 0;
#endif
#ifdef CONFIG_SKINNING
    dcl->dpy_enablezoom = cocoa_enablezoom;
    dcl->dpy_getresolution = cocoa_getresolution;
#endif
	register_displaychangelistener(ds, dcl);

#ifdef CONFIG_SKINNING
	// We need a dummy resize call here, to realize the screensize detection
    dpy_resize(ds);
#endif

    // register cleanup function
    atexit(cocoa_cleanup);
}

#ifdef CONFIG_SKINNING
#include "skin/skin_image.h"
void *skin_loadpng(const char *fn, unsigned *w, unsigned *h)
{
    CGDataProviderRef dpRef = CGDataProviderCreateWithFilename(fn);
    if (dpRef == NULL) {
        fprintf(stderr, "%s: failed to create data provider\n", __FUNCTION__);
        return NULL;
    }
    CGImageRef iRef = CGImageCreateWithPNGDataProvider(dpRef, NULL, false,
                                                       kCGRenderingIntentDefault);
    CGDataProviderRelease(dpRef);
    if (iRef == NULL) {
        fprintf(stderr, "%s: failed to create image\n", __FUNCTION__);
        return NULL;
    }
    *w = CGImageGetWidth(iRef);
    *h = CGImageGetHeight(iRef);
    void *data = NULL;
    CGColorSpaceRef csRef = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    if (csRef == NULL) {
        fprintf(stderr, "%s: failed to allocate color space\n", __FUNCTION__);
    } else {
        data = qemu_mallocz(*w * *h * 4);
        if (data == NULL) {
            fprintf(stderr, "%s: failed to allocate memory\n", __FUNCTION__);
        } else {
            CGContextRef cRef = CGBitmapContextCreate(data, *w, *h, 8, *w * 4, csRef,
                                                      kCGImageAlphaPremultipliedFirst |
                                                      kCGBitmapByteOrder32Little);
            if (cRef == NULL) {
                fprintf(stderr, "%s: failed to create context\n", __FUNCTION__);
                qemu_free(data);
                data = NULL;
            } else {
                CGRect rect = {{0, 0}, {*w, *h}};
                CGContextDrawImage(cRef, rect, iRef);
                CGContextRelease(cRef);
            }
        }
        CGColorSpaceRelease(csRef);
    }
    CGImageRelease(iRef);
    return data;
}
#endif
