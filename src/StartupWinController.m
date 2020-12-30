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

#include "compat.h"
#include "baselayer.h"
#include "build.h"
#include "grpscan.h"
#define GetTime xGetTime
#include "duke3d.h"
#undef GetTime

#import "GameListSource.h"

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

#include <stdlib.h>

@interface StartupWinController : NSWindowController <NSWindowDelegate>
{
    BOOL quiteventonclose;
    GameListSource *gamelistsrc;
    BOOL inmodal;
    struct startwin_settings *settings;

    IBOutlet NSButton *alwaysShowButton;
    IBOutlet NSButton *fullscreenButton;
    IBOutlet NSTextView *messagesView;
    IBOutlet NSTabView *tabView;
    IBOutlet NSTabViewItem *tabConfig;
    IBOutlet NSTabViewItem *tabMessages;
    IBOutlet NSPopUpButton *videoMode3DPUButton;

    IBOutlet NSPopUpButton *soundQualityPUButton;
    IBOutlet NSButton *useMouseButton;
    IBOutlet NSButton *useJoystickButton;

    IBOutlet NSButton *singlePlayerButton;
    IBOutlet NSButton *joinMultiButton;
    IBOutlet NSTextField *hostField;
    IBOutlet NSButton *hostMultiButton;
    IBOutlet NSTextField *numPlayersField;
    IBOutlet NSStepper *numPlayersStepper;

    IBOutlet NSTabViewItem *tabGame;
    IBOutlet NSScrollView *gameList;

    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *startButton;
}

- (int)modalRun;
- (void)closeQuietly;
- (void)populateVideoModes:(BOOL)firstTime;
- (void)populateSoundQuality:(BOOL)firstTime;

- (IBAction)fullscreenClicked:(id)sender;

- (IBAction)multiPlayerModeClicked:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)start:(id)sender;

- (void)setupConfigMode;
- (void)setupMessagesMode:(BOOL)allowCancel;
- (void)putsMessage:(NSString *)str;
- (void)setTitle:(NSString *)str;
- (void)setSettings:(struct startwin_settings *)theSettings;

@end

@implementation StartupWinController

- (void)windowDidLoad
{
    quiteventonclose = TRUE;
}

- (BOOL)windowShouldClose:(id)sender
{
    if (inmodal) {
        [NSApp stopModalWithCode:STARTWIN_CANCEL];
    }
    quitevent = quitevent || quiteventonclose;
    return NO;
}

- (int)modalRun
{
    int retval;

    inmodal = YES;
    switch ([NSApp runModalForWindow:[self window]]) {
        case STARTWIN_RUN: retval = STARTWIN_RUN; break;
        case STARTWIN_CANCEL: retval = STARTWIN_CANCEL; break;
        default: retval = -1;
    }
    inmodal = NO;

    return retval;
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
    int i, mode3d = -1;
    int xdim = 0, ydim = 0, bpp = 0, fullscreen = 0;
    int cd[] = { 32, 24, 16, 15, 8, 0 };
    NSMenu *menu3d = nil;
    NSMenuItem *menuitem = nil;

    if (firstTime) {
        getvalidmodes();
        xdim = settings->xdim3d;
        ydim = settings->ydim3d;
        bpp  = settings->bpp3d;
        fullscreen = settings->fullscreen;
    } else {
        fullscreen = ([fullscreenButton state] == NSOnState);
        mode3d = [[videoMode3DPUButton selectedItem] tag];
        if (mode3d >= 0) {
            xdim = validmode[mode3d].xdim;
            ydim = validmode[mode3d].ydim;
            bpp = validmode[mode3d].bpp;
        }
    }

    // Find an ideal match.
    mode3d = checkvideomode(&xdim, &ydim, bpp, fullscreen, 1);
    if (mode3d < 0) {
        for (i=0; cd[i]; ) { if (cd[i] >= bpp) i++; else break; }
        for ( ; cd[i]; i++) {
            mode3d = checkvideomode(&xdim, &ydim, cd[i], fullscreen, 1);
            if (mode3d < 0) continue;
            break;
        }
    }

    // Repopulate the lists.
    menu3d = [videoMode3DPUButton menu];
    [menu3d removeAllItems];

    for (i = 0; i < validmodecnt; i++) {
        if (validmode[i].fs != fullscreen) continue;

        menuitem = [menu3d addItemWithTitle:[NSString stringWithFormat:@"%d %C %d %d-bpp",
                                          validmode[i].xdim, 0xd7, validmode[i].ydim, validmode[i].bpp]
                                     action:nil
                              keyEquivalent:@""];
        [menuitem setTag:i];
    }

    if (mode3d >= 0) [videoMode3DPUButton selectItemWithTag:mode3d];
}

