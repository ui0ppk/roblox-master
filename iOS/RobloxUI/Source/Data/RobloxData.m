
//
//  RobloxData.m
//  NativeShell-iOS
//
//  Created by alichtin on 4/25/14.
//  Copyright (c) 2014 Roblox. All rights reserved.
//

#import "RobloxData.h"

#import "ABTestManager.h"
#import "RBXFunctions.h"
#import "RobloxInfo.h"
#import "NSDictionary+Parsing.h"
#import "SessionReporter.h"

#import "RobloxXSRFRequest.h"
#import "AFHTTPRequestOperation.h"

static dispatch_queue_t _completionQueue;

// GameData
@implementation RBXGameData
+ (RBXGameData*) parseGameFromJSON:(NSDictionary*)jsonGame
{
    RBXGameData* game = [[RBXGameData alloc] init];
    game.players = [jsonGame stringForKey:@"PlayerCount"];
    game.title = [jsonGame stringForKey:@"Name"];
    game.creatorName = [jsonGame stringForKey:@"CreatorName"];
    game.creatorID = [jsonGame numberForKey:@"CreatorID"];
    game.placeID = [jsonGame stringForKey:@"PlaceID"];
    game.visited = [jsonGame unsignedValueForKey:@"Plays"];
    game.userFavorited = NO;
    game.userVote = RBXUserVoteNoVote;
    game.userOwns = [jsonGame boolValueForKey:@"IsOwned"];
    game.productID = [jsonGame unsignedValueForKey:@"ProductID"];
    game.price = [jsonGame unsignedValueForKey:@"Price"];
    
    game.thumbnailURL = [jsonGame stringForKey:@"ThumbnailUrl"];
    game.thumbnailIsFinal = [jsonGame boolValueForKey:@"ThumbnailIsFinal"];
    
    game.universeID = [jsonGame numberForKey:@"UniverseID" withDefault:nil];
    
    game.upVotes = [jsonGame unsignedValueForKey:@"TotalUpVotes"];
    game.downVotes = [jsonGame unsignedValueForKey:@"TotalDownVotes"];
    
    return game;
}
@end

// GameAttribute
@implementation RBXGameAttribute

@end

// GameSort
@implementation RBXGameSort
+(RBXGameSort*) GameSortWithJSON:(NSDictionary*)json
{
    RBXGameSort* sort = [[RBXGameSort alloc] init];
    sort.sortID = [json numberForKey:@"Id"];
    sort.title = [json stringForKey:@"Name"];
    return sort;
}
+(RBXGameSort*) RecentlyPlayedGamesSort
{
    RBXGameSort* sort = [[RBXGameSort alloc] init];
    sort.sortID = [NSNumber numberWithInt:RBXGameSortTypeMyRecent]; // old 6
    sort.title = NSLocalizedString(@"RecentlyWord", nil);
    return sort;
}
+(RBXGameSort*) MyGamesSort
{
    RBXGameSort* sort = [[RBXGameSort alloc] init];
    sort.sortID = [NSNumber numberWithInt:RBXGameSortTypeMyRecent];
    sort.title = NSLocalizedString(@"MyWord", nil);
    return sort;
}
+(RBXGameSort*) FavoriteGamesSort
{
    RBXGameSort* sort = [[RBXGameSort alloc] init];
    sort.sortID = [NSNumber numberWithInt:RBXGameSortTypeMyFavorite];
    sort.title = NSLocalizedString(@"FavoritedWord", nil);
    return sort;
}
+(RBXGameSort*) DefaultSort
{
    RBXGameSort* sort = [[RBXGameSort alloc] init];
    sort.sortID = [NSNumber numberWithInt:RBXGameSortTypeDefault];
    sort.title = NSLocalizedString(@"DefaultWord", nil);
    return sort;
}

- (BOOL)isEqual:(id)object
{
    return [super isEqual:object]
        && [((RBXGameSort*)object).sortID isEqualToNumber:_sortID];
}

@end

// GameGenre
@implementation RBXGameGenre

+(RBXGameGenre*) gameGenreWithId:(NSNumber *)genreID name:(NSString *)genreName {
    RBXGameGenre *genre = [[RBXGameGenre alloc] init];
    genre.genreID = genreID;
    genre.title = genreName;
    
    return genre;
}

+(NSArray*) allGenres {
    
    NSMutableArray *genresToUse = nil;
    
    NSInteger minGenres = 5;
    
    // 2015.0902 ajain - See if there are remote genres in the iOS App Settings; if so then
    // use the remote genres
    NSString *remoteGenreString = [RobloxInfo gameGenres];
    
    if ([remoteGenreString isKindOfClass:[NSString class]] && ![RBXFunctions isEmptyString:remoteGenreString]) {
        NSArray *remoteGenres = [remoteGenreString componentsSeparatedByString:@"|"];
        genresToUse = [NSMutableArray arrayWithCapacity:remoteGenres.count];
        
        for (NSString *remoteGenre in remoteGenres) {
            NSArray *genreParts = [remoteGenre componentsSeparatedByString:@"-"];
            
            NSString *genreName = genreParts.firstObject;
            if ([genreName rangeOfString:@"+"].location != NSNotFound) {
                NSArray *nameParts = [genreName componentsSeparatedByString:@"+"];
                NSMutableString *newGenreName = [NSMutableString string];
                for (NSInteger i = 0; i < nameParts.count; i++) {
                    NSString *namePart = nameParts[i];
                    [newGenreName appendString:namePart];
                    if (i < nameParts.count - 1) {
                        [newGenreName appendString:@" "];
                    }
                }
                
                genreName = newGenreName;
            }
            
            NSNumber *genreID = genreParts.lastObject;
            
            RBXGameGenre *newGenre = [RBXGameGenre gameGenreWithId:genreID name:genreName];
            [genresToUse addObject:newGenre];
        }
        
        // Sort the remote list of genres alphabetically
        NSSortDescriptor *sort = [NSSortDescriptor sortDescriptorWithKey:@"title" ascending:YES];
        NSMutableArray *sortedGenres = [[genresToUse sortedArrayUsingDescriptors:@[sort]] mutableCopy];
        
        // Find the Genre-All and put it at index position 0
        for (RBXGameGenre *genre in sortedGenres) {
            if ([genre.title.lowercaseString isEqualToString:@"All".lowercaseString]) {
                // Remove the Genre-All from wherever it is in the array
                [sortedGenres removeObject:genre];
                
                // And insert the Genre-All as the first position in the list
                [sortedGenres insertObject:genre atIndex:0];
                
                break;
            }
        }
        
        genresToUse = sortedGenres;
    }
    
    // 2015.0902 ajain - Try to do some cursory level of validation of the remote genres
    BOOL remoteGenresInvalid = ([RBXFunctions isEmpty:genresToUse] || genresToUse.count < minGenres);
    
    // 2015.0902 ajain - If we can't use the remote genres then we can use the local default genres
    if (remoteGenresInvalid) {
        genresToUse = [@[
                         [self gameGenreWithId:@(1) name:@"All"],
                         [self gameGenreWithId:@(13) name:@"Adventure"],
                         [self gameGenreWithId:@(19) name:@"Building"],
                         [self gameGenreWithId:@(15) name:@"Comedy"],
                         [self gameGenreWithId:@(10) name:@"Fighting"],
                         [self gameGenreWithId:@(20) name:@"FPS"],
                         [self gameGenreWithId:@(11) name:@"Horror"],
                         [self gameGenreWithId:@(8) name:@"Medieval"],
                         [self gameGenreWithId:@(17) name:@"Military"],
                         [self gameGenreWithId:@(12) name:@"Naval"],
                         [self gameGenreWithId:@(21) name:@"RPG"],
                         [self gameGenreWithId:@(9) name:@"SciFi"],
                         [self gameGenreWithId:@(14) name:@"Sports"],
                         [self gameGenreWithId:@(7) name:@"Town and City"],
                         [self gameGenreWithId:@(16) name:@"Western"]
                         ] mutableCopy];
    }
    
    return genresToUse;
}

@end

// LeaderboardEntry
@implementation RBXLeaderboardEntry
+ (RBXLeaderboardEntry*) parseLeaderboardEntry:(NSDictionary*)jsonEntry
{
    RBXLeaderboardEntry* entry = [[RBXLeaderboardEntry alloc] init];
    
    NSInteger rank = [jsonEntry intValueForKey:@"Rank" withDefault:-1];
    if(rank != -1)
        entry.displayRank = [jsonEntry stringForKey:@"DisplayRank"];
    else
        entry.displayRank = @"";
    
    entry.name = [jsonEntry stringForKey:@"Name"];
    entry.rank = rank;
    entry.userID = [jsonEntry unsignedValueForKey:@"UserId"];
    entry.targetID = [jsonEntry unsignedValueForKey:@"TargetId"];
    entry.clanName = [jsonEntry stringForKey:@"ClanName"];
    entry.points = [jsonEntry unsignedValueForKey:@"Points"];
    entry.displayPoints = [jsonEntry stringForKey:@"DisplayPoints"];
    entry.clanEmblemID = [jsonEntry unsignedValueForKey:@"ClanEmblemID"];
    entry.userAvatarURL = [jsonEntry stringForKey:@"UserImageUri"];
    entry.userAvatarIsFinal = [jsonEntry boolValueForKey:@"UserImageFinal"];
    entry.clanAvatarURL = [jsonEntry stringForKey:@"ClanImageUri"];
    entry.clanAvatarIsFinal = [jsonEntry boolValueForKey:@"ClanImageFinal"];
    
    return entry;
}
@end

// User profile info
@implementation RBXUserProfileInfo
+(RBXUserProfileInfo*) UserProfileInfoWithJSON:(NSDictionary*)json
{
    RBXUserProfileInfo* profile = [[RBXUserProfileInfo alloc] init];
    profile.userID = [json numberForKey:@"UserId"];
    profile.username = [json stringForKey:@"Username"];
    profile.avatarURL = [json stringForKey:@"AvatarUri"];
    profile.avatarIsFinal = [json boolValueForKey:@"AvatarFinal"];
    profile.BC = [json boolValueForKey:@"BC"];
    profile.TBC = [json boolValueForKey:@"TBC"];
    profile.OBC = [json boolValueForKey:@"OBC"];
    profile.robux = [json unsignedValueForKey:@"Robux" withDefault:-1];
    profile.tickets = [json unsignedValueForKey:@"Tickets" withDefault:-1];
    profile.friendshipStatus = [json unsignedValueForKey:@"FriendshipStatus"];
    profile.isFollowing = [json boolValueForKey:@"IsFollowingDisplayedUser" withDefault:NO];
    
    return profile;
}
@end
@implementation RBXUserAccount
//THIS WILL NEED TO BE UPDATED AT SOME POINT
//{"UserId":68465808,"Name":"iMightBeLying","UserEmail":"kmulherin@roblox.com","IsEmailVerified":true,"AgeBracket":2,"UserAbove13":true}
+(RBXUserAccount*) UserAccountWithJSON:(NSDictionary*)json
{
    RBXUserAccount* accountInfo = [[RBXUserAccount alloc] init];
    accountInfo.userAbove13 = [json boolValueForKey:@"UserAbove13" withDefault:YES];
    accountInfo.userId      = [json numberForKey:@"UserId" withDefault:[NSNumber numberWithInt:1]];
    accountInfo.email       = [json stringForKey:@"UserEmail" withDefault:nil];
    accountInfo.isEmailVerfied = [json boolValueForKey:@"IsEmailVerified" withDefault:NO];
    accountInfo.userName    = [json stringForKey:@"Name" withDefault:nil];
    //accountInfo.ageBracket  = [json numberForKey:@"AgeBracket" withDefault:[NSNumber numberWithInt:2]];
    return accountInfo;
}
@end
@implementation RBXUserPresence
+(RBXUserPresence*) UserPresenceWithJSON:(NSDictionary *)json
{
    RBXUserPresence* userPresence = [[RBXUserPresence alloc] init];
    userPresence.gameID = [json stringForKey:@"GameId" withDefault:nil];
    userPresence.isOnline = [json boolValueForKey:@"IsOnline"];
    userPresence.lastOnlineTime = [json stringForKey:@"LastOnline"];
    userPresence.lastOnlineLocation = [json stringForKey:@"LastLocation"];
    userPresence.locationType = [json unsignedValueForKey:@"LocationType" withDefault:nil];
    userPresence.placeID = [json unsignedValueForKey:@"PlaceId" withDefault:nil];
    
    return userPresence;
}
@end
@implementation RBXUserAccountNotifications
+(RBXUserAccountNotifications*) UserAccountNotificationsWithJSON:(NSDictionary *)json
{
    RBXUserAccountNotifications* notifications = [[RBXUserAccountNotifications alloc] init];
    notifications.emailNotificationEnabled =     [json boolValueForKey:@"EmailNotificationEnabled"       withDefault:NO];
    notifications.passwordNotificationEnabled =  [json boolValueForKey:@"PasswordNotificationEnabled"    withDefault:NO];
    notifications.total =                        [json intValueForKey: @"Count"                          withDefault:0];
    
    return notifications;
}
@end

