//-------------------------------------------------------------------------
/*
 Copyright (C) 2013-2021 Jonathon Fowler <jf@jonof.id.au>

 This file is part of JFDuke3D

 Duke Nukem 3D is free software; you can redistribute it and/or
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
#include "startwin.h"

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

// Callbacks to glue the C in grpscan.c with the Objective-C here.
static void importmeta_progress(void *data, const char *path);
static int importmeta_cancelled(void *data);

@interface StartupWinController : NSWindowController <NSWindowDelegate>
{
    BOOL quiteventonclose;
    GameListSource *gamelistsrc;
    NSThread *importthread;
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

    IBOutlet NSButton *chooseImportButton;
    IBOutlet NSButton *importInfoButton;

    IBOutlet NSButton *cancelButton;
    IBOutlet NSButton *startButton;

    IBOutlet NSWindow *importStatusWindow;
    IBOutlet NSTextField *importStatusText;
    IBOutlet NSButton *importStatusCancel;
}

- (int)modalRun;
- (void)closeQuietly;
- (void)populateVideoModes:(BOOL)firstTime;
- (void)populateSoundQuality:(BOOL)firstTime;

- (IBAction)fullscreenClicked:(id)sender;

- (IBAction)multiPlayerModeClicked:(id)sender;

- (IBAction)chooseImportClicked:(id)sender;
- (IBAction)importInfoClicked:(id)sender;
- (IBAction)importStatusCancelClicked:(id)sender;
- (void)updateImportStatusText:(NSString *)text;
- (void)doImport:(NSString *)path;
- (void)doneImport:(NSNumber *)result;
- (BOOL)isImportCancelled;

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
        mode3d = (int)[[videoMode3DPUButton selectedItem] tag];
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
    } else {
        [table reloadData];
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

- (IBAction)chooseImportClicked:(id)sender
{
    @autoreleasepool {
        NSArray *filetypes = [[NSArray alloc] initWithObjects:@"grp", @"app", nil];
        NSOpenPanel *panel = [NSOpenPanel openPanel];

        [panel setTitle:@"Import game data"];
        [panel setPrompt:@"Import"];
        [panel setMessage:@"Select a .grp file, an .app bundle, or choose a folder to search."];
        [panel setAllowedFileTypes:filetypes];
        [panel setCanChooseFiles:TRUE];
        [panel setCanChooseDirectories:TRUE];
        [panel setShowsHiddenFiles:TRUE];
        [panel beginSheetModalForWindow:[self window]
                      completionHandler:^void (NSModalResponse resp) {
            if (resp == NSFileHandlingPanelOKButton) {
                NSURL *file = [panel URL];
                if ([file isFileURL]) {
                    [self doImport:[file path]];
                }
            }
        }];
    }
}

- (IBAction)importStatusCancelClicked:(id)sender
{
    [importthread cancel];
}

- (void)updateImportStatusText:(NSString *)text
{
    [importStatusText setStringValue:text];
}

- (void)doImport:(NSString *)path
{
    if ([importthread isExecuting]) {
        NSLog(@"import thread is already executing");
        return;
    }

    // Put up the status sheet which becomes modal.
    [NSApp beginSheet:importStatusWindow
       modalForWindow:[self window]
        modalDelegate:self
       didEndSelector:nil
          contextInfo:NULL];

    // Spawn a thread to do the scan.
    importthread = [[NSThread alloc] initWithBlock:^void(void) {
        struct importgroupsmeta meta = {
            (void *)self,
            importmeta_progress,
            importmeta_cancelled
        };
        int result = ImportGroupsFromPath([path UTF8String], &meta);
        [self performSelectorOnMainThread:@selector(doneImport:)
                               withObject:[NSNumber numberWithInt:result]
                            waitUntilDone:FALSE];
    }];
    [importthread start];
}

// Finish up after the import thread returns.
- (void)doneImport:(NSNumber *)result
{
    if ([result intValue] > 0) {
        [self populateGameList:NO];
    }
    [importStatusWindow orderOut:nil];
    [NSApp endSheet:importStatusWindow returnCode:1];
}

// Report on whether the import thread has been been cancelled early.
- (BOOL)isImportCancelled
{
    return [importthread isCancelled];
}

- (IBAction)importInfoClicked:(id)sender
{
    @autoreleasepool {
        NSAlert *alert = [[NSAlert alloc] init];
        NSURL *sharewareurl = [NSURL URLWithString:@"https://www.jonof.id.au/files/jfduke3d/dn3dsw13.zip"];

        [alert setAlertStyle:NSAlertStyleInformational];
        [alert setMessageText:@"JFDuke3D can scan locations of your choosing for Duke Nukem 3D game data"];
        [alert setInformativeText:@"Click the 'Choose a location...' button, then locate "
            @"a .grp file, an .app bundle, or a folder to scan.\n\n"
            @"Common locations to check include:\n"
            @" • CD/DVD drives\n"
            @" • GOG-managed .app bundles\n"
            @" • Steam library folders\n\n"
            @"To play the Shareware version, download the shareware data (dn3dsw13.zip), unzip the file, "
                @"then select the DUKE3D.GRP file with the 'Choose a location...' option."];
        [alert addButtonWithTitle:@"OK"];
        [alert addButtonWithTitle:@"Download Shareware"];
        switch ([alert runModal]) {
            case NSAlertFirstButtonReturn:
                break;
            case NSAlertSecondButtonReturn:
                LSOpenCFURLRef((CFURLRef)sharewareurl, nil);
                break;
        }
    }
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

    mode = (int)[[videoMode3DPUButton selectedItem] tag];
    if (mode >= 0) {
        settings->xdim3d = validmode[mode].xdim;
        settings->ydim3d = validmode[mode].ydim;
        settings->bpp3d = validmode[mode].bpp;
        settings->fullscreen = validmode[mode].fs;
    }

    settings->usemouse = [useMouseButton state] == NSOnState;
    settings->usejoy = [useJoystickButton state] == NSOnState;

    mode = (int)[[soundQualityPUButton selectedItem] tag];
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
    [chooseImportButton setEnabled:YES];
    [importInfoButton setEnabled:YES];

    [cancelButton setEnabled:YES];
    [startButton setEnabled:YES];

    if (!settings->selectedgrp) {
        [tabView selectTabViewItem:tabGame];
    } else {
        [tabView selectTabViewItem:tabConfig];
    }
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
    [chooseImportButton setEnabled:NO];
    [importInfoButton setEnabled:NO];

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
        NSString *str = [NSString stringWithUTF8String:s];
        if ([NSThread isMainThread]) {
            [startwin putsMessage:str];
        } else {
            [startwin performSelectorOnMainThread:@selector(putsMessage:)
                                       withObject:str
                                    waitUntilDone:TRUE];
        }

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

// Callback for the C-universe ImportGroupsFrom*() to notify the UI in Obj-C land.
static void importmeta_progress(void *data, const char *path)
{
    StartupWinController *control = (StartupWinController *)data;

    @autoreleasepool {
        [control performSelectorOnMainThread:@selector(updateImportStatusText:)
                                  withObject:[NSString stringWithUTF8String:path]
                               waitUntilDone:FALSE];
    }
}

// Callback for the C-universe ImportGroupsFrom*() to discover they've been cancelled by the UI.
static int importmeta_cancelled(void *data)
{
    StartupWinController *control = (StartupWinController *)data;
    return [control isImportCancelled];
}