- (void)populateSoundQuality:(BOOL)firstTime
{
    int i, curidx = -1;
    NSMenu *menu = nil;
    NSMenuItem *menuitem = nil;

    if (firstTime) {
        for (i = 0; soundQualities[i].frequency > 0; i++) {
            if (soundQualities[i].frequency == settings->samplerate &&
                soundQualities[i].samplesize == settings->bitspersample &&
                soundQualities[i].channels == settings->channels) {
                curidx = i;
                break;
            }
        }
        if (curidx < 0) {
            soundQualities[i].frequency = settings->samplerate;
            soundQualities[i].samplesize = settings->bitspersample;
            soundQualities[i].channels = settings->channels;
        }
    }

    menu = [soundQualityPUButton menu];
    [menu removeAllItems];

    for (i = 0; soundQualities[i].frequency > 0; i++) {
        menuitem = [menu addItemWithTitle:[NSString stringWithFormat:@"%d kHz, %d-bit, %s",
                                          soundQualities[i].frequency / 1000,
                                          soundQualities[i].samplesize,
                                          soundQualities[i].channels == 1 ? "Mono" : "Stereo"]
                                   action:nil
                            keyEquivalent:@""];
        [menuitem setTag:i];
    }

    if (curidx >= 0) [soundQualityPUButton selectItemAtIndex:curidx];
}

- (void)populateGameList:(BOOL)firstTime
{
    NSTableView *table;
    int selindex = -1;

    table = [gameList documentView];

    if (firstTime) {
        gamelistsrc = [[GameListSource alloc] init];
        [table setDataSource:gamelistsrc];
        [table deselectAll:nil];

        selindex = [gamelistsrc indexForGrp:settings->selectedgrp];
        if (selindex >= 0) {
            [table selectRowIndexes:[NSIndexSet indexSetWithIndex:selindex] byExtendingSelection:NO];
        }

        [gamelistsrc autorelease];
    }
}

- (IBAction)fullscreenClicked:(id)sender
{
    [self populateVideoModes:NO];
}

- (IBAction)multiPlayerModeClicked:(id)sender
{
    [singlePlayerButton setState:(sender == singlePlayerButton ? NSOnState : NSOffState)];

    [joinMultiButton setState:(sender == joinMultiButton ? NSOnState : NSOffState)];
    [hostField setEnabled:(sender == joinMultiButton)];

    [hostMultiButton setState:(sender == hostMultiButton ? NSOnState : NSOffState)];
    [numPlayersField setEnabled:(sender == hostMultiButton)];
    [numPlayersStepper setEnabled:(sender == hostMultiButton)];
}

- (IBAction)cancel:(id)sender
{
    if (inmodal) {
        [NSApp stopModalWithCode:STARTWIN_CANCEL];
    }
    quitevent = quitevent || quiteventonclose;
}

- (IBAction)start:(id)sender
{
    int mode = -1;
    NSTableView *table;
    NSValue *grpvalue;
    struct grpfile *grpfile;

    mode = [[videoMode3DPUButton selectedItem] tag];
    if (mode >= 0) {
        settings->xdim3d = validmode[mode].xdim;
        settings->ydim3d = validmode[mode].ydim;
        settings->bpp3d = validmode[mode].bpp;
        settings->fullscreen = validmode[mode].fs;
    }

    settings->usemouse = [useMouseButton state] == NSOnState;
    settings->usejoy = [useJoystickButton state] == NSOnState;

    mode = [[soundQualityPUButton selectedItem] tag];
    if (mode >= 0) {
        settings->samplerate = soundQualities[mode].frequency;
        settings->bitspersample = soundQualities[mode].samplesize;
        settings->channels = soundQualities[mode].channels;
    }

    settings->numplayers = 0;
    settings->joinhost = NULL;
    if ([singlePlayerButton state] == NSOnState) {
        settings->numplayers = 1;
    } else if ([joinMultiButton state] == NSOnState) {
        NSString *host = [hostField stringValue];
        settings->numplayers = 2;
        settings->joinhost = strdup([host cStringUsingEncoding:NSUTF8StringEncoding]);
    } else if ([hostMultiButton state] == NSOnState) {
        settings->numplayers = [numPlayersField intValue];
    }

    // Get the chosen game entry.
    table = [gameList documentView];
    grpvalue = [[table dataSource] tableView:table
                   objectValueForTableColumn:[table tableColumnWithIdentifier:@"2"]
                                         row:[table selectedRow]];
    if (grpvalue) {
        settings->selectedgrp = (struct grpfile *)[grpvalue pointerValue];
    }

    settings->forcesetup = [alwaysShowButton state] == NSOnState;

    if (inmodal) {
        [NSApp stopModalWithCode:STARTWIN_RUN];
    }
}