@implementation RBXUserSearchInfo

+(RBXUserSearchInfo*) UserSearchInfoWithJSON:(NSDictionary *)json
{
    RBXUserSearchInfo* user = [[RBXUserSearchInfo alloc] init];
    user.avatarURL = [json stringForKey:@"AvatarUri"];
    user.avatarIsFinal = [json boolValueForKey:@"AvatarFinal" withDefault:NO];
    user.userId = [json numberForKey:@"UserId" withDefault:[NSNumber numberWithInt:-1]];
    user.userName = [json stringForKey:@"Name" withDefault:@""];
    user.blurb = [json stringForKey:@"Blurb" withDefault:@""];
    user.previousNames = [json stringForKey:@"PreviousUserNamesCsv" withDefault:@""];
    user.isOnline = [json boolValueForKey:@"IsOnline" withDefault:NO];
    user.lastLocation = [json stringForKey:@"LastLocation" withDefault:@""];
    user.profilePageURL = [json stringForKey:@"UserProfilePageUrl" withDefault:@"/User.aspx?id=-1"];
    user.lastSeenDate = [json stringForKey:@"LastSeenDate" withDefault:@""];
    
    return user;
}
@end


// Friend info
@implementation RBXFriendInfo
+(RBXFriendInfo*) FriendInfoWithJSON:(NSDictionary*)json
{
    RBXFriendInfo* friend = [[RBXFriendInfo alloc] init];
    friend.userID = [json numberForKey:@"UserId"];
    friend.username = [json stringForKey:@"Username"];
    friend.avatarURL = [json stringForKey:@"AvatarUri"];
    friend.avatarIsFinal = [json boolValueForKey:@"AvatarFinal"];
    friend.friendshipStatus = [json boolValueForKey:@"IsBestFriend"] == YES ? RBXFriendshipStatusBestFriends : RBXFriendshipStatusFriends;
    friend.invitationID = [json unsignedValueForKey:@"InvitationId"];
    
    NSDictionary* onlineStatus = [json dictionaryForKey:@"OnlineStatus" withDefault:nil];
    if (onlineStatus)
        friend.isOnline = [[onlineStatus stringForKey:@"ImageUrl"] isEqualToString:@"~/images/online.png"];
    else
        friend.isOnline = false;

    return friend;
}
+(RBXFriendInfo*) FriendInfoFromFollower:(RBXFollower*)follower
{
    RBXFriendInfo* friend = [[RBXFriendInfo alloc] init];
    friend.userID = follower.userId;
    friend.username = follower.username;
    friend.avatarURL = follower.avatarURL;
    friend.avatarIsFinal = follower.avatarIsFinal;
    friend.friendshipStatus = RBXFriendshipStatusNonFriends;
    friend.invitationID = nil;
    friend.isOnline = follower.isOnline;
    
    return friend;
}
@end

// Badge info
@implementation RBXBadgeInfo
+(RBXBadgeInfo*) parseGameBadgeFromJSON:(NSDictionary*)json
{
    RBXBadgeInfo* badge = [[RBXBadgeInfo alloc] init];
    badge.badgeAssetId = [NSString stringWithFormat:@"%lu", (unsigned long)[json unsignedValueForKey:@"BadgeAssetId"]];
    badge.badgeCreatorId = [NSString stringWithFormat:@"%lu", (unsigned long)[json unsignedValueForKey:@"CreatorId"]];
    
    badge.imageURL = [json stringForKey:@"ImageUrl" withDefault:nil];
    if(badge.imageURL == nil)
        badge.imageURL = [json stringForKey:@"ImageUri" withDefault:nil];
    
    badge.isImageURLFinal = [json boolValueForKey:@"IsImageUrlFinal"];
    badge.name = [json stringForKey:@"Name"];
    badge.badgeDescription = [json stringForKey:@"Description"];
    badge.isOwned = [json boolValueForKey:@"IsOwned"];
    badge.badgeRarity = [json doubleValueForKey:@"Rarity"];
    badge.badgeTotalAwardedEver = [json stringForKey:@"TotalAwarded"];
    badge.badgeTotalAwardedYesterday = [json stringForKey:@"TotalAwardedYesterday"];
    badge.badgeCreatedDate = [json stringForKey:@"Created"];
    badge.badgeUpdatedDate = [json stringForKey:@"Updated"];
    badge.badgeRarityName = [json stringForKey:@"RarityName"];
    
    return badge;
}

@end

// Game Gear
@implementation RBXGameGear

@end

// Game Pass
@implementation RBXGamePass

@end

// Message info
@implementation RBXMessageInfo
+(RBXMessageInfo*) MessageWithJSON:(NSDictionary*)json
{
    NSDictionary* recipient = [json dictionaryForKey:@"Recipient"];
    //NSDictionary* recipientThumbnail = [json dictionaryForKey:@"RecipientThumbnail"];
    NSDictionary* sender = [json dictionaryForKey:@"Sender"];
    //NSDictionary* senderThumbnail = [json dictionaryForKey:@"SenderThumbnail"];
    
    
    RBXMessageInfo* message = [[RBXMessageInfo alloc] init];
    message.date = [json stringForKey:@"Created"];
    message.messageID = [json unsignedValueForKey:@"Id"];
    message.isRead = [json unsignedValueForKey:@"IsRead" withDefault:1] != 0;
    //message.isReportAbuseDisplayed = [json unsignedValueForKey:@"IsReportAbuseDisplayed" withDefault:1] != 0;
    //message.isSystemMessage = [json unsignedValueForKey:@"IsSystemMessage" withDefault:1] != 0;
    message.recipientUserID = [recipient numberForKey:@"UserId"];
    message.recipientUsername = [recipient stringForKey:@"UserName"];
    message.senderUserID = [sender numberForKey:@"UserId"];
    message.senderUsername = [sender stringForKey:@"UserName"];
    
    
    message.subject = [json stringForKey:@"Subject"];
    message.body = [[json stringForKey:@"Body"] stringByReplacingOccurrencesOfString:@"\n" withString:@" <br/>"];
    
    
    return message;
}
+(NSMutableArray*) ArrayFromJSON:(NSDictionary*)json
{
    NSMutableArray* messages = [NSMutableArray array];
    
    NSArray* jsonMessages = [json arrayForKey:@"Collection"]; //[json arrayForKey:@"Messages"];
    for(NSDictionary* jsonMessage in jsonMessages)
    {
        [messages addObject:[RBXMessageInfo MessageWithJSON:jsonMessage]];
    }
    
    return messages;
}
@end

// Followers and Following Info
@implementation RBXFollower
+(RBXFollower*) FollowerWithJSON:(NSDictionary*)json
{
    RBXFollower* aFollower = [[RBXFollower alloc] init];
    aFollower.userId = [json numberForKey:@"Id"];
    aFollower.username = [json stringForKey:@"Username"];
    aFollower.avatarIsFinal = [json boolValueForKey:@"AvatarFinal" withDefault:NO];
    aFollower.avatarURL = [json stringForKey:@"AvatarUri"];
    aFollower.isOnline = [json boolValueForKey:@"IsOnline" withDefault:NO];
    
    return aFollower;
}
@end

//RBX Thumbnail
@implementation RBXThumbnail
+(RBXThumbnail*) ThumbnailWithJson:(NSDictionary*)json
{
    RBXThumbnail* aThumbnail = [[RBXThumbnail alloc] init];
    aThumbnail.assetId = [NSString stringWithFormat:@"%@", json[@"AssetId"]];
    aThumbnail.assetHash = [NSString stringWithFormat:@"%@", json[@"AssetHash"]];
    aThumbnail.assetTypeId = [json[@"AssetTypeId"] unsignedIntegerValue];
    aThumbnail.assetURL = [NSString stringWithFormat:@"%@", json[@"Url"]];
    aThumbnail.assetIsFinal = [json[@"IsFinal"] boolValue];
    
    return aThumbnail;
}

@end

//RBX Notifications
@implementation RBXNotifications
+(RBXNotifications*) NotificationsWithJson:(NSDictionary *)json
{
    RBXNotifications* allPlayerNotifications = [[RBXNotifications alloc] init];
    allPlayerNotifications.unreadMessages = [json intValueForKey:@"unreadMessageCount" withDefault:0];
    allPlayerNotifications.unansweredFriendRequests = [json intValueForKey:@"friendRequestsCount" withDefault:0];
    
    return allPlayerNotifications;
}

@end

@implementation RBXSponsoredEvent

+(RBXSponsoredEvent*) SponsoredEventWithJson:(NSDictionary *)json
{
    //[{"Name":"wintergames2015",
    //  "Title":"wintergames2015",
    //  "LogoImageURL":"http://images.rbxcdn.com/99bc31cc7d069159abb1e3e959a19724",
    //  "PageType":"Sponsored",
    //  "PageUrl":"/sponsored/wintergames2015"}]
    RBXSponsoredEvent* event = [[RBXSponsoredEvent alloc] init];
    event.eventName = [json stringForKey:@"Name" withDefault:@""];
    event.eventTitle = [json stringForKey:@"Title" withDefault:@""];
    event.eventLogoURLString = [json stringForKey:@"LogoImageURL" withDefault:nil];
    event.eventPageType =[json stringForKey:@"PageType" withDefault:nil];
    event.eventPageURLExtension = [json stringForKey:@"PageUrl" withDefault:nil];
    
    return event;
}
+(NSMutableArray*) EventsWithJson:(NSArray *)jsonArray
{
    NSMutableArray* events = [NSMutableArray array];
    
    for (NSDictionary* jsonObj in jsonArray)
        [events addObject:[RBXSponsoredEvent SponsoredEventWithJson:jsonObj]];
    
    return events;
}

@end


//AB Test
@implementation RBXABTest

+(RBXABTest*) TestWithJSON:(NSDictionary *)json
{
    RBXABTest* aTest = [[RBXABTest alloc] init];
    aTest.experimentName = [json stringForKey:@"ExperimentName" withDefault:nil];
    aTest.subjectType = [json intValueForKey:@"SubjectType" withDefault:nil];
    aTest.subjectTargetId = [json numberForKey:@"SubjectTargetId" withDefault:nil];
    aTest.variation = [json intValueForKey:@"Variation" withDefault:nil];
    aTest.status = [json stringForKey:@"Status" withDefault:@"NoExperiment"];
    return aTest;
}
+(NSDictionary*) TestsFromJSON:(NSArray*)json
{
    if (!json)
        return @{};
    
    NSLog(@"AB Test JSON:");
    NSLog(@"%@", json);
    
    NSMutableDictionary* tests = [NSMutableDictionary dictionaryWithCapacity:json.count];
    for (NSDictionary* jsonObj in json)
    {
        RBXABTest* aTest = [RBXABTest TestWithJSON:jsonObj];
        if (aTest.experimentName)
            [tests setObject:aTest forKey:aTest.experimentName];
    }
    
    return [tests copy];
}

