// Objective-C programmers shall recoil in fear at this mess

#import <Cocoa/Cocoa.h>

#define GetTime xGetTime
#include "duke3d.h"
#undef GetTime
#include "build.h"
#include "compat.h"
#include "baselayer.h"
#include "grpscan.h"

static struct {
	int fullscreen;
	int xdim3d, ydim3d, bpp3d;
	int forcesetup;
	char selectedgrp[BMAX_PATH+1];
	int game;
} settings;

@interface GrpFile : NSObject
{
	NSString *name;
	struct grpfile *fg;
}
- (id)initWithGrpfile:(struct grpfile *)grpfile andName:(NSString*)aName;
- (void)dealloc;
- (NSString *)name;
- (NSString *)grpname;
- (struct grpfile *)entryptr;
@end

@implementation GrpFile
- (id)initWithGrpfile:(struct grpfile *)grpfile andName:(NSString*)aName
{
	self = [super init];
	if (self) {
		fg = grpfile;
		name = aName;
		[aName retain];
	}
	return self;
}
- (void)dealloc
{
	[name release];
	[super dealloc];
}
- (NSString *)name
{
	return name;
}
- (NSString *)grpname
{
	return [NSString stringWithCString:(fg->name)];
}
- (struct grpfile *)entryptr
{
	return fg;
}
@end

@interface GameListSource : NSObject
{
	NSMutableArray *list;
}
- (id)init;
- (void)dealloc;
- (GrpFile*)grpAtIndex:(int)index;
- (int)findIndexForGrpname:(NSString*)grpname;
- (id)tableView:(NSTableView *)aTableView
    objectValueForTableColumn:(NSTableColumn *)aTableColumn
	    row:(int)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
@end

@implementation GameListSource
- (id)init
{
	self = [super init];
	if (self) {
		struct grpfile *p;
		int i;
		
		list = [[NSMutableArray alloc] init];
		
		for (p = foundgrps; p; p=p->next) {
			for (i=0; i<numgrpfiles; i++) if (p->crcval == grpfiles[i].crcval) break;
			if (i == numgrpfiles) continue;
			[list addObject:[[GrpFile alloc] initWithGrpfile:p andName:[NSString stringWithCString:grpfiles[i].name]]];
		}
	}
	
	return self;
}

- (void)dealloc
{
	[list release];
	[super dealloc];
}

- (GrpFile*)grpAtIndex:(int)index
{
	return [list objectAtIndex:index];
}

- (int)findIndexForGrpname:(NSString*)grpname
{
	int i;
	for (i=0; i<[list count]; i++) {
		if ([[[list objectAtIndex:i] grpname] isEqual:grpname]) return i;
	}
	return -1;
}

- (id)tableView:(NSTableView *)aTableView
    objectValueForTableColumn:(NSTableColumn *)aTableColumn
	    row:(int)rowIndex
{
	NSParameterAssert(rowIndex >= 0 && rowIndex < [list count]);
	switch ([[aTableColumn identifier] intValue]) {
		case 0:	// name column
			return [[list objectAtIndex:rowIndex] name];
		case 1:	// grp column
			return [[list objectAtIndex:rowIndex] grpname];
		default: return nil;
	}
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	return [list count];
}
@end

@interface StartupWinController : NSWindowController
{
	NSMutableArray *modeslist3d;
	GameListSource *gamelistsrc;

	IBOutlet NSButton *alwaysShowButton;
	IBOutlet NSButton *fullscreenButton;
	IBOutlet NSTextView *messagesView;
	IBOutlet NSTabView *tabView;
	IBOutlet NSPopUpButton *videoMode3DPUButton;
	IBOutlet NSScrollView *gameList;
	
	IBOutlet NSButton *cancelButton;
	IBOutlet NSButton *startButton;
}

- (void)dealloc;
- (void)populateVideoModes:(BOOL)firstTime;

- (IBAction)alwaysShowClicked:(id)sender;
- (IBAction)fullscreenClicked:(id)sender;

- (IBAction)cancel:(id)sender;
- (IBAction)start:(id)sender;

- (void)setupRunMode;
- (void)setupMessagesMode;
- (void)putsMessage:(NSString *)str;
- (void)setTitle:(NSString *)str;
@end

@implementation StartupWinController

- (void)dealloc
{
	[gamelistsrc release];
	[modeslist3d release];
	[super dealloc];
}