- (void)setupConfigMode
{
    [alwaysShowButton setState: (settings->forcesetup ? NSOnState : NSOffState)];
    [alwaysShowButton setEnabled:YES];

    [videoMode3DPUButton setEnabled:YES];
    [self populateVideoModes:YES];
    [fullscreenButton setEnabled:YES];
    [fullscreenButton setState: (settings->fullscreen ? NSOnState : NSOffState)];

    [soundQualityPUButton setEnabled:YES];
    [self populateSoundQuality:YES];
    [useMouseButton setEnabled:YES];
    [useMouseButton setState: (settings->usemouse ? NSOnState : NSOffState)];
    [useJoystickButton setEnabled:YES];
    [useJoystickButton setState: (settings->usejoy ? NSOnState : NSOffState)];

    if (!settings->netoverride) {
        [singlePlayerButton setEnabled:YES];
        [singlePlayerButton setState:NSOnState];

        [hostMultiButton setEnabled:YES];
        [hostMultiButton setState:NSOffState];
        [numPlayersField setEnabled:NO];
        [numPlayersField setIntValue:2];
        [numPlayersStepper setEnabled:NO];
        [numPlayersStepper setMaxValue:MAXPLAYERS];

        [joinMultiButton setEnabled:YES];
        [joinMultiButton setState:NSOffState];
        [hostField setEnabled:NO];
    } else {
        [singlePlayerButton setEnabled:NO];
        [hostMultiButton setEnabled:NO];
        [numPlayersField setEnabled:NO];
        [numPlayersStepper setEnabled:NO];
        [joinMultiButton setEnabled:NO];
        [hostField setEnabled:NO];
    }

    [self populateGameList:YES];
    [[gameList documentView] setEnabled:YES];

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

    [[gameList documentView] setEnabled:NO];

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

- (void)setSettings:(struct startwin_settings *)theSettings
{
    settings = theSettings;
}

@end

static StartupWinController *startwin = nil;

int startwin_open(void)
{
    if (startwin != nil) return 1;

    @autoreleasepool {
        startwin = [[StartupWinController alloc] initWithWindowNibName:@"startwin.game"];
        if (startwin == nil) return -1;

        [startwin window];  // Forces the window controls on the controller to be initialised.
        [startwin setupMessagesMode:YES];
        [startwin showWindow:nil];

        return 0;
    }
}

int startwin_close(void)
{
    if (startwin == nil) return 1;

    @autoreleasepool {
        [startwin closeQuietly];
        [startwin release];
        startwin = nil;

        return 0;
    }
}

int startwin_puts(const char *s)
{
    if (!s) return -1;
    if (startwin == nil) return 1;

    @autoreleasepool {
        [startwin putsMessage:[NSString stringWithUTF8String:s]];

        return 0;
    }
}

int startwin_settitle(const char *s)
{
    if (!s) return -1;
    if (startwin == nil) return 1;

    @autoreleasepool {
        [startwin setTitle:[NSString stringWithUTF8String:s]];

        return 0;
    }
}

int startwin_idle(void *v)
{
    (void)v;
    return 0;
}

int startwin_run(struct startwin_settings *settings)
{
    int retval;

    if (startwin == nil) return 0;

    @autoreleasepool {
        [startwin setSettings:settings];

        [startwin setupConfigMode];
        retval = [startwin modalRun];
        [startwin setupMessagesMode: (settings->numplayers > 1)];

        return retval;
    }
}