- (BOOL) isLockedOn {
    
    if ([self.status isEqualToString:@"LockedOn"]) {
        return YES;
    }
    
    return NO;
}

- (id) initWithCoder:(NSCoder *)aDecoder {
    if (self == [super init]) {
        self.experimentName = [aDecoder decodeObjectForKey:@"experimentName"];
        self.subjectType = [aDecoder decodeIntForKey:@"subjectType"];
        self.subjectTargetId = [aDecoder decodeObjectForKey:@"subjectTargetId"];
        self.variation = [aDecoder decodeIntForKey:@"variation"];
        self.status = [aDecoder decodeObjectForKey:@"status"];
    }
    
    return self;
}

- (void) encodeWithCoder:(NSCoder *)aCoder {
    [aCoder encodeObject:self.experimentName forKey:@"experimentName"];
    [aCoder encodeInt:self.subjectType forKey:@"subjectType"];
    [aCoder encodeObject:self.subjectTargetId forKey:@"subjectTargetId"];
    [aCoder encodeInt:self.variation forKey:@"variation"];
    [aCoder encodeObject:self.status forKey:@"status"];
}

@end

// RobloxData

@implementation RobloxData

+ (void)initialize
{
    [super initialize];
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _completionQueue = dispatch_queue_create("com.roblox.network_completion_queue", DISPATCH_QUEUE_CONCURRENT);
    });
}




/* fetchRobloxImageURLWithURL: will poll the image url until it is marked as final,
                               or until
 */
+ (void) fetchRobloxImageURLWithURL:(NSString*)urlAsString intentID:(NSUInteger)intent withQueue:(NSOperationQueue*)queue completion:(void(^)(NSString* imageURL))handler
{
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        BOOL isFinalImage = [responseObject boolValueForKey:@"Final"];
        if(isFinalImage)
        {
            NSString* imageURL = [responseObject stringForKey:@"Url" withDefault:nil];
            handler(imageURL);
        }
        else
        {
            // The image is still not generated.
            // Schedule a request with a delay to test if its generation is completed
            NSArray* delays = @[@1, @1, @2, @4, @8];
            
            // Cut condition
            if(intent < delays.count)
            {
                NSOperation* operation = [NSBlockOperation blockOperationWithBlock:^
                {
                    [NSThread sleepForTimeInterval:[delays[intent] doubleValue]];
                    [RobloxData fetchRobloxImageURLWithURL:urlAsString intentID:(intent+1) withQueue:queue completion:handler];
                }];
                [queue addOperation:operation];
            }
            else
            {
                // Enough intents. Return the placeholder image
                NSString* imageURL = [responseObject stringForKey:@"Url" withDefault:nil];
                handler(imageURL);
            }
        }
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    [queue addOperation:operation];
}

/* fetchURLForAssetID: Retrieve the image URL for the given game
 */
+ (void) fetchURLForAssetID:(NSString*)assetID
                   withSize:(CGSize)size
                 completion:(void(^)(NSString* imageURL))handler
{
	NSString* urlAsString = [NSString stringWithFormat:@"%@asset-thumbnail/json?assetId=%@&width=%d&height=%d&format=jpeg",
                             [RobloxInfo getWWWBaseUrl],
                             assetID,
                             (int)size.width,
                             (int)size.height];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [RobloxData fetchRobloxImageURLWithURL:urlAsString intentID:0 withQueue:queue completion:handler];
}
+ (void) fetchThumbnailURLForGameID:(NSString*)assetID
                           withSize:(CGSize)size
                         completion:(void(^)(NSString* imageURL))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@asset-thumbnail/json?assetId=%@&width=%d&height=%d&format=jpeg&ignoreAssetMedia=true",
                             [RobloxInfo getWWWBaseUrl],
                             assetID,
                             (int)size.width,
                             (int)size.height];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [RobloxData fetchRobloxImageURLWithURL:urlAsString intentID:0 withQueue:queue completion:handler];
}

/* fetchThumbnailsForPlace: Retrieves an array of RBXThumbnails for a given game
 */
+ (void) fetchThumbnailsForPlace:(NSString*)placeID
                        withSize:(CGSize)size
                      completion:(void(^)(NSArray* thumbnails))handler
{
    NSString* url = [NSString stringWithFormat:@"%@thumbnail/place-thumbnails/?placeId=%@&imageWidth=%d&imageHeight=%d&imageFormat=jpeg&ignoreAssetMedia=true",
                     [RobloxInfo getBaseUrl],
                     placeID,
                     (int)size.width,
                     (int)size.height];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResponse = responseObject;
        NSArray* jsonThumbnails = [jsonResponse arrayForKey:@"thumbnails" withDefault:nil];
        NSMutableArray* thumbnails = [NSMutableArray array];
        for(NSDictionary* aThumbnail in jsonThumbnails)
        {
            [thumbnails addObject:[RBXThumbnail ThumbnailWithJson:aThumbnail]];
        }
       
        handler([thumbnails copy]);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler([[NSArray alloc] init]);
    }];
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchAvatarURLForUserID: Retrieve the image URL for the given avatar ID
 */
+ (void) fetchAvatarURLForUserID:(NSUInteger)avatarID
                        withSize:(CGSize)size
                      completion:(void(^)(NSString* imageURL))handler
{
	NSString* urlAsString = [NSString stringWithFormat:@"%@avatar-thumbnail/json?userId=%u&width=%d&height=%d&format=png",
                             [RobloxInfo getBaseUrl],
                             (unsigned int)avatarID,
                             (int)size.width,
                             (int)size.height];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [RobloxData fetchRobloxImageURLWithURL:urlAsString intentID:0 withQueue:queue completion:handler];
}

/* Analytics Event Tracking functions:
 */
+(void) initializeBrowserTrackerWithCompletion:(void(^)(bool success, NSString* browserTracker))handler
{
    //construct the url with a unique device identifier: the vendor ID
    NSString* urlString = [NSString stringWithFormat:@"%@/device/initialize?mobileDeviceId=%@",
                           [RobloxInfo getApiBaseUrl],
                           [[[UIDevice currentDevice] identifierForVendor] UUIDString]];
    [self hitEndpoint:urlString
         isGetRequest:NO
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject) {
            NSDictionary* json = (NSDictionary*) responseObject;
            if (json)
            {
                NSString* trackerID = [json stringForKey:@"browserTrackerId" withDefault:nil];
                if (trackerID)
                {
                    // Save the browser tracker Id
                    [self setBrowserTrackerId:trackerID];
                    
                    handler(YES, trackerID);
                    return;
                }
            }
            
            handler(NO, nil);
        }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error) {
            handler(NO, nil);
        }];
}

+ (NSString *) browserTrackerId
{
    id obj = [[NSUserDefaults standardUserDefaults] objectForKey:BROWSER_TRACKER_KEY];
    
    if (![RBXFunctions isEmpty:obj]) {
        if ([obj isKindOfClass:[NSString class]]) {
            return (NSString *)obj;
        }
    }
    
    return nil;
}

+ (void) setBrowserTrackerId:(NSString *)trackerId {
    NSString *oldTrackerId = [[NSUserDefaults standardUserDefaults] objectForKey:BROWSER_TRACKER_KEY];
    
    if ([trackerId isEqualToString:oldTrackerId]) {
        // New tracker is the same as the old tracker; no need to update
        return;
    }
    
    [[NSUserDefaults standardUserDefaults] setObject:trackerId forKey:BROWSER_TRACKER_KEY];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    // 2015.0922 ajain - Report browser tracker change - we expect a tracker to be set upon app launch, so we do not report
    // app launch setting of browser tracker, but after the browser tracker is set the first time, we do report subsequent changes
    if (![RBXFunctions isEmptyString:oldTrackerId]) {
        [[SessionReporter sharedInstance] reportSessionForContext:RBXAContextAppLaunch
                                                           result:RBXAResultSuccess
                                                        errorName:RBXAErrorNoError
                                                     responseCode:999
                                                             data:@{@"btid_changed":@(YES),
                                                                    @"old_tracker_id":oldTrackerId,
                                                                    @"new_tracker_id":trackerId}];
    }
    
    // Remove items tied to old tracker id
    [[ABTestManager sharedInstance] removeStoredExperiments];
}

+ (void) reportEvent:(NSString*)event
      withCompletion:(void(^)(bool success, NSError* err))completion
{
    [self hitEndpoint:event
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         if (completion)
             completion(YES, nil);
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         if (completion)
             completion(NO, error);
     }];
}

/* AB Testing functions:
 */
+ (NSDictionary*) fetchABTestEnrollments:(NSString*)experimentalData
{
    if (!experimentalData)
        return @{};
    
    //configure the request
    NSString* urlAsString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/ab/v1/get-enrollments"];
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlAsString]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[experimentalData dataUsingEncoding:NSUTF8StringEncoding]];
    
    
    //fire off the request
    NSURLResponse* response;
    NSError* error;
    NSData* data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
    
    //check that we haven't gotten some error or bad response
    if (!data || !response || ([(NSHTTPURLResponse*)response statusCode] != 200))
        return @{};
    
    //looks like we got a good response, parse it out and send it back
    return [RBXABTest TestsFromJSON:[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]];
}
+ (NSDictionary*) enrollInABTests:(NSString*)experimentalData
{
    if (!experimentalData)
        return @{};
    
    NSString* urlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/ab/v1/enroll"];
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlString]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[experimentalData dataUsingEncoding:NSUTF8StringEncoding]];
    
    
    //fire off the request
    NSURLResponse* response;
    NSError* error;
    NSData* data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
    
    //check that we haven't gotten some error or bad response
    if (!data || !response || ([(NSHTTPURLResponse*)response statusCode] != 200))
        return @{};
    
    //looks like we got a good response, parse it out and send it back
    return [RBXABTest TestsFromJSON:[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]];
}

/* Sign Up and Login Functions:
 */
+(void) oldCheckIfValidUsername:(NSString*)username
                  completion:(void(^)(BOOL success, NSString* message))handler
{
    //old url
    NSString* url = [NSString stringWithFormat:@"%@/UserCheck/checkifinvalidusernameforsignup?username=%@",
                     [RobloxInfo getSecureBaseUrl],
                     username];
    
    [self hitEndpoint:url
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSDictionary* response = responseObject;
         //old data looks like this:
         //{
         //    "data": 1
         //}
         if ([[response objectForKey:@"data"] boolValue])
             handler(NO, @"");
         else
             handler(YES, @"");
         
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         //if an error occurs with this endpoint, not the username itself,
         // do not prevent the user from signing up, but still return the error message
         if (handler)
             handler(YES, error.description);
     }];
}
+(void) checkIfValidUsername:(NSString*)username
                  completion:(void(^)(BOOL success, NSString* message))handler
{
    //new url
    NSString* url = [NSString stringWithFormat:@"%@/signup/is-username-valid?username=%@",
                     [RobloxInfo getApiBaseUrl],
                     username];
    
    [self hitEndpoint:url
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* response = responseObject;
        
        //new data - 10/15/2015
        bool isValid = [response boolValueForKey:@"IsValid" withDefault:NO];
        NSString* message = [response stringForKey:@"ErrorMessage" withDefault:@""];
        
        if (handler)
            handler(isValid, message);
        
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        //if an error occurs with this endpoint, not the username itself,
        // do not prevent the user from signing up, but still return the error message
        if (handler)
            handler(YES, error.description);
    }];
}
+(void) checkIfBlacklistedEmail:(NSString*)email
                     completion:(void(^)(BOOL isBlacklisted))handler
{
    NSString* url = [NSString stringWithFormat:@"%@/UserCheck/checkifemailisblacklisted?email=%@",
                     [RobloxInfo getBaseUrl],
                     [email stringByAddingPercentEscapesUsingEncoding:NSASCIIStringEncoding]];
    [self hitEndpoint:url
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* json = responseObject;
        bool isBlacklisted = NO;
        if (json)
            isBlacklisted = [json boolValueForKey:@"success" withDefault:NO];
        
        handler(isBlacklisted);
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        //if there was a problem with this endpoint, do not block signups because of it
        handler(NO);
    }];
}

