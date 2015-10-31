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

@interface GameListSource : NSObject <NSComboBoxDataSource>
{
    NSMutableArray *list;
}
- (id)init;
- (void)dealloc;
- (GrpFile*)grpAtIndex:(int)index;
- (int)findIndexForGrpname:(NSString*)grpname;
- (id)tableView:(NSTableView *)aTableView
objectValueForTableColumn:(NSTableColumn *)aTableColumn
	    row:(NSInteger)rowIndex;
- (int)numberOfRowsInTableView:(NSTableView *)aTableView;
@end

