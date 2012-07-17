/* ChatModel.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import "ChatModel.h"
#import "LinphoneManager.h"

@implementation ChatModel

@synthesize chatId;
@synthesize localContact;
@synthesize remoteContact;
@synthesize message;
@synthesize direction;
@synthesize time;
@synthesize read;


#pragma mark - Lifecycle Functions

- (id)initWithData:(sqlite3_stmt *)sqlStatement {
    self = [super init];
    if (self != nil) {
        self->chatId = [[NSNumber alloc] initWithInt: sqlite3_column_int(sqlStatement, 0)];
        self.localContact = [NSString stringWithUTF8String: (const char*) sqlite3_column_text(sqlStatement, 1)];
        self.remoteContact = [NSString stringWithUTF8String: (const char*) sqlite3_column_text(sqlStatement, 2)];
        self.direction = [NSNumber numberWithInt:sqlite3_column_int(sqlStatement, 3)];
        self.message = [NSString stringWithUTF8String: (const char*) sqlite3_column_text(sqlStatement, 4)];
        self.time = [NSDate dateWithTimeIntervalSince1970:sqlite3_column_int(sqlStatement, 5)];
        self.read = [NSNumber numberWithInt:sqlite3_column_int(sqlStatement, 6)];
    }
    return self;
}

- (void)dealloc {
    [chatId release];
    [localContact release];
    [remoteContact release];
    [message release];
    [direction release];
    [time release];
    [read release];
    
    [super dealloc];
}


#pragma mark - CRUD Functions

- (void)create {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return;
    }
    const char *sql = [[NSString stringWithFormat:@"INSERT INTO chat (localContact, remoteContact, direction, message, time, read) VALUES (\"%@\", \"%@\", %i, \"%@\", %f, %i)",
                        localContact, remoteContact, [direction intValue], message, [time timeIntervalSince1970], [read intValue]] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return;
    }
    
    if (sqlite3_step(sqlStatement) != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
    }
    
    if([self chatId] != nil) {
        [self->chatId release];
    } 
    self->chatId = [[NSNumber alloc] initWithInt:sqlite3_last_insert_rowid(database)];
    sqlite3_finalize(sqlStatement);
}

+ (ChatModel*)read:(NSNumber*)chatId {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return nil;
    }

    const char *sql = [[NSString stringWithFormat:@"SELECT id, localContact, remoteContact, direction, message, time, read FROM chat WHERE id=%i",
                        [chatId intValue]] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return nil;
    }
    
    ChatModel* line = nil;
    int err = sqlite3_step(sqlStatement);
    if (err == SQLITE_ROW) {
        line = [[ChatModel alloc] initWithData:sqlStatement];
    } else if (err != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return nil;
    }
    
    sqlite3_finalize(sqlStatement);
    return line;
}

- (void)update {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return;
    }
    
    const char *sql = [[NSString stringWithFormat:@"UPDATE chat SET localContact=\"%@\", remoteContact=\"%@\", direction=%i, message=\"%@\", time=%f, read=%i WHERE id=%i",
                        localContact, remoteContact, [direction intValue], message, [time timeIntervalSince1970], [read intValue], [chatId intValue]] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return;
    }
    
    if (sqlite3_step(sqlStatement) != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return;
    }
    
    sqlite3_finalize(sqlStatement);
}

- (void)delete {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return;
    }
    
    const char *sql = [[NSString stringWithFormat:@"DELETE FROM chat WHERE id=%i",
                        [chatId intValue]] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return;
    }    
    
    if (sqlite3_step(sqlStatement) != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return;
    }
    
    sqlite3_finalize(sqlStatement);
}


#pragma mark - 

+ (NSMutableArray *)listConversations {
    NSMutableArray *array = [NSMutableArray array];
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return array;
    }
    
    const char *sql = "SELECT id, localContact, remoteContact, direction, message, time, read FROM chat GROUP BY remoteContact ORDER BY time DESC";
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't execute the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return array;
    }
    
    int err;
    while ((err = sqlite3_step(sqlStatement)) == SQLITE_ROW) {
        ChatModel *line = [[ChatModel alloc] initWithData:sqlStatement];
        [array addObject:line];
    }
    
    if (err != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        return array;
    }
    
    sqlite3_finalize(sqlStatement);

    return array;
}

+ (NSMutableArray *)listMessages:(NSString *)contact {
    NSMutableArray *array = [NSMutableArray array];
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return array;
    }
    
    const char *sql = [[NSString stringWithFormat:@"SELECT id, localContact, remoteContact, direction, message, time, read FROM chat WHERE remoteContact=\"%@\" ORDER BY time ASC",
                        contact] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't execute the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return array;
    }
    
    int err;
    while ((err = sqlite3_step(sqlStatement)) == SQLITE_ROW) {
        ChatModel *line = [[ChatModel alloc] initWithData:sqlStatement];
        [array addObject:line];
    }
    
    if (err != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        return array;
    }
    
    sqlite3_finalize(sqlStatement);
    
    return array;
}

+ (void)removeConversation:(NSString *)contact {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return;
    }
    
    const char *sql = [[NSString stringWithFormat:@"DELETE FROM chat WHERE remoteContact=\"%@\"",
                        contact] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return;
    }    
    
    if (sqlite3_step(sqlStatement) != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return;
    }
    
    sqlite3_finalize(sqlStatement);
}

+ (int)unreadMessages {
    int count = -1;
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return count;
    }
    
    const char *sql = [[NSString stringWithFormat:@"SELECT count(*) FROM chat WHERE read=0"] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return count;
    }    
    
    if (sqlite3_step(sqlStatement) != SQLITE_ROW) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return count;
    }
    
    count = sqlite3_column_int(sqlStatement, 0);
    
    sqlite3_finalize(sqlStatement);
    return count;
}

+ (void)readConversation:(NSString *)contact {
    sqlite3* database = [[LinphoneManager instance] database];
    if(database == NULL) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Database not ready"];
        return;
    }
    
    const char *sql = [[NSString stringWithFormat:@"UPDATE chat SET read=1 WHERE remoteContact=\"%@\"",
                        contact] UTF8String];
    sqlite3_stmt *sqlStatement;
    if (sqlite3_prepare(database, sql, -1, &sqlStatement, NULL) != SQLITE_OK) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Can't prepare the query: %s (%s)", sql, sqlite3_errmsg(database)];
        return;
    }    
    
    if (sqlite3_step(sqlStatement) != SQLITE_DONE) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Error during execution of query: %s (%s)", sql, sqlite3_errmsg(database)];
        sqlite3_finalize(sqlStatement);
        return;
    }
    
    sqlite3_finalize(sqlStatement);
}

@end