+(void) recommendUsername:(NSString*)usernameToTry
               completion:(void(^)(BOOL success, NSString* newUsername))handler
{
    NSString* url = [NSString stringWithFormat:@"%@/UserCheck/getrecommendedusername?usernameToTry=%@", [RobloxInfo getSecureBaseUrl], usernameToTry];
    
    NSError* err;
    NSString* usernameRecommendation = [NSString stringWithContentsOfURL:[NSURL URLWithString:url] encoding:NSUTF8StringEncoding error:&err];
    
    if (!err && usernameRecommendation)
    {
        handler(YES, usernameRecommendation);
    }
    else
    {
        handler(NO, err.description);
    }
}
+(void) oldCheckIfValidPassword:(NSString*)password
                       username:(NSString*)username
                     completion:(void(^)(BOOL success, NSString* message))handler
{
    //Old url
    NSString* url = [[RobloxInfo getSecureBaseUrl] stringByAppendingString:@"/UserCheck/validatepasswordforsignup?"];
    NSString* args = [NSString stringWithFormat:@"password=%@&username=%@",password,username];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    [request setHTTPMethod:@"POST"];
    [request setHTTPBody:[args dataUsingEncoding:NSUTF8StringEncoding]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:request queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *responseData, NSError *responseError)
     {
         if(!responseError)
         {
             NSDictionary* responseDict = [[NSDictionary alloc] init];
             NSError* dictError = nil;
             responseDict = [NSJSONSerialization JSONObjectWithData:responseData options:kNilOptions error:&dictError];
    
             if(!dictError && responseDict)
             {
                 bool success = [responseDict boolValueForKey:@"success" withDefault:NO];
                 if (success)
                     handler(YES, @"");
                 else
                 {
                     NSString* err = [responseDict stringForKey:@"error" withDefault:nil];
                     handler(NO, err);
                 }
             }
             else
                 handler(NO, nil);
         }
         else
             handler(NO, nil);
     }];
}
+(void) checkIfValidPassword:(NSString*)password
                    username:(NSString*)username
                  completion:(void(^)(BOOL success, NSString* message))handler
{
    //new URL 10/15/2015
    NSString* url = [NSString stringWithFormat:@"%@/signup/is-password-valid?username=%@&password=%@",
                     [RobloxInfo getApiBaseUrl], username, password];
    [self hitEndpoint:url
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* json = responseObject;
        if (json)
        {
            bool isValid = [json boolValueForKey:@"IsValid" withDefault:NO];
            NSString* message = [json stringForKey:@"ErrorMessage" withDefault:nil];
            
            if (handler)
                handler(isValid, message);
        }
        else
        {
            if (handler)
                handler(NO, nil);
        }
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        if (handler)
            handler(NO, error.description);
    }];
}


/* Force Upgrade
 */
+ (void) checkForForceUpgrade:(NSString*)versionNumber
               withCompletion:(void(^)(NSDictionary* response))handler;
{
    NSString* urlString = [NSString stringWithFormat:@"%@mobileapi/check-app-version?appVersion=%@", [RobloxInfo getWWWBaseUrl], versionNumber];
    [self hitEndpoint:urlString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         handler(responseObject);
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(nil);
     }];
}




/* fetchDefaultSorts: Retrieves the selected sorts to display in the main page
 */
+ (void) fetchDefaultSorts:(NSUInteger)minimumGamesInSort
                completion:(void(^)(NSArray* sorts)) handler
{
	NSString* url = [NSString stringWithFormat:@"%@/games/default-sorts/?gamesRowLength=%d", [RobloxInfo getBaseUrl], (int)minimumGamesInSort];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;

    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
	{
        NSArray* jsonSorts = responseObject;
        
        NSMutableArray* sorts = [NSMutableArray array];
        for(NSDictionary* jsonSort in jsonSorts)
        {
            [sorts addObject:[RBXGameSort GameSortWithJSON:jsonSort]];
        }
		
        handler(sorts);
    }
	failure:^(AFHTTPRequestOperation *operation, NSError *error)
	{
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchAllSorts: Retrieves the all the sorts
 */
+ (void) fetchAllSorts:(void(^)(NSArray* sorts)) completionHandler
{
	NSString* url = [NSString stringWithFormat:@"%@/games/all-sorts", [RobloxInfo getBaseUrl]];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]];
    [request setValue:[RobloxInfo getUserAgentString] forHTTPHeaderField:@"User-Agent"];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
	{
        NSArray* jsonSorts = responseObject;
        NSMutableArray* sorts = [NSMutableArray array];
        for(NSDictionary* jsonSort in jsonSorts)
        {
            RBXGameSort* sort = [[RBXGameSort alloc] init];
            sort.sortID = [jsonSort numberForKey:@"Id"];
            sort.title = [jsonSort stringForKey:@"Name"];
            
            [sorts addObject:sort];
        }
		
        completionHandler(sorts);
    }
	failure:^(AFHTTPRequestOperation *operation, NSError *error)
	{
        completionHandler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchUserPlaces: Retrieves the user places
 */
+ (void) fetchUserPlaces:(NSNumber*)userID
              startIndex:(NSUInteger)startIndex
                numGames:(NSUInteger)numGames
               thumbSize:(CGSize)thumbSize
              completion:(void(^)(NSArray* games)) handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/games/list-users-games-json?userId=%@&startIndex=%lu&pageSize=%lu&thumbWidth=%d&thumbHeight=%d&thumbFormat=png",
                             [RobloxInfo getBaseUrl],
                             [userID stringValue],
                             (unsigned long)startIndex,
                             (unsigned long)numGames,
                             (int)thumbSize.width,
                             (int)thumbSize.height];
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSArray* jsonGames = (NSArray*) responseObject;
        NSMutableArray* games = [[NSMutableArray alloc] init];

        for(NSDictionary* jsonGame in jsonGames)
        {
            RBXGameData* game = [RBXGameData parseGameFromJSON:jsonGame];
            [games addObject:game];
        }
        handler(games);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchUserFavoriteGames: Retrieves the user favorite games
 */
+ (void) fetchUserFavoriteGames:(NSNumber*)userID
                     startIndex:(NSUInteger)startIndex
                       numGames:(NSUInteger)numGames
                      thumbSize:(CGSize)thumbSize
                     completion:(void(^)(NSArray* games)) handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/games/list-users-favorite-games-json?userId=%@&startIndex=%lu&pageSize=%lu&thumbWidth=%d&thumbHeight=%d&thumbFormat=png",
                             [RobloxInfo getBaseUrl],
                             [userID stringValue],
                             (unsigned long)startIndex,
                             (unsigned long)numGames,
                             (int)thumbSize.width,
                             (int)thumbSize.height];
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSArray* jsonGames = (NSArray*) responseObject;
        NSMutableArray* games = [[NSMutableArray alloc] init];

        for(NSDictionary* jsonGame in jsonGames)
        {
            RBXGameData* game = [RBXGameData parseGameFromJSON:jsonGame];
            [games addObject:game];
        }
        handler(games);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchGameList: Retrieves a list of games from the server
 */

+ (void) fetchGameListWithSortID:(NSNumber*)sortID
                         genreID:(NSNumber*)genreID
                       fromIndex:(NSUInteger)startIndex
                        numGames:(NSUInteger)numGames
                       thumbSize:(CGSize)thumbSize
                      completion:(void (^)(NSArray* games)) handler
{
    [RobloxData fetchGameListWithSortID:sortID genreID:genreID playerID:nil fromIndex:startIndex numGames:numGames thumbSize:thumbSize completion:handler];
}

+ (void) fetchGameListWithSortID:(NSNumber*)sortID
                         genreID:(NSNumber*)genreID
                        playerID:(NSNumber*)playerID
                       fromIndex:(NSUInteger)startIndex
                        numGames:(NSUInteger)numGames
                       thumbSize:(CGSize)thumbSize
                      completion:(void (^)(NSArray* games)) handler
{
    if( [sortID isEqualToNumber:[RBXGameSort MyGamesSort].sortID] )
    {
        [RobloxData fetchUserPlaces:playerID startIndex:startIndex numGames:numGames thumbSize:thumbSize completion:handler];
    }
    else if( [sortID isEqualToNumber:[RBXGameSort FavoriteGamesSort].sortID] )
    {
        [RobloxData fetchUserFavoriteGames:playerID startIndex:startIndex numGames:numGames thumbSize:thumbSize completion:handler];
    }
    else if( [sortID isEqualToNumber:[RBXGameSort DefaultSort].sortID] )
    {
        [self fetchGameListWithSortID:nil genreID:genreID playerID:playerID fromIndex:startIndex numGames:numGames thumbSize:thumbSize completion:handler];
    }
    else
    {
        NSMutableString* urlAsString = [NSMutableString stringWithFormat:@"%@/Games/List-Json?sortFilter=%d&startRows=%d&maxRows=%d&thumbWidth=%d&thumbHeight=%d&thumbFormat=jpeg",
                                        [RobloxInfo getBaseUrl],
                                        [sortID intValue],
                                        (int)startIndex,
                                        (int)numGames,
                                        (int)thumbSize.width,
                                        (int)thumbSize.height];
        if (![RBXFunctions isEmpty:genreID]) {
            [urlAsString appendString:[NSString stringWithFormat:@"&genreID=%ld", (long)[genreID integerValue]]];
        }
        NSURL *url = [[NSURL alloc] initWithString:urlAsString];
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
        [RobloxInfo setDefaultHTTPHeadersForRequest:request];
        
        AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
        operation.responseSerializer = [AFJSONResponseSerializer serializer];
        operation.completionQueue = _completionQueue;
        
        [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            NSArray* jsonGames = (NSArray*) responseObject;
            NSMutableArray* games = [NSMutableArray arrayWithCapacity:jsonGames.count];
            
            for(NSDictionary* jsonGame in jsonGames)
            {
                RBXGameData* game = [RBXGameData parseGameFromJSON:jsonGame];
                [games addObject:game];
            }
            handler(games);
            
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(nil);
        }];
        
        NSOperationQueue* queue = [[NSOperationQueue alloc] init];
        [queue addOperation:operation];
    }
}

/* fetchGameDetails: Retrieve full game details
 */
