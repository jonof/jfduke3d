//-------------------------------------------------------------------------
/*
 Copyright (C) 2013 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFDuke3D

 Shadow Warrior is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 See the GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
//-------------------------------------------------------------------------

#import <Cocoa/Cocoa.h>

#define GetTime xGetTime
#include "duke3d.h"
#undef GetTime
#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "grpscan.h"
#include "mmulti.h"

#import "GrpFile.h"
#import "GameListSource.h"

static struct {
    int fullscreen;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
    char selectedgrp[BMAX_PATH+1];
    int game;
    int samplerate, bitspersample, channels;
    int usemouse, usejoystick;

    int multiargc;
    char const * *multiargv;
    char *multiargstr;
} settings;

static struct soundQuality_t {
    int frequency;
    int samplesize;
    int channels;
} soundQualities[] = {
    { 44100, 16, 2 },
    { 22050, 16, 2 },
    { 11025, 16, 2 },
    { 0, 0, 0 },    // May be overwritten by custom sound settings.
    { 0, 0, 0 },
};


@interface StartupWinController : NSWindowController <NSWindowDelegate>
{
    BOOL quiteventonclose;
    NSMutableArray *modeslist3d;
    GameListSource *gamelistsrc;

    IBOutlet NSButton *alwaysShowButton;
    IBOutlet NSButton *fullscreenButton;
    IBOutlet NSButton *useMouseButton;
    IBOutlet NSButton *useJoystickButton;
    IBOutlet NSTextView *messagesView;
    IBOutlet NSTabView *tabView;
    IBOutlet NSTabViewItem *tabConfig;
    IBOutlet NSTabViewItem *tabGame;
    IBOutlet NSTabViewItem *tabMessages;
    IBOutlet NSPopUpButton *videoMode3DPUButton;
    IBOutlet NSPopUpButton *soundQualityPUButton;

    IBOutlet NSButton *singlePlayerButton;
    IBOutlet NSButton *multiPlayerButton;
    IBOutlet NSTextField *numPlayersField;
    IBOutlet NSStepper *numPlayersStepper;
    IBOutlet NSTextField *playerHostsLabel;
    IBOutlet NSTextField *playerHostsField;
    IBOutlet NSButton *peerToPeerButton;

    IBOutlet NSScrollView *gameList;

    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *startButton;
}

- (void)dealloc;
- (void)closeQuietly;
- (void)populateVideoModes:(BOOL)firstTime;
- (void)populateSoundQuality:(BOOL)firstTime;

- (IBAction)alwaysShowClicked:(id)sender;
- (IBAction)fullscreenClicked:(id)sender;

- (IBAction)multiPlayerModeClicked:(id)sender;
- (IBAction)peerToPeerModeClicked:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)start:(id)sender;

- (void)setupRunMode;
- (void)setupMessagesMode:(BOOL)allowCancel;
- (void)putsMessage:(NSString *)str;
- (void)setTitle:(NSString *)str;

- (void)setupPlayerHostsField;
@end

@implementation StartupWinController

- (void)windowDidLoad
{
    quiteventonclose = TRUE;
}

- (void)dealloc
{
    [gamelistsrc release];
    [modeslist3d release];
    [super dealloc];
}

// Close the window, but don't cause the quitevent flag to be set
// as though this is a cancel operation.
- (void)closeQuietly
{
    quiteventonclose = FALSE;
    [self close];
}

- (void)populateVideoModes:(BOOL)firstTime
{
    int i, mode3d, fullscreen = ([fullscreenButton state] == NSOnState);
    int idx3d = -1;
    int xdim, ydim, bpp = 8;

    if (firstTime) {
        xdim = settings.xdim3d;
        ydim = settings.ydim3d;
        bpp  = settings.bpp3d;
    } else {
        mode3d = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
        if (mode3d >= 0) {
            xdim = validmode[mode3d].xdim;
            ydim = validmode[mode3d].ydim;
            bpp = validmode[mode3d].bpp;
        }

    }
    mode3d = checkvideomode(&xdim, &ydim, bpp, fullscreen, 1);
    if (mode3d < 0) {
        int i, cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i]; ) { if (cd[i] >= bpp) i++; else break; }
        for ( ; cd[i]; i++) {
            mode3d = checkvideomode(&xdim, &ydim, cd[i], fullscreen, 1);
            if (mode3d < 0) continue;
            break;
        }
    }

    [modeslist3d release];
    [videoMode3DPUButton removeAllItems];

    modeslist3d = [[NSMutableArray alloc] init];

    for (i = 0; i < validmodecnt; i++) {
        if (fullscreen == validmode[i].fs) {
            if (i == mode3d) idx3d = [modeslist3d count];
            [modeslist3d addObject:[NSNumber numberWithInt:i]];
            [videoMode3DPUButton addItemWithTitle:[NSString stringWithFormat:@"%d %C %d %d-bpp",
                                                   validmode[i].xdim, 0xd7, validmode[i].ydim, validmode[i].bpp]];
        }
    }

    if (idx3d >= 0) [videoMode3DPUButton selectItemAtIndex:idx3d];
}

- (void)populateSoundQuality:(BOOL)firstTime
{
    int i, curidx = -1;

    [soundQualityPUButton removeAllItems];

    for (i = 0; soundQualities[i].frequency > 0; i++) {
        NSString *s = [NSString stringWithFormat:@"%dkHz, %d-bit, %s",
                            soundQualities[i].frequency / 1000,
                            soundQualities[i].samplesize,
                            soundQualities[i].channels == 1 ? "Mono" : "Stereo"
                     ];
        [soundQualityPUButton addItemWithTitle:s];

        if (firstTime &&
            soundQualities[i].frequency == settings.samplerate &&
            soundQualities[i].samplesize == settings.bitspersample &&
            soundQualities[i].channels == settings.channels) {
            curidx = i;
        }
    }

    if (firstTime && curidx < 0) {
        soundQualities[i].frequency = settings.samplerate;
        soundQualities[i].samplesize = settings.bitspersample;
        soundQualities[i].channels = settings.channels;

        NSString *s = [NSString stringWithFormat:@"%dkHz, %d-bit, %s",
                       soundQualities[i].frequency / 1000,
                       soundQualities[i].samplesize,
                       soundQualities[i].channels == 1 ? "Mono" : "Stereo"
                       ];
        [soundQualityPUButton addItemWithTitle:s];

        curidx = i;
    }

    if (curidx >= 0) {
        [soundQualityPUButton selectItemAtIndex:curidx];
    }
}

- (IBAction)alwaysShowClicked:(id)sender
{
}

- (IBAction)fullscreenClicked:(id)sender
{
    [self populateVideoModes:NO];
}

- (IBAction)multiPlayerModeClicked:(id)sender
{
    [singlePlayerButton setState:(sender == singlePlayerButton ? NSOnState : NSOffState)];
    [multiPlayerButton setState:(sender == multiPlayerButton ? NSOnState : NSOffState)];

    [peerToPeerButton setEnabled:(sender == multiPlayerButton)];
    [playerHostsField setEnabled:(sender == multiPlayerButton)];
    [self setupPlayerHostsField];

    BOOL enableNumPlayers = (sender == multiPlayerButton) && ([peerToPeerButton state] == NSOffState);
    [numPlayersField setEnabled:enableNumPlayers];
    [numPlayersStepper setEnabled:enableNumPlayers];
}

- (IBAction)peerToPeerModeClicked:(id)sender
{
    BOOL enableNumPlayers = ([peerToPeerButton state] == NSOffState);
    [numPlayersField setEnabled:enableNumPlayers];
    [numPlayersStepper setEnabled:enableNumPlayers];
    [self setupPlayerHostsField];
}

- (void)setupPlayerHostsField
{
    if ([peerToPeerButton state] == NSOffState) {
        [playerHostsLabel setStringValue:@"Host address:"];
        [playerHostsField setPlaceholderString:@"Leave empty to host the game."];
    } else {
        [playerHostsLabel setStringValue:@"Player addresses:"];
        [playerHostsField setPlaceholderString:@"List each address in order. Use * to indicate this machine's position."];
    }
}

- (void)windowWillClose:(NSNotification *)notification
{
    [NSApp stopModalWithCode:STARTWIN_CANCEL];
    quitevent = quitevent || quiteventonclose;
}

- (IBAction)cancel:(id)sender
{
    [NSApp stopModalWithCode:STARTWIN_CANCEL];
    quitevent = quitevent || quiteventonclose;
}

- (IBAction)start:(id)sender
{
    int retval;
    int mode = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
    if (mode >= 0) {
        settings.xdim3d = validmode[mode].xdim;
        settings.ydim3d = validmode[mode].ydim;
        settings.bpp3d = validmode[mode].bpp;
        settings.fullscreen = validmode[mode].fs;
    }

    int quality = [soundQualityPUButton indexOfSelectedItem];
    if (quality >= 0) {
        settings.samplerate = soundQualities[quality].frequency;
        settings.bitspersample = soundQualities[quality].samplesize;
        settings.channels = soundQualities[quality].channels;
    }

    settings.usemouse = [useMouseButton state] == NSOnState;
    settings.usejoystick = [useJoystickButton state] == NSOnState;

    int row = [[gameList documentView] selectedRow];
    if (row >= 0) {
        struct grpfile *p = [[gamelistsrc grpAtIndex:row] entryptr];
        if (p) {
            strcpy(settings.selectedgrp, p->name);
            settings.game = p->game;
        }
    }

    if ([multiPlayerButton state] == NSOnState) {
        @autoreleasepool {
            NSMutableArray *args = [[[NSMutableArray alloc] init] autorelease];
            NSString *str;
            int argstrlen = 0, i;
            char *argstrptr;

            // The game type parameter.
            if ([peerToPeerButton state] == NSOnState) {
                str = [[NSString alloc] initWithString:@"-n1"];
            } else {
                str = [[NSString alloc] initWithFormat:@"-n0:%d", [numPlayersField intValue]];
            }
            [str autorelease];
            [args addObject:str];
            argstrlen += [str lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;

            // The peer list.
            NSCharacterSet *split = [NSCharacterSet characterSetWithCharactersInString:@" ,\n\t"];
            NSArray *parts = [[playerHostsField stringValue] componentsSeparatedByCharactersInSet:split];
            NSEnumerator *iterparts = [parts objectEnumerator];

            while (str = (NSString *)[iterparts nextObject]) {
                if ([str length] == 0) continue;
                [args addObject:str];
                argstrlen += [str lengthOfBytesUsingEncoding:NSUTF8StringEncoding] + 1;
            }

            settings.multiargc = [args count];
            settings.multiargv = (char const * *)malloc(settings.multiargc * sizeof(char *));
            settings.multiargstr = (char *)malloc(argstrlen);

            argstrptr = settings.multiargstr;
            for (i = 0; i < settings.multiargc; i++) {
                str = (NSString *)[args objectAtIndex:i];

                strcpy(argstrptr, [str cStringUsingEncoding:NSUTF8StringEncoding]);
                settings.multiargv[i] = argstrptr;
                argstrptr += strlen(argstrptr) + 1;
            }
        }

        retval = STARTWIN_RUN_MULTI;
    } else {
        settings.multiargc = 0;
        settings.multiargv = NULL;
        settings.multiargstr = NULL;

        retval = STARTWIN_RUN;
    }

    settings.forcesetup = [alwaysShowButton state] == NSOnState;

    [NSApp stopModalWithCode:retval];
}

- (void)setupRunMode
{
    [alwaysShowButton setState: (settings.forcesetup ? NSOnState : NSOffState)];
    [alwaysShowButton setEnabled:YES];

    getvalidmodes();
    [videoMode3DPUButton setEnabled:YES];
    [self populateVideoModes:YES];
    [fullscreenButton setEnabled:YES];
    [fullscreenButton setState: (settings.fullscreen ? NSOnState : NSOffState)];

    [soundQualityPUButton setEnabled:YES];
    [self populateSoundQuality:YES];
    [useMouseButton setState: (settings.usemouse ? NSOnState : NSOffState)];
    [useMouseButton setEnabled:YES];
    [useJoystickButton setState: (settings.usejoystick ? NSOnState : NSOffState)];
    [useJoystickButton setEnabled:YES];

    [singlePlayerButton setEnabled:YES];
    [singlePlayerButton setState:NSOnState];
    [multiPlayerButton setEnabled:YES];
    [multiPlayerButton setState:NSOffState];
    [numPlayersField setEnabled:NO];
    [numPlayersField setIntValue:2];
    [numPlayersStepper setEnabled:NO];
    [numPlayersStepper setMaxValue:MAXPLAYERS];
    [peerToPeerButton setEnabled:NO];
    [playerHostsField setEnabled:NO];
    [self setupPlayerHostsField];

    gamelistsrc = [[GameListSource alloc] init];
    [[gameList documentView] setDataSource:gamelistsrc];
    [[gameList documentView] deselectAll:nil];

    int row = [gamelistsrc findIndexForGrpname:[NSString stringWithUTF8String:settings.selectedgrp]];
    if (row >= 0) {
        [[gameList documentView] scrollRowToVisible:row];
        [[gameList documentView] selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
    }

    [cancelButton setEnabled:YES];
    [startButton setEnabled:YES];

    [tabView selectTabViewItem:tabConfig];
    [NSCursor unhide];  // Why should I need to do this?
}

- (void)setupMessagesMode:(BOOL)allowCancel
{
    [tabView selectTabViewItem:tabMessages];

    // disable all the controls on the Configuration page
    NSEnumerator *enumerator = [[[tabConfig view] subviews] objectEnumerator];
    NSControl *control;
    while (control = [enumerator nextObject]) {
        [control setEnabled:false];
    }

    [alwaysShowButton setEnabled:NO];

    [cancelButton setEnabled:allowCancel];
    [startButton setEnabled:false];
}

- (void)putsMessage:(NSString *)str
{
    NSRange end;
    NSTextStorage *text = [messagesView textStorage];
    BOOL shouldAutoScroll;

    shouldAutoScroll = ((int)NSMaxY([messagesView bounds]) == (int)NSMaxY([messagesView visibleRect]));

    end.location = [text length];
    end.length = 0;

    [text beginEditing];
    [messagesView replaceCharactersInRange:end withString:str];
    [text endEditing];

    if (shouldAutoScroll) {
        end.location = [text length];
        end.length = 0;
        [messagesView scrollRangeToVisible:end];
    }
}

- (void)setTitle:(NSString *)str
{
    [[self window] setTitle:str];
}

@end

static StartupWinController *startwin = nil;

int startwin_open(void)
{
    if (startwin != nil) return 1;

    startwin = [[StartupWinController alloc] initWithWindowNibName:@"startwin.game"];
    if (startwin == nil) return -1;

    [startwin window];  // Forces the window controls on the controller to be initialised.
    [startwin setupMessagesMode:YES];
    [startwin showWindow:nil];

    return 0;
}

int startwin_close(void)
{
    if (startwin == nil) return 1;

    [startwin closeQuietly];
    [startwin release];
    startwin = nil;

    return 0;
}

int startwin_puts(const char *s)
{
    NSString *ns;

    if (!s) return -1;
    if (startwin == nil) return 1;

    ns = [[NSString alloc] initWithUTF8String:s];
    [startwin putsMessage:ns];
    [ns release];

    return 0;
}

int startwin_settitle(const char *s)
{
    NSString *ns;

    if (!s) return -1;
    if (startwin == nil) return 1;

    ns = [[NSString alloc] initWithUTF8String:s];
    [startwin setTitle:ns];
    [ns release];

    return 0;
}

int startwin_idle(void *v)
{
    return 0;
}

extern char *duke3dgrp;

int startwin_run(void)
{
    int retval;

    if (startwin == nil) return 0;

    settings.fullscreen = ScreenMode;
    settings.xdim3d = ScreenWidth;
    settings.ydim3d = ScreenHeight;
    settings.bpp3d = ScreenBPP;
    settings.samplerate = MixRate;
    settings.bitspersample = NumBits;
    settings.channels = NumChannels;
    settings.usemouse = UseMouse;
    settings.usejoystick = UseJoystick;
    settings.forcesetup = ForceSetup;
    settings.game = gametype;
    strncpy(settings.selectedgrp, duke3dgrp, BMAX_PATH);

    [startwin setupRunMode];

    switch ([NSApp runModalForWindow:[startwin window]]) {
        case STARTWIN_RUN_MULTI: retval = STARTWIN_RUN_MULTI; break;
        case STARTWIN_RUN: retval = STARTWIN_RUN; break;
        case STARTWIN_CANCEL: retval = STARTWIN_CANCEL; break;
        default: retval = -1;
    }

    [startwin setupMessagesMode:(retval == STARTWIN_RUN_MULTI)];

    if (retval) {
        ScreenMode = settings.fullscreen;
        ScreenWidth = settings.xdim3d;
        ScreenHeight = settings.ydim3d;
        ScreenBPP = settings.bpp3d;
        MixRate = settings.samplerate;
        NumBits = settings.bitspersample;
        NumChannels = settings.channels;
        UseMouse = settings.usemouse;
        UseJoystick = settings.usejoystick;
        ForceSetup = settings.forcesetup;
        duke3dgrp = settings.selectedgrp;
        gametype = settings.game;

        if (retval == STARTWIN_RUN_MULTI) {
            if (!initmultiplayersparms(settings.multiargc, settings.multiargv)) {
                retval = STARTWIN_RUN;
            }
            free(settings.multiargv);
            free(settings.multiargstr);
        }
    }

    return retval;
}
