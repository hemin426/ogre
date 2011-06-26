/*
 -----------------------------------------------------------------------------
 This source file is part of OGRE
 (Object-oriented Graphics Rendering Engine)
 For the latest info, see http://www.ogre3d.org/
 
 Copyright (c) 2000-2009 Torus Knot Software Ltd
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 -----------------------------------------------------------------------------
 */

#ifndef __SampleBrowser_iOS_H__
#define __SampleBrowser_iOS_H__

#include "OgrePlatform.h"

#if OGRE_PLATFORM != OGRE_PLATFORM_APPLE_IOS
#error This header is for use with iOS only
#endif

#import <UIKit/UIKit.h> 
#import <QuartzCore/QuartzCore.h>

// To use CADisplayLink for smoother animation on iPhone comment out
// the following line or define it to 1.  Use with caution, it can
// sometimes cause input lag.
#define USE_CADISPLAYLINK 1

#ifdef __OBJC__

@interface AppDelegate : NSObject <UIApplicationDelegate>
{
    NSTimer *mTimer;
    OgreBites::SampleBrowser sb;

    // Use of the CADisplayLink class is the preferred method for controlling your animation timing.
    // CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
    // The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
    // isn't available.
    id mDisplayLink;
    NSDate* mDate;
    NSTimeInterval mLastFrameTime;
    BOOL mDisplayLinkSupported;
    BOOL mIsAtLeastiOS4;
}

- (void)go;
- (void)renderOneFrame:(id)sender;

@property (retain) NSTimer *mTimer;
@property (nonatomic) NSTimeInterval mLastFrameTime;

@end

@implementation AppDelegate

@synthesize mTimer;
@dynamic mLastFrameTime;

- (NSTimeInterval)mLastFrameTime
{
    return mLastFrameTime;
}

- (void)setLastFrameTime:(NSTimeInterval)frameInterval
{
    // Frame interval defines how many display frames must pass between each time the
    // display link fires. The display link will only fire 30 times a second when the
    // frame internal is two on a display that refreshes 60 times a second. The default
    // frame interval setting of one will fire 60 times a second when the display refreshes
    // at 60 times a second. A frame interval setting of less than one results in undefined
    // behavior.
    if (frameInterval >= 1)
    {
        mLastFrameTime = frameInterval;
    }
}

- (void)go {

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    try {
        sb.go();

        Ogre::Root::getSingleton().getRenderSystem()->_initRenderTargets();

        // Clear event times
		Ogre::Root::getSingleton().clearEventTimes();
    } catch( Ogre::Exception& e ) {
        std::cerr << "An exception has occurred: " <<
        e.getFullDescription().c_str() << std::endl;
    }

    if (mDisplayLinkSupported)
    {
        // CADisplayLink is API new to iPhone SDK 3.1. Compiling against earlier versions will result in a warning, but can be dismissed
        // if the system version runtime check for CADisplayLink exists in -initWithCoder:. The runtime check ensures this code will
        // not be called in system versions earlier than 3.1.
        mDate = [[NSDate alloc] init];
        mLastFrameTime = -[mDate timeIntervalSinceNow];
        
        mDisplayLink = [NSClassFromString(@"CADisplayLink") displayLinkWithTarget:self selector:@selector(renderOneFrame:)];
        [mDisplayLink setFrameInterval:mLastFrameTime];
        [mDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    }
    else
    {
        mTimer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0f / 60.0f) * mLastFrameTime
                                                  target:self
                                                selector:@selector(renderOneFrame:)
                                                userInfo:nil
                                                 repeats:YES];
    }

    [pool release];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    mDisplayLinkSupported = NO;
    mLastFrameTime = 1;
    mDisplayLink = nil;
    mTimer = nil;
    mIsAtLeastiOS4 = NO;

    if([[[UIDevice currentDevice] systemVersion] floatValue] >= 4.0)
        mIsAtLeastiOS4 = YES;

    // A system version of 3.1 or greater is required to use CADisplayLink. The NSTimer
    // class is used as fallback when it isn't available.
#if USE_CADISPLAYLINK
    NSString *reqSysVer = @"3.1";
    NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
    if ([currSysVer compare:reqSysVer options:NSNumericSearch] != NSOrderedAscending)
        mDisplayLinkSupported = YES;
#endif
    
    [self go];

    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    Ogre::Root::getSingleton().queueEndRendering();

    if (mDisplayLinkSupported)
    {
        [mDate release];
        mDate = nil;
        
        [mDisplayLink invalidate];
        mDisplayLink = nil;
    }
    else
    {
        [mTimer invalidate];
        mTimer = nil;
    }
    
    sb.shutdown();
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    Ogre::Root::getSingleton().saveConfig();
}

- (void)renderOneFrame:(id)sender
{
    [sb.mGestureView becomeFirstResponder];

    if (mDisplayLinkSupported)
    {
        // NSTimerInterval is a simple typedef for double
        NSTimeInterval currentFrameTime = -[mDate timeIntervalSinceNow];
        NSTimeInterval differenceInSeconds = currentFrameTime - mLastFrameTime;
        mLastFrameTime = currentFrameTime;

#if __IPHONE_4_0
        if(mIsAtLeastiOS4)
            dispatch_async(dispatch_get_main_queue(), ^(void)
            {
                Root::getSingleton().renderOneFrame((Real)differenceInSeconds);
            });
        else
#endif
            Root::getSingleton().renderOneFrame((Real)differenceInSeconds);
    }
    else
    {
        Root::getSingleton().renderOneFrame((Real)[mTimer timeInterval]);
    }
}

@end

#endif

#endif