+ (void) fetchGameDetails:(NSString*)placeID
               completion:(void(^)(RBXGameData* game))handler
{
	NSString* urlAsString = [NSString stringWithFormat:@"%@places/api-get-details/?assetId=%@", [RobloxInfo getWWWBaseUrl], placeID];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
	{
        NSDictionary* xmlGame = (NSDictionary*) responseObject;
		
		RBXGameData* game = [[RBXGameData alloc] init];
        game.placeID = placeID;
		game.creatorName = [xmlGame stringForKey:@"Builder"];
		game.dateCreated = [xmlGame stringForKey:@"Created"];
        game.dateUpdated = [xmlGame stringForKey:@"Updated"];
		game.gameDescription = [xmlGame stringForKey:@"Description"];
		game.favorited = [xmlGame unsignedValueForKey:@"FavoritedCount"];
		game.maxPlayers = [xmlGame unsignedValueForKey:@"MaxPlayers"];
		game.title = [xmlGame stringForKey:@"Name"];
		game.visited = [xmlGame unsignedValueForKey:@"VisitedCount"];
        game.thumbnailURL = nil;
        game.thumbnailIsFinal = false;
        game.onlineCount = [xmlGame unsignedValueForKey:@"OnlineCount"];
        game.assetGenre = [xmlGame stringForKey:@"AssetGenre"];
        game.userFavorited = [xmlGame boolValueForKey:@"IsFavoritedByUser"];
        game.universeID = [xmlGame numberForKey:@"UniverseId" withDefault:nil];
        game.upVotes = [xmlGame unsignedValueForKey:@"TotalUpVotes"];
        game.downVotes = [xmlGame unsignedValueForKey:@"TotalDownVotes"];
        
        game.userOwns = NO;
        game.productID = 0;
        game.price = 0;
        
        id userVote = [xmlGame objectForKey:@"UserVote"];
        if(userVote != nil && [userVote isKindOfClass:[NSNumber class]] )
            game.userVote = [userVote boolValue] == YES ? RBXUserVotePositive : RBXUserVoteNegative;
        else
            game.userVote = RBXUserVoteNoVote;
        
		handler(game);
    }
	failure:^(AFHTTPRequestOperation *operation, NSError *error)
	{
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchGameGear: Retrieve game gear
 */
+ (void) fetchGameGear:(NSString*)placeID
            startIndex:(NSUInteger)startIndex
               maxRows:(NSUInteger)maxRows
            completion:(void(^)(NSArray* gear))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@PlaceItem/GetPlaceProductPromotions/?placeId=%@&startIndex=%d&maxRows=%d",
                             [RobloxInfo getBaseUrl],
                             placeID,
                             (int)startIndex,
                             (int)maxRows];
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonGear = (NSDictionary*) responseObject;
        NSArray* gearList = jsonGear[@"data"];
        
        NSMutableArray* result = [NSMutableArray array];
        for(NSDictionary* gearData in gearList)
        {
            RBXGameGear* gear = [[RBXGameGear alloc] init];
            gear.productID = [gearData numberForKey:@"ProductID"];
            gear.assetID = [gearData numberForKey:@"AssetID"];
            gear.name = [gearData stringForKey:@"Name"];
            gear.gearDescription = [gearData stringForKey:@"Description"];
            gear.sellerID = [gearData numberForKey:@"SellerID"];
            gear.sellerName = [gearData stringForKey:@"SellerName"];
            gear.totalSales = [gearData unsignedValueForKey:@"totalSales"];
            gear.userOwns = [gearData boolValueForKey:@"UserOwns"];
            gear.isRentable = [gearData boolValueForKey:@"IsRentable"];
            gear.promotionID = [gearData numberForKey:@"PromotionID"];
            gear.isForSale = [gearData boolValueForKey:@"IsForSale"];
            gear.favoriteCount = [gearData unsignedValueForKey:@"TotalFavorites"];
            gear.userFavorited = [gearData boolValueForKey:@"IsFavoritedByUser"];
            gear.priceInRobux = [gearData unsignedValueForKey:@"PriceInRobux"];
            gear.priceInTickets = [gearData unsignedValueForKey:@"PriceInTickets"];
            
            [result addObject:gear];
        }
        
        handler(result);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchGamePasses: Retrieve game passes
 */
+ (void) fetchGamePasses:(NSString*)placeID
              startIndex:(NSUInteger)startIndex
                 maxRows:(NSUInteger)maxRows
              completion:(void(^)(NSArray* passes))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@PlaceItem/GetGamePassesPaged/?placeId=%@&startIndex=%d&maxRows=%d",
                             [RobloxInfo getBaseUrl],
                             placeID,
                             (int)startIndex,
                             (int)maxRows];
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonPass = (NSDictionary*) responseObject;
        NSArray* passList = jsonPass[@"data"];
         
        NSMutableArray* result = [NSMutableArray array];
        for(NSDictionary* passData in passList)
        {
            RBXGamePass* pass = [[RBXGamePass alloc] init];
            pass.productID = [passData numberForKey:@"ProductID"];
            pass.passID = [passData numberForKey:@"PassID"];
            pass.passName = [passData stringForKey:@"PassName"];
            pass.passDescription = [passData stringForKey:@"Description"];
            pass.totalSales = [passData unsignedValueForKey:@"TotalSales"];
            pass.upVotes = [passData unsignedValueForKey:@"TotalUpVotes"];
            pass.downVotes = [passData unsignedValueForKey:@"TotalDownVotes"];
            pass.userFavorited = [passData boolValueForKey:@"IsFavoritedByUser"];
            pass.favoriteCount = [passData unsignedValueForKey:@"TotalFavorites"];
            pass.userOwns = [passData boolValueForKey:@"UserOwns"];
            pass.priceInRobux = [passData unsignedValueForKey:@"PriceInRobux"];
            pass.priceInTickets = [passData unsignedValueForKey:@"PriceInTickets"];
            
            id userVote = [passData objectForKey:@"UserVote"];
            if(userVote != nil && [userVote isKindOfClass:[NSNumber class]] )
                pass.userVote = [userVote boolValue] == YES ? RBXUserVotePositive : RBXUserVoteNegative;
            else
                pass.userVote = RBXUserVoteNoVote;
            
            [result addObject:pass];
        }
        
        handler(result);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

+ (void) fetchGameBadges:(NSString*)placeID
              completion:(void(^)(NSArray* badges))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@badges/list-badges-for-place?placeId=%@",
                             [RobloxInfo getBaseUrl],
                             placeID];
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonBadge = (NSDictionary*) responseObject;
        if(jsonBadge != nil && [jsonBadge objectForKey:@"GameBadges"] != nil)
        {
            NSArray* passList = jsonBadge[@"GameBadges"];
            
            NSMutableArray* result = [NSMutableArray array];
            for(NSDictionary* badgeData in passList)
            {
                [result addObject:[RBXBadgeInfo parseGameBadgeFromJSON:badgeData]];
            }
            
            handler(result);
        }
        else
            handler(nil);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}


/* searchGames: Retrieves a list of games from the server
 */
+ (void) searchGames:(NSString*)keyword
		   fromIndex:(NSUInteger)startIndex
            numGames:(NSUInteger)numGames
           thumbSize:(CGSize)thumbSize
          completion:(void (^)(NSArray* games)) handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/Games/List-Json?keyword=%@&startRows=%d&maxRows=%d&thumbWidth=%d&thumbHeight=%d&thumbFormat=jpeg",
                             [RobloxInfo getWWWBaseUrl],
                             [keyword stringByAddingPercentEscapesUsingEncoding:NSASCIIStringEncoding],
							 (int)startIndex,
                             (int)numGames,
                             (int)thumbSize.width,
                             (int)thumbSize.height];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation
        setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            NSArray* jsonGames = (NSArray*) responseObject;
            NSMutableArray* games = [[NSMutableArray alloc] init];

            for(NSDictionary* jsonGame in jsonGames)
            {
                RBXGameData* game = [[RBXGameData alloc] init];
                game.players = [jsonGame stringForKey:@"PlayerCount"];
                game.title = [jsonGame stringForKey:@"Name"];
                game.creatorName = [jsonGame stringForKey:@"CreatorName"];
                game.placeID = [jsonGame stringForKey:@"PlaceID"];
                game.visited = [jsonGame unsignedValueForKey:@"Plays"];
                game.thumbnailURL = [jsonGame stringForKey:@"ThumbnailUrl"];
                game.thumbnailIsFinal = [jsonGame boolValueForKey:@"ThumbnailIsFinal"];
                game.userFavorited = NO;
                game.upVotes = [jsonGame unsignedValueForKey:@"TotalUpVotes"];
                game.downVotes = [jsonGame unsignedValueForKey:@"TotalDownVotes"];
                
                [games addObject:game];
            }
            
            handler(games);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(nil);
        }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* searchUsers: Retrieves a list of users from the server
 */
+ (void) searchUsers:(NSString*)keyword
           fromIndex:(NSUInteger)startIndex
            numUsers:(NSUInteger)numUsers
           thumbSize:(CGSize)thumbSize
          completion:(void (^)(NSArray* users)) handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/search/search-users-json?keyword=%@&includeAvatar=true&startRows=%d&maxRows=%d&imgWidth=%d&imgHeight=%d&imgFormat=jpeg",
                             [RobloxInfo getWWWBaseUrl],
                             [keyword stringByAddingPercentEscapesUsingEncoding:NSASCIIStringEncoding],
                             (int)startIndex,
                             (int)numUsers,
                             (int)thumbSize.width,
                             (int)thumbSize.height];
        
    
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        //parse the responseObject. It looks something like this:
        //"Keyword": "imightbelying",
        //"StartRow": 0,
        //"MaxRows": 10,
        //"UserSearchResults": [
        //                      {
        //                          "AvatarUri": "http://t2.rbxcdn.com/d7882cb26ca4a298b386c9a4b1cbb24c",
        //                          "AvatarFinal": true,
        //                          "UserId": 68465808,
        //                          "Name": "iMightBeLying",
        //                          "Blurb": "a really long string with /n and other characters",
        //                          "PreviousUserNamesCsv": "",
        //                          "IsOnline": true,
        //                          "LastLocation": "Website",
        //                          "UserProfilePageUrl": "/User.aspx?id=68465808",
        //                          "LastSeenDate": "1/14/2015 12:04 PM"
        //                      }
        //]
        NSDictionary* json = responseObject;
        NSMutableArray* users = [NSMutableArray arrayWithCapacity:[json intValueForKey:@"MaxRows"]];
        NSArray* jsonUsers = [json arrayForKey:@"UserSearchResults" withDefault:@[]];
        for (int i = 0; i < jsonUsers.count; i++)
        {
            [users addObject:[RBXUserSearchInfo UserSearchInfoWithJSON:jsonUsers[i]]];
        }
        
        handler([users copy]);
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        //something went wrong, return an empty array
        handler(@[]);
    }];
    
}

/* fetchUserLeaderboardInfo: Retrieve the leaderboard info for a specific game and the current logged in user.
 Returns null if there is no session active or there was an error in the request.
 */
+ (void) fetchUserLeaderboardInfo:(RBXLeaderboardsTarget)target
                           userID:(NSNumber*)currentUserID
                    distributorID:(NSNumber*)distributorID
                       timeFilter:(RBXLeaderboardsTimeFilter)time
                       avatarSize:(CGSize)avatarSize
                       completion:(void(^)(RBXLeaderboardEntry* userEntry))handler
{
    //example
    //http://www.roblox.com/leaderboards/rank/json?targetType=0&distributorTargetId=65241&timeFilter=0&max=1&imgWidth=30&imgHeight=30&imgFormat=png
    //{
    //  "Rank": 144,
    //  "DisplayRank": "144",
    //  "FullRank": null,
    //  "WasRankTruncated": false,
    //  "Name": "iMightBeLying",
    //  "UserId": 68465808,
    //  "TargetId": 68465808,
    //  "ProfileUri": "/User.aspx?ID=68465808",
    //  "ClanName": null,
    //  "ClanUri": null,
    //  "Points": 204,
    //  "DisplayPoints": "204",
    //  "FullPoints": null,
    //  "WasPointsTruncated": false,
    //  "ClanEmblemID": 0,
    //  "UserImageUri": "http://t6.rbxcdn.com/74dc9e800fde9bea6c4e512860283620",
    //  "UserImageFinal": true,
    //  "ClanImageUri": "",
    //  "ClanImageFinal": false
    //}

	NSString* urlAsString = [NSString stringWithFormat: @"%@leaderboards/rank/json/?targetType=%lu"
                                                        @"&distributorTargetId=%ld"
                                                        @"&timeFilter=%lu"
                                                        @"&max=2"
                                                        @"&imgWidth=%d&imgHeight=%d&imgFormat=png",
                             [RobloxInfo getBaseUrl],
                             (unsigned long)target,
                             (long)(distributorID != nil ? [distributorID integerValue] : 0),
                             (unsigned long)time,
                             (int)avatarSize.width,
                             (int)avatarSize.height];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSArray* jsonResults = (NSArray*) responseObject;
         
         // Find the user entry in the results
         NSDictionary* jsonUserEntry = nil;
         for(NSDictionary* jsonEntry in jsonResults)
         {
             NSNumber* userID = [jsonEntry numberForKey:@"UserId"];
             if([userID isEqualToNumber:currentUserID])
             {
                 jsonUserEntry = jsonEntry;
                 break;
             }
         }
         
         // return the parsed result if there is one
         handler(jsonUserEntry != nil ? [RBXLeaderboardEntry parseLeaderboardEntry:jsonUserEntry] : nil);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(nil);
     }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchLeaderboards: Retrieve leaderboards for a specific game
 */