- (void)populateVideoModes:(BOOL)firstTime
{
	int i, mode3d, fullscreen = ([fullscreenButton state] == NSOnState);
	int idx3d = -1;
	int xdim, ydim, bpp;
	
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

- (IBAction)alwaysShowClicked:(id)sender
{
}

- (IBAction)fullscreenClicked:(id)sender
{
	[self populateVideoModes:NO];
}

- (IBAction)cancel:(id)sender
{
	[NSApp abortModal];
}

- (IBAction)start:(id)sender
{
	int mode = [[modeslist3d objectAtIndex:[videoMode3DPUButton indexOfSelectedItem]] intValue];
	if (mode >= 0) {
		settings.xdim3d = validmode[mode].xdim;
		settings.ydim3d = validmode[mode].ydim;
		settings.bpp3d = validmode[mode].bpp;
		settings.fullscreen = validmode[mode].fs;
	}
	
	int row = [[gameList documentView] selectedRow];
	if (row >= 0) {
		struct grpfile *p = [[gamelistsrc grpAtIndex:row] entryptr];
		if (p) {
			strcpy(settings.selectedgrp, p->name);
			settings.game = p->game;
		}
	}
		
	settings.forcesetup = [alwaysShowButton state] == NSOnState;

	[NSApp stopModal];
}

- (void)setupRunMode
{
	getvalidmodes();

	[fullscreenButton setState: (settings.fullscreen ? NSOnState : NSOffState)];
	[alwaysShowButton setState: (settings.forcesetup ? NSOnState : NSOffState)];
	[self populateVideoModes:YES];

	// enable all the controls on the Configuration page
	NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
	NSControl *control;
	while (control = [enumerator nextObject]) [control setEnabled:true];
	
	gamelistsrc = [[GameListSource alloc] init];
	[[gameList documentView] setDataSource:gamelistsrc];
	[[gameList documentView] deselectAll:nil];

	int row = [gamelistsrc findIndexForGrpname:[NSString stringWithCString:settings.selectedgrp]];
	if (row >= 0) {
		[[gameList documentView] scrollRowToVisible:row];
#if defined(MAC_OS_X_VERSION_10_3) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3)
		[[gameList documentView] selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
#else
		[[gameList documentView] selectRow:row byExtendingSelection:NO];
#endif
	}
	
	[cancelButton setEnabled:true];
	[startButton setEnabled:true];

	[tabView selectTabViewItemAtIndex:0];
	[NSCursor unhide];	// Why should I need to do this?
}

- (void)setupMessagesMode
{
	[tabView selectTabViewItemAtIndex:2];

	// disable all the controls on the Configuration page except "always show", so the
	// user can enable it if they want to while waiting for something else to happen
	NSEnumerator *enumerator = [[[[tabView tabViewItemAtIndex:0] view] subviews] objectEnumerator];
	NSControl *control;
	while (control = [enumerator nextObject]) {
		if (control == alwaysShowButton) continue;
		[control setEnabled:false];
	}

	[cancelButton setEnabled:false];
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

	[startwin setupMessagesMode];
	[startwin showWindow:nil];

	return 0;
}

int startwin_close(void)
{
	if (startwin == nil) return 1;

	[startwin close];
	[startwin release];
	startwin = nil;

	return 0;
}

int startwin_puts(const char *s)
{
	NSString *ns;

	if (!s) return -1;
	if (startwin == nil) return 1;

	ns = [[NSString alloc] initWithCString:s];
	[startwin putsMessage:ns];
	[ns release];

	return 0;
}

int startwin_settitle(const char *s)
{
	NSString *ns;
	
	if (!s) return -1;
	if (startwin == nil) return 1;
	
	ns = [[NSString alloc] initWithCString:s];
	[startwin setTitle:ns];
	[ns release];

	return 0;
}

int startwin_idle(void *v)
{
	if (startwin) [[startwin window] displayIfNeeded];
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
	settings.forcesetup = ForceSetup;
	settings.game = gametype;
	strncpy(settings.selectedgrp, duke3dgrp, BMAX_PATH);
	
	[startwin setupRunMode];
	
	switch ([NSApp runModalForWindow:[startwin window]]) {
		case NSRunStoppedResponse: retval = 1; break;
		case NSRunAbortedResponse: retval = 0; break;
		default: retval = -1;
	}
	
	[startwin setupMessagesMode];

	if (retval) {
		ScreenMode = settings.fullscreen;
		ScreenWidth = settings.xdim3d;
		ScreenHeight = settings.ydim3d;
		ScreenBPP = settings.bpp3d;
		ForceSetup = settings.forcesetup;
		duke3dgrp = settings.selectedgrp;
		gametype = settings.game;
	}
	
	return retval;
}
