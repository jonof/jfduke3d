//-------------------------------------------------------------------------
/*
 Copyright (C) 2013 Jonathon Fowler <jf@jonof.id.au>

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

#include "grpscan.h"
#import "GameListSource.h"

@implementation GameListSource
- (id)tableView:(NSTableView *)aTableView
        objectValueForTableColumn:(NSTableColumn *)aTableColumn
            row:(NSInteger)rowIndex
{
    int row;
    struct grpfile const *fg = GroupsFound();

    for (row = 0; row < rowIndex && fg; fg = fg->next) {
        if (fg->ref) row++;
    }
    if (!fg) {
        return nil;
    }
    switch ([[aTableColumn identifier] intValue]) {
        case 0:	// name column
            if (fg->ref) {
                return [NSString stringWithUTF8String: fg->ref->name];
            } else {
                return @"Unknown game";
            }
        case 1:	// grp column
            return [NSString stringWithUTF8String: fg->name];
        case 2: // hidden column pointing to the grpfile entry.
            return [NSValue valueWithPointer: fg];
        default:
            return nil;
    }
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    int count = 0;
    struct grpfile const *fg = GroupsFound();

    for (count = 0; fg; fg = fg->next) {
        if (fg->ref) count++;
    }
    return count;
}

- (int)indexForGrp:(struct grpfile const *)grpFile
{
    int index;
    struct grpfile const *fg = GroupsFound();

    for (index = 0; fg; fg = fg->next) {
        if (fg == grpFile) return index;
        if (fg->ref) index++;
    }
    return -1;
}
@end