+ (void) fetchLeaderboardsFor:(RBXLeaderboardsTarget)target
                   timeFilter:(RBXLeaderboardsTimeFilter)time
                distributorID:(NSNumber*)distributorID
                   startIndex:(NSUInteger)startIndex
                     numItems:(NSUInteger)numItems
                    lastEntry:(RBXLeaderboardEntry*)lastEntry
                   avatarSize:(CGSize)avatarSize
                   completion:(void(^)(NSArray* leaderboard))handler
{
    ///{
    ///     "Rank": 1,
    ///     "DisplayRank": "1",
    ///     "FullRank": null,
    ///     "WasRankTruncated": false,
    ///     "Name": "balidhan",
    ///     "UserId": 31106566,
    ///     "TargetId": 31106566,
    ///     "ProfileUri": "/User.aspx?ID=31106566",
    ///     "ClanName": null,
    ///     "ClanUri": null,
    ///     "Points": 3657,
    ///     "DisplayPoints": "3,657",
    ///     "FullPoints": null,
    ///     "WasPointsTruncated": false,
    ///     "ClanEmblemID": 0,
    ///     "UserImageUri": "http://t7.rbxcdn.com/85136625f88cd07dd4e22b2b7d7a75b8",
    ///     "UserImageFinal": true,
    ///     "ClanImageUri": "",
    ///     "ClanImageFinal": false
    ///}

	NSString* urlAsString = [NSString stringWithFormat:@"%@leaderboards/game/json/?targetType=%lu"
                                                       @"&distributorTargetId=%ld"
                                                       @"&timeFilter=%lu"
                                                       @"&startIndex=%lu&max=%lu"
                                                       @"&currentRank=%lu"
                                                       @"&previousPoints=%lu"
                                                       @"&imgWidth=%d&imgHeight=%d&imgFormat=png",
                             [RobloxInfo getBaseUrl],
                             (unsigned long)target,
                             (long)(distributorID != nil ? [distributorID integerValue] : 0),
                             (unsigned long)time,
                             (unsigned long)startIndex,
                             (unsigned long)numItems,
                             (unsigned long)(lastEntry != nil ? lastEntry.rank : 1),
                             (unsigned long)(lastEntry != nil ? lastEntry.points : 0),
                             (int)avatarSize.width,
                             (int)avatarSize.height];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
	{
        NSArray* jsonResults = (NSArray*) responseObject;
        
        NSMutableArray* leaderboard = [NSMutableArray arrayWithCapacity:jsonResults.count];
        
        for(NSDictionary* jsonEntry in jsonResults)
        {
            RBXLeaderboardEntry* entry = [RBXLeaderboardEntry parseLeaderboardEntry:jsonEntry];
            [leaderboard addObject:entry];
        }
		
		handler(leaderboard);
    }
	failure:^(AFHTTPRequestOperation *operation, NSError *error)
	{
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchMyProfileWithSize: Retrieve current user profile info
 */
+ (void) fetchMyProfileWithSize:(CGSize)avatarSize
                     completion:(void(^)(RBXUserProfileInfo* profile))handler
{
	NSString* urlAsString = [NSString stringWithFormat:@"%@my/profile?imgWidth=%d&imgHeight=%d",
                             [RobloxInfo getBaseUrl],
                             (int)avatarSize.width,
                             (int)avatarSize.height];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResult = responseObject;
        handler([RBXUserProfileInfo UserProfileInfoWithJSON:jsonResult]);
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* fetchUserProfile: Retrieve a user profile info
 */
+ (void) fetchUserProfile:(NSNumber*)userID
               avatarSize:(CGSize)avatarSize
			   completion:(void(^)(RBXUserProfileInfo* profile))handler
{
	NSString* urlAsString = [NSString stringWithFormat:@"%@profile?userId=%d&imgWidth=%d&imgHeight=%d",
                             [RobloxInfo getBaseUrl],
                             [userID intValue],
                             (int)avatarSize.width,
                             (int)avatarSize.height];
    
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResult = responseObject;
        handler([RBXUserProfileInfo UserProfileInfoWithJSON:jsonResult]);
    }
    handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
}

/* fetchUserFriends
 */
+ (void) fetchUserFriends:(NSNumber*)userID
               friendType:(RBXFriendType)friendType
               startIndex:(NSUInteger)startIndex
                 numItems:(NSUInteger)numItems
               avatarSize:(CGSize)avatarSize
               completion:(void(^)(NSUInteger totalFriends, NSArray* friends))handler
{
    NSString* friendTypeStr = nil;
    switch (friendType)
    {
        case RBXFriendTypeAllFriends: friendTypeStr = @"AllFriends"; break;
        case RBXFriendTypeRegularFriend: friendTypeStr = @"RegularFriends"; break;
        case RBXFriendTypeBestFriend: friendTypeStr = @"BestFriends"; break;
        case RBXFriendTypeFriendRequest: friendTypeStr = @"FriendRequests"; break;
    }
    
    // friendsType: AllFriends, RegularFriends, BestFriends
	NSString* urlAsString = [NSString stringWithFormat:@"%@friends/json?userId=%d&currentPage=%lu&pageSize=%lu&imgWidth=%d&imgHeight=%d&imgFormat=jpeg&friendsType=%@",
                             [RobloxInfo getBaseUrl],
                             [userID intValue],
                             (unsigned long)startIndex,
                             (unsigned long)numItems,
                             (int)avatarSize.width,
                             (int)avatarSize.height,
                             friendTypeStr];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResult = responseObject;
        NSArray* jsonFriends = jsonResult[@"Friends"];
        
        NSUInteger totalFriends = [jsonResult[@"TotalFriends"] integerValue];
        
        if(jsonFriends)
        {
            NSMutableArray* friends = [[NSMutableArray alloc] initWithCapacity:jsonFriends.count];

            for(NSDictionary* jsonFriend in jsonFriends)
            {
                [friends addObject:[RBXFriendInfo FriendInfoWithJSON:jsonFriend]];
            }

            handler(totalFriends, friends);
        }
        else
        {
            handler(totalFriends, nil);
        }
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(0, nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}


/* fetchUserBadges
 */
+ (void) fetchUserBadges:(RBXUserBadgeType)badgeType
                 forUser:(NSNumber*)userID
               badgeSize:(CGSize)badgeSize
              completion:(void(^)(NSArray* badges))handler
{
    // friendsType: AllFriends, RegularFriends, BestFriends
	NSString* urlAsString = [NSString stringWithFormat:@"%@badges/%@?userId=%d&imgWidth=%d&imgHeight=%d&imgFormat=png",
                             [RobloxInfo getBaseUrl],
                             (badgeType == RBXUserBadgeTypePlayer) ? @"player" : @"roblox",
                             [userID intValue],
                             (int)badgeSize.width,
                             (int)badgeSize.height];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
	operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResults = responseObject;
        if(jsonResults)
        {
            NSArray* jsonBadges = [NSArray array];
            if (badgeType == RBXUserBadgeTypeRoblox)
            {
                jsonBadges = [jsonResults arrayForKey:@"RobloxBadges"];
            }
            else if (badgeType == RBXUserBadgeTypePlayer)
            {
                jsonBadges = [jsonResults arrayForKey:@"PlayerBadges"];
            }
            
            NSMutableArray* badges = [[NSMutableArray alloc] initWithCapacity:jsonBadges.count];
            for(NSDictionary* jsonBadge in jsonBadges)
            {
                [badges addObject:[RBXBadgeInfo parseGameBadgeFromJSON:jsonBadge]];
            }

            handler(badges);
        }
        else
        {
            handler(nil);
        }
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* acceptFriendRequest  - Actively phasing out in favor for API proxy calls
 */
+ (void) acceptFriendRequest:(NSUInteger)invitationID
            withTargetUserID:(NSUInteger)targetUserID
                  completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/acceptfriendrequest?invitationID=%lu&targetUserID=%lu", [RobloxInfo getBaseUrl], (unsigned long)invitationID, (unsigned long)targetUserID];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}


/* declineFriendRequest - Actively phasing out in favor for API proxy calls
 */
+ (void) declineFriendRequest:(NSUInteger)invitationID
             withTargetUserID:(NSUInteger)targetUserID
                   completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/declinefriendrequest?invitationID=%lu&targetUserID=%lu", [RobloxInfo getBaseUrl], (unsigned long)invitationID, (unsigned long)targetUserID];
    
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* addFriend  - Actively phasing out in favor for API proxy calls
 */
+ (void) sendFriendRequest:(NSNumber*)userID
                completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/sendfriendrequest?targetUserID=%lu",
                                [RobloxInfo getBaseUrl],
                                (unsigned long)[userID unsignedIntegerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* request friendship - API Proxy alternative to sending a friendship request
 */
+ (void) requestFrienshipWithUserID:(NSNumber*)friendUserID
                        completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/user/request-friendship?recipientUserId=%ld", [RobloxInfo getApiBaseUrl], (long)[friendUserID integerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
              success:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         handler(YES);
     }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(NO);
     }];
}


/* removeFriend - Actively phasing out in favor for API proxy calls
 */
+ (void) removeFriend:(NSNumber*)userID
           completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/removefriend?targetUserID=%lu",
                                [RobloxInfo getBaseUrl],
                                (unsigned long)[userID unsignedIntegerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* unfriend - API proxy equivalent to removeFriend
 */
+ (void) unfriend:(NSNumber*)userID
           completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/user/unfriend?friendUserId=%lu",
                             [RobloxInfo getApiBaseUrl],
                             (unsigned long)[userID unsignedIntegerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
              success:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         handler(YES);
     }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(NO);
     }];
}


/* makeBestFriend - Deprecated
 */
+ (void) makeBestFriend:(NSNumber*)userID
            completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/addbestfriend?targetUserID=%lu",
                                [RobloxInfo getBaseUrl],
                                (unsigned long)[userID unsignedIntegerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            //parse the result before blindly returning yes
        
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* removeBestFriend  - Deprecated
 */
+ (void) removeBestFriend:(NSNumber*)userID
               completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@api/friends/removebestfriend?targetUserID=%lu",
                                [RobloxInfo getBaseUrl],
                                (unsigned long)[userID unsignedIntegerValue]];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            //parse the result before blindly returning yes
            
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* followUser
 */
+ (void) followUser:(NSNumber *)userID
         completion:(void (^)(BOOL))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/user/follow?followedUserId=%ld", [RobloxInfo getApiBaseUrl], (long)userID.integerValue];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
              success:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         //responseObject JSON should look like this
         //{
         //    message = Success; //some string
         //    success = 1; //some boolean
         //}
         BOOL successfulRequest = [responseObject boolValueForKey:@"success" withDefault:NO];
         handler(successfulRequest);
     }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(NO);
     }];
}

/* unfollowUser
 */
+ (void) unfollowUser:(NSNumber *)userID
           completion:(void (^)(BOOL))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/user/unfollow?followedUserId=%ld", [RobloxInfo getApiBaseUrl], (long)userID.integerValue];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:urlAsString parameters:nil
              success:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSDictionary* jsonResults = responseObject;
         if (jsonResults)
         {
             //responseObject JSON should look like this
             //{
             //    message = Success; //some string
             //    success = 1; //some boolean
             //}
             BOOL successfulRequest = [responseObject boolValueForKey:@"success" withDefault:NO];
             handler(successfulRequest);
         }
         else
         {
             handler(NO);
         }
     }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(NO);
     }];
}

/* fetch count of friends
 */
+ (void) fetchFriendCount:(NSNumber*)currentUserId
           withCompletion:(void(^)(BOOL success, NSString* message, int count))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/user/get-friendship-count?userId=%ld",
                             [RobloxInfo getApiBaseUrl],
                             (long)currentUserId.integerValue];
    
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* json = responseObject;
        if (json)
        {
            bool success = [json boolValueForKey:@"success" withDefault:NO];
            NSString* message = [json stringForKey:@"message" withDefault:@"Failure"];
            int count = [json intValueForKey:@"count" withDefault:-1];
            
            handler(success, message, count);
            return;
        }
        
        //else
        handler(false, @"Failure", -1);
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(false, @"Failure", -1);
    }];
}

/* fetch a user's followers
 */
+ (void) fetchFollowersForUser:(NSNumber*)userID
                        atPage:(int)userPage
                   avatarSize:(CGSize)avatarSize
                withCompletion:(void(^)(bool success, NSArray* followers))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/users/followers?userId=%ld&page=%d&imageWidth=%d&imageHeight=%d&imageFormat=jpeg",
                            [RobloxInfo getApiBaseUrl], (long)userID.integerValue, userPage, (int)avatarSize.width, (int)avatarSize.height];
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSArray* people = responseObject;
         if (people.count > 0)
         {
             NSMutableArray* followers = [NSMutableArray arrayWithCapacity:people.count];
             for (int i = 0; i < people.count; i++)
             {
                 RBXFollower* follower = [RBXFollower FollowerWithJSON:people[i]];
                 //this can be cleaned up by moving the initialization code to the RBXFriendInfo
                 [followers addObject:[RBXFriendInfo FriendInfoFromFollower:follower]];
             }
             handler(YES, [followers copy]);
         }
         else { handler(NO, @[]); }
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(NO, @[]);
     }];
    
}

/* fetch all the users that a user is following
 */
+ (void) fetchMyFollowingAtPage:(int)userPage
                     avatarSize:(CGSize)avatarSize
                withCompletion:(void(^)(bool success, NSArray* following))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/users/followings?page=%d&imageWidth=%d&imageHeight=%d&imageFormat=jpeg",
                             [RobloxInfo getApiBaseUrl], userPage, (int)avatarSize.width, (int)avatarSize.height];
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSArray* people = responseObject;
        if (people.count > 0)
        {
            NSMutableArray* following = [NSMutableArray arrayWithCapacity:people.count];
            for (int i = 0; i < people.count; i++)
            {
                RBXFollower* follower = [RBXFollower FollowerWithJSON:people[i]];
                //this can be cleaned up by moving the initialization code to the RBXFriendInfo
                [following addObject:[RBXFriendInfo FriendInfoFromFollower:follower]];
            }
            handler(YES, [following copy]);
        }
        else { handler(NO, @[]); }
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(NO, @[]);
    }];
}

/* changeUserPassword
 */
+ (void) changeUserOldPassword:(NSString*)passwordOld
                 toNewPassword:(NSString*)passwordNew
              withConfirmation:(NSString*)passwordConfirm
                 andCompletion:(void(^)(BOOL success, NSString* message))handler
{
    //submit a message to the server to request that the current player's password be changed
    //send an https post request to /account/changepassword
    //XSRF Token required
    //Parameters:
    //  oldPassword - user’s current password
    //  newPassword - new password
    //  confirmNewPassword - same as new password.  must match.
    //Returns JSON object with properties:
    //  Success: true if password was changed
    //  Message: error message, or empty string if successful
    //  sl_translate: just ignore this
    
    //compile request parameters into a dictionary
    NSDictionary *requestParams = [NSDictionary dictionaryWithObjects:@[passwordOld, passwordNew, passwordConfirm]
                                                              forKeys:@[@"oldPassword", @"newPassword", @"confirmNewPassword"]];
    //create the url and secure it with https
    NSString* newPasswordURLString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"account/changepassword"];
    newPasswordURLString = [newPasswordURLString stringByReplacingOccurrencesOfString:@"http:" withString:@"https:"];
    
    //send the request and parse the response
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:newPasswordURLString parameters:requestParams
              success:^(AFHTTPRequestOperation *operation, id responseObject)
                    {
                         NSDictionary* dict = responseObject;
                         bool wasSuccessful = [dict unsignedValueForKey:@"Success"] == 1;
                         if (wasSuccessful)
                             handler(YES, NSLocalizedString(@"SuccessChangePassword", nil));
                         else
                             handler(NO, [dict stringForKey:@"Message"]);
                     }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
                     {
                         handler(NO, NSLocalizedString(@"ErrorUnknownPasswordChange", nil));
                     }];
    

    
}

/* changeUserEmail
 */
+ (void) changeUserEmail:(NSString*)email
            withPassword:(NSString*)password
           andCompletion:(void(^)(BOOL success, NSString* message))handler
{
    //submit a message to the server to request that the current player's email be changed
    //send an https post request to the url : /account/changeemail
    //XSRF Token required
    //parameters :
    //  emailAddress - new email address
    //  password - user’s current password
    //Returns JSON object with properties:
    //  Success: true if email was changed
    //  Message: error message, or “Email Added” if successful
    //  sl_translate: just ignore this
    
    //compile inputs into a dictionary
    NSDictionary *requestParams = [NSDictionary dictionaryWithObjects:@[email, password]
                                                              forKeys:@[@"emailAddress", @"password"]];
    //create the url and secure it with https
    NSString* emailURLString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"account/changeemail"];
    emailURLString = [emailURLString stringByReplacingOccurrencesOfString:@"http:" withString:@"https:"];
    
    //send the request and parse the response
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    [request XSRFPOST:emailURLString parameters:requestParams
              success:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* dict = responseObject;
        bool wasSuccessful = [dict unsignedValueForKey:@"Success"] == 1;
        if (wasSuccessful)
            handler(YES, NSLocalizedString(@"SuccessChangeEmail", nil));
        else
            handler(NO, [dict stringForKey:@"Message"]);
    }
              failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(NO, NSLocalizedString(@"ErrorUnknownEmailChange", nil));
    }];
}

/* fetch My Account
 */
+ (void) fetchMyAccountWithCompletion:(void(^)(RBXUserAccount* account))handler
{
    NSString* urlAsString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"/my/account/json"];
    
    [self hitEndpoint:urlAsString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        handler([RBXUserAccount UserAccountWithJSON:responseObject]);
    }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
}

+ (void) fetchUserHasSetPasswordWithCompletion:(void(^)(bool passwordSet))handler
{
    NSString* urlString = [[[RobloxInfo getApiBaseUrl] stringByReplacingOccurrencesOfString:@"http://" withString:@"https://"] stringByAppendingString:@"/social/has-user-set-password"];
    [self hitEndpoint:urlString
         isGetRequest:YES
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)   {
            NSDictionary* json = (NSDictionary*)responseObject;
            if ([[json stringForKey:@"status"] isEqualToString:@"success"])
            {
                bool isConnected = [[[json stringForKey:@"result" withDefault:@"false"] lowercaseString] isEqualToString:@"true"];
                if (handler)
                    handler(isConnected);
            }
            else
            {
                if (handler)
                    handler(YES); //DEFAULT SHOULD BE YES
            }
        }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)      {
            if (handler)
                handler(YES); //DEFAULT SHOULD BE YES
        }];
}

/* fetch My Account Notifications
 */
+ (void) fetchAccountNotificationsWithCompletion:(void(^)(RBXUserAccountNotifications* notifications))handler
{
    NSString* urlString = [[RobloxInfo getApiBaseUrl] stringByAppendingString:@"/notifications/account"];
    [RobloxData hitEndpoint:urlString
               isGetRequest:YES
              handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        handler([RBXUserAccountNotifications UserAccountNotificationsWithJSON:responseObject]);
    }
              handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler([RBXUserAccountNotifications UserAccountNotificationsWithJSON:nil]);
    }];
}

/* fetch User Presence
 */
+ (void) fetchPresenceForUser:(NSNumber*)userID
               withCompletion:(void(^)(RBXUserPresence*))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/users/%d/onlinestatus", [RobloxInfo getApiBaseUrl], userID.intValue];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlAsString]];
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         handler([RBXUserPresence UserPresenceWithJSON:responseObject]);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         //functions handling this should check for this result
         handler(nil);
     }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}



/* fetchMessages: Retrieve user messages for the specified message type
 */
+ (void) fetchMessages:(RBXMessageType)messageType
            pageNumber:(NSUInteger)pageNumber
              pageSize:(NSUInteger)pageSize
            completion:(void(^)(NSArray* messages))handler
{
    NSString* messageTypeString = @"";
    switch (messageType)
    {
        case RBXMessageTypeInbox: messageTypeString = @"Inbox"; break;
        case RBXMessageTypeSent: messageTypeString = @"Sent"; break;
        case RBXMessageTypeArchive: messageTypeString = @"Archive"; break;
        case RBXMessageTypeAnnouncements: messageTypeString = @"Announcements"; break;
    }
    
    // friendsType: AllFriends, RegularFriends, BestFriends
	NSString* urlAsString = [NSString stringWithFormat:@"%@messages/api/get-messages?pageNumber=%lu&pageSize=%lu&MessageTab=%@",
                             [RobloxInfo getBaseUrl],
                             (unsigned long)pageNumber,
                             (unsigned long)pageSize,
                             messageTypeString];
    
    [RobloxData hitEndpoint:urlAsString 
               isGetRequest:YES
              handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResults = responseObject;
        if(jsonResults)
            handler([RBXMessageInfo ArrayFromJSON:jsonResults]);
        else
            handler(nil);
    }
              handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
}

/* fetchNotifications: Retrieve user notifications
 */
+ (void) fetchNotificationsWithCompletion:(void(^)(NSArray* messages))handler
{
    // friendsType: AllFriends, RegularFriends, BestFriends
	NSString* urlAsString = [NSString stringWithFormat:@"%@notifications/api/get-notifications", [RobloxInfo getBaseUrl]];
	NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    operation.completionQueue = _completionQueue;
    
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSDictionary* jsonResults = responseObject;
        
        if(jsonResults)
        {
            NSMutableArray* messages = [NSMutableArray array];
            NSArray* collection = [jsonResults arrayForKey:@"Collection"];
            
            for(NSDictionary* jsonMessage in collection)
            {
                NSDictionary* sender = [jsonMessage dictionaryForKey:@"Sender"];
                //NSDictionary* senderThumbnail = [jsonMessage dictionaryForKey:@"SenderThumbnail"];
                
                RBXMessageInfo* message = [[RBXMessageInfo alloc] init];
                message.date = [jsonMessage stringForKey:@"Created"];
                //messages.dateUpdated = [jsonMessage stringForKey:@"Updated"];
                message.isRead = [jsonMessage unsignedValueForKey:@"IsRead"] != 0;
                message.recipientUserID = [jsonMessage numberForKey:@"RecipientUserId"];
                message.recipientUsername = [jsonMessage stringForKey:@"RecipientUserName"];
                message.senderUserID = [sender numberForKey:@"UserId"];
                message.senderUsername = [sender stringForKey:@"UserName"];
                
                message.messageID = [jsonMessage unsignedValueForKey:@"Id"];
                
                
                message.subject = [jsonMessage stringForKey:@"Subject"];
                message.body = [jsonMessage stringForKey:@"Body"];
                
                [messages addObject:message];
            }

            handler(messages);
        }
        else
        {
            handler(nil);
        }
    }
    failure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
    
    NSOperationQueue* queue = [[NSOperationQueue alloc] init];
    [queue addOperation:operation];
}

/* markMessageAsRead: Mark the specified message as read
 */
+ (void) markMessageAsRead:(NSUInteger)messageID completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@messages/api/mark-messages-read", [RobloxInfo getBaseUrl]];

    NSDictionary* message = @{@"messageIds": @[[NSNumber numberWithInteger:messageID]] };
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    request.requestSerializer = [AFJSONRequestSerializer serializer];
    
    [request XSRFPOST:urlAsString parameters:message
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            handler(YES);
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO);
        }];
}

/* fetchTotalNotificationsForUser: WithCompletion: Retrieve a count of unread messages, system notifications, and outstanding friend requests
 */
+ (void) fetchTotalNotificationsForUser:(NSNumber*)userID WithCompletion:(void(^)(RBXNotifications* notifications))handler;
{
    //NSString* urlAsString = [NSString stringWithFormat:@"%@messages/api/get-my-unread-messages-count", [RobloxInfo getBaseUrl]];
    NSString* urlAsString = [NSString stringWithFormat:@"%@/incoming-items/counts", [RobloxInfo getApiBaseUrl]];
    
    [RobloxData hitEndpoint:urlAsString
               isGetRequest:YES
              handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        handler([RBXNotifications NotificationsWithJson:responseObject]);
    }
              handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
}

/* fetchSponsoredEvents: Retrieve a list of Roblox's Sponsored and Holiday Events
 */
+ (void) fetchSponsoredEvents:(void (^)(NSArray *))handler
{
    NSString* urlAsString = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"sponsoredpage/list-json"];
    
    [RobloxData hitEndpoint:urlAsString
               isGetRequest:YES
              handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
    {
        NSMutableArray* jsonArray = responseObject;
        if (jsonArray)
            handler([RBXSponsoredEvent EventsWithJson:jsonArray]);
        else
            handler(nil);
        
    }
              handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
    {
        handler(nil);
    }];
}


/* sendMessage: Send o reply a message to another user
 */
+ (void) sendMessage:(NSString*)body
             subject:(NSString*)subject
         recipientID:(NSNumber*)recipientID
      replyMessageID:(NSNumber*)replyMessageID
          completion:(void(^)(BOOL success, NSString* errorMessage))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@messages/send", [RobloxInfo getBaseUrl]];

    NSMutableDictionary* message = [NSMutableDictionary dictionary];
    [message setObject:subject forKey:@"subject"];
    [message setObject:body forKey:@"body"];
    [message setObject:recipientID forKey:@"recipientId"];
    if(replyMessageID != nil)
        [message setObject:replyMessageID forKey:@"replyMessageId"];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    request.requestSerializer = [AFJSONRequestSerializer serializer];
        
    [request XSRFPOST:urlAsString parameters:message
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            if(responseObject != nil)
            {
                BOOL success = [responseObject unsignedValueForKey:@"success"] != 0;
                if(success)
                    handler(YES, nil);
                else
                    handler(NO, [responseObject stringForKey:@"message"]);
            }
            else
            {
                handler(NO, NSLocalizedString(@"UnknownError", nil));
            }
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO, NSLocalizedString(@"UnknownError", nil));
        }];
}

/* archiveMessage: Sends a message to the archived folder
 */
+ (void) archiveMessage:(NSUInteger)messageID
             completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@messages/api/archive-messages?messageIds[0]=%lu",
                             [RobloxInfo getBaseUrl],
                             (unsigned long)messageID];
    
    [self hitEndpoint:urlAsString
         isGetRequest:NO
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         //the response object is an array of failed archived messages, a zero count array means success
         NSArray* response = responseObject;
         handler(response && response.count == 0);
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(false);
     }];
}

/* unarchiveMessage: Removes a message from the archived folder
 */
+ (void) unarchiveMessage:(NSUInteger)messageID
               completion:(void(^)(BOOL success))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@messages/api/unarchive-messages?messageIds[0]=%lu",
                             [RobloxInfo getBaseUrl],
                             (unsigned long)messageID];
    
    [self hitEndpoint:urlAsString
         isGetRequest:NO
        handleSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         //the response object is an array of failed archived messages, a zero count array means success
         NSArray* response = responseObject;
         handler(response && response.count == 0);
     }
        handleFailure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         handler(false);
     }];
}


/* purchaseProduct: Purchase any product by its product ID
 */
+ (void) purchaseProduct:(NSUInteger)productID
            currencyType:(RBXCurrencyType)currencyType
           purchasePrice:(NSUInteger)purchasePrice
              completion:(void(^)(BOOL success, NSString* errorMessage))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/marketplace/purchase-from-anywhere"
                                                       @"?productId=%lu"
                                                       @"&currencyTypeId=%lu"
                                                       @"&purchasePrice=%lu",
                                [RobloxInfo getApiBaseUrl],
                                (unsigned long)productID,
                                (unsigned long)currencyType,
                                (unsigned long)purchasePrice];

    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    request.requestSerializer = [AFJSONRequestSerializer serializer];
        
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            if(responseObject != nil)
            {
                BOOL success = [responseObject unsignedValueForKey:@"success"] != 0;
                if(success)
                    handler(YES, nil);
                else
                    handler(NO, [responseObject stringForKey:@"message"]);
            }
            else
            {
                handler(NO, NSLocalizedString(@"UnknownError", nil));
            }
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO, NSLocalizedString(@"UnknownError", nil));
        }];
}

/* voteForAssetID: Vote for item
 */
+ (void) voteForAssetID:(NSString*)assetID
           votePositive:(BOOL)votePositive
             completion:(void(^)(BOOL success, NSString* message, NSUInteger totalPositives, NSUInteger totalNegatives, RBXUserVote userVote))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/voting/vote?assetId=%@&vote=%@",
                             [RobloxInfo getBaseUrl],
                             assetID,
                             votePositive ? @"true" : @"false"];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    request.requestSerializer = [AFJSONRequestSerializer serializer];
    
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            if(responseObject != nil)
            {
                BOOL success = [responseObject[@"Success"] boolValue];
                if(success)
                {
                    NSDictionary* model = [responseObject dictionaryForKey:@"Model"];
                    
                    NSUInteger upVotes = [model unsignedValueForKey:@"UpVotes"];
                    NSUInteger downVotes = [model unsignedValueForKey:@"DownVotes"];
                    RBXUserVote userVote;
                    
                    id responseUserVote = [model objectForKey:@"UserVote"];
                    if(responseUserVote != nil && [responseUserVote isKindOfClass:[NSNumber class]] )
                        userVote = [responseUserVote boolValue] == YES ? RBXUserVotePositive : RBXUserVoteNegative;
                    else
                        userVote = RBXUserVoteNoVote;
                    
                    handler(YES, nil, upVotes, downVotes, userVote);
                }
                else
                {
                    NSString* message = nil;
                    
                    NSString* modalType = [responseObject stringForKey:@"ModalType"];
                    if( [modalType isEqualToString:@"FloodCheckThresholdMet"] )
                    {
                        message = NSLocalizedString(@"VotingTooQuicklyPhrase", nil);
                    }
                    else if( [modalType isEqualToString:@"PlayGame"] )
                    {
                        message = NSLocalizedString(@"VotingGameNotPlayed", nil);
                    }
                    
                    if( message == nil )
                    {
                        message = [responseObject stringForKey:@"Message" withDefault:nil];
                    }
                    
                    if( message != nil )
                    {
                        handler(NO, message, 0, 0, RBXUserVoteNoVote);
                    }
                    else
                    {
                        handler(NO, NSLocalizedString(@"UnknownError", nil), 0, 0, RBXUserVoteNoVote);
                    }
                }
            }
            else
            {
                handler(NO, NSLocalizedString(@"UnknownError", nil), 0, 0, RBXUserVoteNoVote);
            }
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO, NSLocalizedString(@"UnknownError", nil), 0, 0, RBXUserVoteNoVote);
        }];
}

/* favoriteToggleForAssetID: Toggle favorite for item
 */
+ (void) favoriteToggleForAssetID:(NSString*)assetID
                       completion:(void(^)(BOOL success, NSString* message))handler
{
    NSString* urlAsString = [NSString stringWithFormat:@"%@/favorite/toggle?assetId=%@",
                             [RobloxInfo getBaseUrl],
                             assetID];
    
    RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
    request.requestSerializer = [AFJSONRequestSerializer serializer];
    
    [request XSRFPOST:urlAsString parameters:nil
        success:^(AFHTTPRequestOperation *operation, id responseObject)
        {
            if(responseObject != nil)
            {
                BOOL success = [responseObject boolValueForKey:@"success"];
                if(success)
                    handler(YES, nil);
                else
                {
                    NSString* message = [responseObject stringForKey:@"message" withDefault:nil];
                    if(message != nil)
                        handler(NO, message);
                    else
                        handler(NO, NSLocalizedString(@"UnknownError", nil));
                }
            }
            else
            {
                handler(NO, NSLocalizedString(@"UnknownError", nil));
            }
        }
        failure:^(AFHTTPRequestOperation *operation, NSError *error)
        {
            handler(NO, NSLocalizedString(@"UnknownError", nil));
        }];
}


/* Helper Function - Hit Endpoint
 */
+ (NSData*) hitEndpointSynchronous:(NSString*)urlAsString
                      isGetRequest:(BOOL) isGET
                          response:(NSURLResponse**)response
                             error:(NSError **) responseError
{
    //build the url
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:urlAsString]];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    [RobloxInfo setDefaultHTTPHeadersForRequest:request];
    
    [request setHTTPMethod:(isGET) ? @"GET" : @"POST"];
    
    //response can be nil
    return [NSURLConnection sendSynchronousRequest:request returningResponse:response error:responseError];;
}
+ (void) hitEndpoint:(NSString*)urlAsString
        isGetRequest:(BOOL)isGET
       handleSuccess:(void(^)(AFHTTPRequestOperation* operation, id responseObject))successfulOperation
       handleFailure:(void(^)(AFHTTPRequestOperation* operation, NSError* error))failedOperation
{
    //build the request object
    if (isGET)
    {
        NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:[[NSURL alloc] initWithString:urlAsString]];
        [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
        [request setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];
        [RobloxInfo setDefaultHTTPHeadersForRequest:request];

        AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
        operation.responseSerializer = [AFJSONResponseSerializer serializer];
        operation.completionQueue = _completionQueue;
        
        [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
         {
             if (successfulOperation)
                 successfulOperation(operation, responseObject);
         }
         failure:^(AFHTTPRequestOperation *operation, NSError *error)
         {
             if (failedOperation)
                 failedOperation(operation, error);
         }];
        
        //fire the request
        NSOperationQueue* queue = [[NSOperationQueue alloc] init];
        [queue addOperation:operation];
    }
    else
    {
        RobloxXSRFRequest* request = [[RobloxXSRFRequest alloc] initWithBaseURL:nil];
        request.requestSerializer = [AFJSONRequestSerializer serializer];
        
        [request XSRFPOST:urlAsString parameters:nil success:^(AFHTTPRequestOperation *operation, id responseObject)
         {
             if (successfulOperation)
                 successfulOperation(operation, responseObject);
         }
         failure:^(AFHTTPRequestOperation *operation, NSError *error)
         {
             if (failedOperation)
                 failedOperation(operation, error);
         }];
        
    }
}
 
@end
