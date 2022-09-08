//
//  RobloxData.h
//  NativeShell-iOS
//
//  Created by alichtin on 4/25/14.
//  Copyright (c) 2014 Roblox. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CGGeometry.h>

#define BROWSER_TRACKER_KEY @"BrowserTracker"
#define BROWSER_TRACKER_KEY_OLD @"ABTestBrowserTracker"

typedef NS_ENUM(NSInteger, RBXGameSortType)
{
    // 2015.0904 ajain - IDs updated against the prod web site today
    RBXGameSortTypeDefault = -999,

    RBXGameSortTypeUnknown = 0,
    RBXGameSortTypePopular = 1,
    RBXGameSortTypeTopFavorite = 2,
    RBXGameSortTypeFeatured = 3,
    RBXGameSortTypeMyFavorite = 5,
    RBXGameSortTypeMyRecent = 6,
    RBXGameSortTypeTopEarning = 8,
    RBXGameSortTypeTopPaid = 9,
    RBXGameSortTypePurchased = 10,
    RBXGameSortTypeTopRated = 11,
    RBXGameSortTypeBuildersClub = 14,
    RBXGameSortTypePersonalServer = 15,
    RBXGameSortTypeRecommended = 16,
    RBXGameSortTypeFriendActivity = 17
};

typedef NS_ENUM(NSUInteger, RBXLeaderboardsTarget)
{
    RBXLeaderboardsTargetUser = 0,
    RBXLeaderboardsTargetClan = 1
};

typedef NS_ENUM(NSUInteger, RBXLeaderboardsTimeFilter)
{
    RBXLeaderboardsTimeFilterDay = 0,
    RBXLeaderboardsTimeFilterWeek = 1,
    RBXLeaderboardsTimeFilterMonth = 2,
    RBXLeaderboardsTimeFilterAllTime = 3
};

typedef NS_ENUM(NSUInteger, RBXUserBadgeType)
{
    RBXUserBadgeTypePlayer = 0,
    RBXUserBadgeTypeRoblox = 1
};

typedef NS_ENUM(NSUInteger, RBXMessageType)
{
    RBXMessageTypeInbox         = 0,
    RBXMessageTypeSent          = 1,
    RBXMessageTypeArchive       = 3,
    RBXMessageTypeAnnouncements = 4
};

typedef NS_ENUM(NSUInteger, RBXFriendType)
{
    RBXFriendTypeAllFriends    = 0,
    RBXFriendTypeRegularFriend = 1,
    RBXFriendTypeBestFriend    = 2,
    RBXFriendTypeFriendRequest = 3
};

typedef NS_ENUM(NSUInteger, RBXFriendshipStatus)
{
    RBXFriendshipStatusNonFriends      = 0,
    RBXFriendshipStatusRequestSent     = 1,
    RBXFriendshipStatusRequestReceived = 2,
    RBXFriendshipStatusFriends         = 3,
    RBXFriendshipStatusBestFriends     = 4
};

typedef NS_ENUM(NSUInteger, RBXCurrencyType)
{
    RBXCurrencyTypeRobux   = 1,
    RBXCurrencyTypeTickets = 2
};

typedef NS_ENUM(NSUInteger, RBXUserVote)
{
    RBXUserVoteNoVote   = 0,
    RBXUserVotePositive = 1,
    RBXUserVoteNegative = 2
};

typedef NS_ENUM(NSUInteger, RBXThumbnailType)
{
    RBXThumbnailImage = 1,
    RBXThumbnailVideo = 33
};

typedef NS_ENUM(NSUInteger, RBXLocationType)
{
    RBXLocationMobileWebsite = 0,
    RBXLocationIOS = 1,
    RBXLocationPage = 2,
    RBXLocationStudio = 3,
    RBXLocationGame = 4
};

typedef NS_ENUM(NSInteger, SearchResultType)
{
    SearchResultGames = 1,
    SearchResultUsers = 2,
    SearchResultCatalog = 3,
    SearchResultGroups = 4
};

typedef NS_ENUM(NSInteger, ABTestSubjectType)
{
    ABTestSubjectTypeUser = 1,
    ABTestSubjectTypeBrowserTracker = 2
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
@interface RBXGameAttribute : NSObject

    @property (strong, nonatomic) NSString* title;

@end

@interface RBXGameSort : RBXGameAttribute
    +(RBXGameSort*) GameSortWithJSON:(NSDictionary*)json;
    +(RBXGameSort*) MyGamesSort;
    +(RBXGameSort*) RecentlyPlayedGamesSort;
    +(RBXGameSort*) FavoriteGamesSort;
    +(RBXGameSort*) DefaultSort;

    @property (strong, nonatomic) NSNumber* sortID;
@end

@interface RBXGameGenre : RBXGameAttribute
    +(NSArray*) allGenres;

    @property (strong, nonatomic) NSNumber* genreID;
@end

@interface RBXGameData : NSObject
    @property(strong, nonatomic) NSString* title;
    @property(strong, nonatomic) NSString* creatorName;
    @property(strong, nonatomic) NSNumber* creatorID;
    @property(strong, nonatomic) NSString* players;
    @property(strong, nonatomic) NSString* placeID;
    @property(strong, nonatomic) NSNumber* universeID;
    @property(strong, nonatomic) NSString* dateCreated;
    @property(strong, nonatomic) NSString* dateUpdated;
    @property(strong, nonatomic) NSString* gameDescription;
    @property(strong, nonatomic) NSString* thumbnailURL;
    @property(nonatomic) BOOL thumbnailIsFinal;
    @property(nonatomic) NSUInteger favorited;
    @property(nonatomic) NSUInteger visited;
    @property(nonatomic) NSUInteger maxPlayers;
    @property(nonatomic) NSUInteger onlineCount;
    @property(nonatomic) NSUInteger upVotes;
    @property(nonatomic) NSUInteger downVotes;
    @property(nonatomic) RBXUserVote userVote;
    @property(nonatomic) NSUInteger productID;
    @property(nonatomic) NSUInteger price;
    @property(nonatomic) BOOL userOwns;
    @property(nonatomic) BOOL userFavorited;
    @property(strong, nonatomic) NSString* assetGenre;

    + (RBXGameData*) parseGameFromJSON:(NSDictionary*)jsonGame;
@end

@interface RBXLeaderboardEntry : NSObject
    @property(nonatomic) NSUInteger rank;
	@property(strong, nonatomic) NSString* displayRank;
    @property(nonatomic) NSUInteger userID;
    @property(strong, nonatomic) NSString* name;
    @property(nonatomic) NSUInteger targetID;
    @property(strong, nonatomic) NSString* clanName;
    @property(nonatomic) NSUInteger points;
    @property(strong, nonatomic) NSString* displayPoints;
    @property(nonatomic) NSUInteger clanEmblemID;
	@property(strong, nonatomic) NSString* userAvatarURL;
    @property(nonatomic) BOOL userAvatarIsFinal;
	@property(strong, nonatomic) NSString* clanAvatarURL;
    @property(nonatomic) BOOL clanAvatarIsFinal;

    + (RBXLeaderboardEntry*) parseLeaderboardEntry:(NSDictionary*)jsonEntry;
@end

@interface RBXUserProfileInfo : NSObject
	@property(strong, nonatomic) NSNumber* userID;
	@property(strong, nonatomic) NSString* username;
	@property(strong, nonatomic) NSString* avatarURL;
    @property(nonatomic) BOOL avatarIsFinal;
	@property(nonatomic) BOOL BC;
	@property(nonatomic) BOOL TBC;
	@property(nonatomic) BOOL OBC;
    @property(nonatomic) NSUInteger robux;
    @property(nonatomic) NSUInteger tickets;
    @property(nonatomic) RBXFriendshipStatus friendshipStatus;
    @property(nonatomic) BOOL isFollowing;

    +(RBXUserProfileInfo*) UserProfileInfoWithJSON:(NSDictionary*)json;
@end

@interface RBXUserAccount : NSObject
    @property (strong, nonatomic) NSString* userName;
    @property (strong, nonatomic) NSString* email;
    //@property (strong, nonatomic) NSNumber* ageBracket;
    @property (strong, nonatomic) NSNumber* userId;
    @property BOOL isEmailVerfied;
    @property BOOL userAbove13;


    +(RBXUserAccount*) UserAccountWithJSON:(NSDictionary*)json;
@end

@interface RBXUserAccountNotifications : NSObject
    @property int   total;
    @property BOOL  emailNotificationEnabled;
    @property BOOL  passwordNotificationEnabled;

    +(RBXUserAccountNotifications*) UserAccountNotificationsWithJSON:(NSDictionary*)json;
@end

@interface RBXUserPresence : NSObject
    @property (strong, nonatomic) NSString* gameID;
    @property (nonatomic) BOOL isOnline;
    @property (strong, nonatomic) NSString* lastOnlineTime;
    @property (strong, nonatomic) NSString* lastOnlineLocation;
    @property (nonatomic) NSUInteger* locationType;
    @property (nonatomic) NSUInteger* placeID;

    +(RBXUserPresence*) UserPresenceWithJSON:(NSDictionary*)json;
@end

@interface RBXUserSearchInfo : NSObject
    @property (strong, nonatomic) NSString* avatarURL;
    @property (nonatomic) BOOL avatarIsFinal;
    @property (strong, nonatomic) NSNumber* userId;
    @property (strong, nonatomic) NSString* userName;
    @property (strong, nonatomic) NSString* blurb;
    @property (strong, nonatomic) NSString* previousNames;
    @property (nonatomic) BOOL isOnline;
    @property (strong, nonatomic) NSString* lastLocation;
    @property (strong, nonatomic) NSString* profilePageURL;
    @property (strong, nonatomic) NSString* lastSeenDate;

    +(RBXUserSearchInfo*) UserSearchInfoWithJSON:(NSDictionary*)json;
@end

@interface RBXFollower : NSObject
    @property (strong, nonatomic) NSNumber* userId;
    @property (strong, nonatomic) NSString* username;
    @property (strong,nonatomic) NSString* avatarURL;
    @property (nonatomic) BOOL avatarIsFinal;
    @property (nonatomic) BOOL isOnline;
    +(RBXFollower*) FollowerWithJSON:(NSDictionary*)json;
@end

@interface RBXFriendInfo : NSObject
    @property(strong, nonatomic) NSNumber* userID;
    @property(strong, nonatomic) NSString* username;
    @property(strong, nonatomic) NSString* avatarURL;
    @property(nonatomic) BOOL avatarIsFinal;
    @property(nonatomic) NSUInteger invitationID;
    @property(nonatomic) RBXFriendshipStatus friendshipStatus;
    @property(nonatomic) BOOL isOnline;

    +(RBXFriendInfo*) FriendInfoWithJSON:(NSDictionary*)json;
    +(RBXFriendInfo*) FriendInfoFromFollower:(RBXFollower*)follower;
@end

@interface RBXBadgeInfo : NSObject
    @property BOOL isImageURLFinal;
    @property BOOL isOwned;
    @property double badgeRarity;
    @property(strong, nonatomic) NSString* badgeAssetId;
    @property(strong, nonatomic) NSString* imageURL;
    @property(strong, nonatomic) NSString* name;
    @property(strong, nonatomic) NSString* badgeDescription;
    @property(strong, nonatomic) NSString* badgeCreatorId;
    @property(strong, nonatomic) NSString* badgeCreatedDate;
    @property(strong, nonatomic) NSString* badgeUpdatedDate;
    @property(strong, nonatomic) NSString* badgeRarityName;
    @property(strong, nonatomic) NSString* badgeTotalAwardedYesterday;
    @property(strong, nonatomic) NSString* badgeTotalAwardedEver;

    +(RBXBadgeInfo*) parseGameBadgeFromJSON:(NSDictionary*)json;
@end

@interface RBXMessageInfo : NSObject
    @property(strong, nonatomic) NSNumber* senderUserID;
    @property(strong, nonatomic) NSString* senderUsername;
    @property(strong, nonatomic) NSNumber* recipientUserID;
    @property(strong, nonatomic) NSString* recipientUsername;
    @property(nonatomic) NSUInteger messageID;
    @property(strong, nonatomic) NSString* date;
    @property(strong, nonatomic) NSString* subject;
    @property(strong, nonatomic) NSString* body;
    @property(strong, nonatomic) NSString* message;
    @property(nonatomic) BOOL isRead;

    +(RBXMessageInfo*) MessageWithJSON:(NSDictionary*)json;
    +(NSMutableArray*) ArrayFromJSON:(NSDictionary*)json;
@end

@interface RBXGameGear : NSObject
    @property(strong, nonatomic) NSNumber* productID;
    @property(strong, nonatomic) NSNumber* assetID;
    @property(strong, nonatomic) NSString* name;
    @property(strong, nonatomic) NSString* gearDescription;
    @property(nonatomic) NSUInteger priceInRobux;
    @property(nonatomic) NSUInteger priceInTickets;
    @property(strong, nonatomic) NSNumber* sellerID;
    @property(strong, nonatomic) NSString* sellerName;
    @property(nonatomic) NSUInteger favoriteCount;
    @property(nonatomic) BOOL userFavorited;
    @property(nonatomic) NSUInteger totalSales;
    @property(nonatomic) BOOL userOwns;
    @property(nonatomic) BOOL isRentable;
    @property(strong, nonatomic) NSNumber* promotionID;
    @property(nonatomic) BOOL isForSale;
@end

@interface RBXGamePass : NSObject
    @property(strong, nonatomic) NSNumber* productID;
    @property(strong, nonatomic) NSNumber* passID;
    @property(strong, nonatomic) NSString* passName;
    @property(strong, nonatomic) NSString* passDescription;
    @property(nonatomic) NSUInteger totalSales;
    @property(nonatomic) NSUInteger priceInRobux;
    @property(nonatomic) NSUInteger priceInTickets;
    @property(nonatomic) NSUInteger upVotes;
    @property(nonatomic) NSUInteger downVotes;
    @property(nonatomic) NSUInteger favoriteCount;
    @property(nonatomic) BOOL userFavorited;
    @property(nonatomic) RBXUserVote userVote;
    @property(nonatomic) BOOL userOwns;
@end

@interface RBXThumbnail : NSObject
    @property bool assetIsFinal;
    @property NSUInteger assetTypeId;
    @property(strong, nonatomic) NSString* assetId;
    @property(strong, nonatomic) NSString* assetHash;
    @property(strong, nonatomic) NSString* assetURL;

    +(RBXThumbnail*) ThumbnailWithJson:(NSDictionary*)json;
@end

@interface RBXNotifications : NSObject
    @property NSInteger unreadMessages;
    @property NSInteger unansweredFriendRequests;

    +(RBXNotifications*) NotificationsWithJson:(NSDictionary*)json;
@end

@interface RBXSponsoredEvent : NSObject
    @property (strong, nonatomic) NSString* eventName;
    @property (strong, nonatomic) NSString* eventTitle;
    @property (strong, nonatomic) NSString* eventLogoURLString;
    @property (strong, nonatomic) NSString* eventPageType;
    @property (strong, nonatomic) NSString* eventPageURLExtension;

    +(RBXSponsoredEvent*) SponsoredEventWithJson:(NSDictionary*)json;
    +(NSMutableArray*) EventsWithJson:(NSArray*)jsonArray;
@end

@interface RBXABTest : NSObject <NSCoding>
    @property (strong, nonatomic) NSString* experimentName;
    @property (strong, nonatomic) NSString* status;
    @property (strong, nonatomic) NSNumber* subjectTargetId;
    @property int subjectType;
    @property int variation;
    @property (nonatomic, readonly) BOOL isLockedOn;

    +(RBXABTest*) TestWithJSON:(NSDictionary *)json;
@end

////////////////////////////////////////////////////////////////////////////////////////////////////////////////


@interface RobloxData : NSObject

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Images API
//

/* fetchURLForAssetID: Retrieve a game image url once it is final
 */
+ (void) fetchURLForAssetID:(NSString*)placeID
                   withSize:(CGSize)size
                 completion:(void(^)(NSString* imageURL))handler;

/* fetchThumbnailURLForGameID: Retrieve a square game image url once it is final
 */
+ (void) fetchThumbnailURLForGameID:(NSString*)assetID
                           withSize:(CGSize)size
                         completion:(void(^)(NSString* imageURL))handler;

/* fetchAvatarURLForUserID: Retrieve a user avatar url once it is final
 */
+ (void) fetchAvatarURLForUserID:(NSUInteger)userID
                        withSize:(CGSize)size
                      completion:(void(^)(NSString* imageURL))handler;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Analytics Event Reporting
//
+ (void) initializeBrowserTrackerWithCompletion:(void(^)(bool success, NSString* browserTracker))handler;
+ (NSString*) browserTrackerId;
+ (void) setBrowserTrackerId:(NSString *)trackerId;
+ (void) reportEvent:(NSString*)event
      withCompletion:(void(^)(bool success, NSError* err))completion;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AB TEST and Tutorial APIs
//
+ (NSDictionary*) fetchABTestEnrollments:(NSString*)experimentalData;
+ (NSDictionary*) enrollInABTests:(NSString*)experimentalData;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Force Upgrade
//
+ (void) checkForForceUpgrade:(NSString*)versionNumber
               withCompletion:(void(^)(NSDictionary* response))handler;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sign Up and Log In APIs
//
/* checkIfValidUsername: asks the server if a provided username is acceptable
 */
+(void) oldCheckIfValidUsername:(NSString*)username
                     completion:(void(^)(BOOL success, NSString* message))handler;
+(void) checkIfValidUsername:(NSString*)username
                  completion:(void(^)(BOOL success, NSString* message))handler;

/* checkIfBlacklistedEmail: asks the server if a provided email is acceptable
 */
+(void) checkIfBlacklistedEmail:(NSString*)email
                  completion:(void(^)(BOOL isBlacklisted))handler;

/* recommendUsername: asks the server for an alternative name to an already taken username
 */
+(void) recommendUsername:(NSString*)usernameToTry
               completion:(void(^)(BOOL success, NSString* newUsername))handler;

/* checkIfValidPassword: asks the server if a password is acceptable
 */
+(void) oldCheckIfValidPassword:(NSString*)password
                       username:(NSString*)username
                     completion:(void(^)(BOOL success, NSString* message))handler;
+(void) checkIfValidPassword:(NSString*)password
                    username:(NSString*)username
                  completion:(void(^)(BOOL success, NSString* message))handler;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Games API
//

/* fetchDefaultSorts: Retrieves the selected sorts to display in the main page
 */
+ (void) fetchDefaultSorts:(NSUInteger)minimumGamesInSort
                completion:(void(^)(NSArray* sorts)) handler;

/* fetchAllSorts: Retrieves the all the sorts
 */
+ (void) fetchAllSorts:(void(^)(NSArray* sorts)) completionHandler;

/* fetchGameList: Retrieves a list of games from the server
 */
+ (void) fetchGameListWithSortID:(NSNumber*)sortID
                         genreID:(NSNumber*)genreID
                       fromIndex:(NSUInteger)startIndex
                        numGames:(NSUInteger)numGames
                       thumbSize:(CGSize)thumbSize
                      completion:(void (^)(NSArray* games)) handler;

+ (void) fetchGameListWithSortID:(NSNumber*)sortID
                         genreID:(NSNumber*)genreID
                        playerID:(NSNumber*)playerID
                       fromIndex:(NSUInteger)startIndex
                        numGames:(NSUInteger)numGames
                       thumbSize:(CGSize)thumbSize
                      completion:(void (^)(NSArray* games)) handler;

/* fetchUserPlaces: Retrieves the user places
 */
+ (void) fetchUserPlaces:(NSNumber*)userID
              startIndex:(NSUInteger)startIndex
                numGames:(NSUInteger)numGames
               thumbSize:(CGSize)thumbSize
              completion:(void(^)(NSArray* games)) completionHandler;

/* fetchThumbnailsForPlace: Retrieves an array of image URLs for a given game
 */
+ (void) fetchThumbnailsForPlace:(NSString*)placeID
                        withSize:(CGSize)size
                      completion:(void(^)(NSArray* thumbnails))handler;

/* fetchUserFavoriteGames: Retrieves the user favorite games
 */
+ (void) fetchUserFavoriteGames:(NSNumber*)userID
                     startIndex:(NSUInteger)startIndex
                       numGames:(NSUInteger)numGames
                      thumbSize:(CGSize)thumbSize
                     completion:(void(^)(NSArray* games)) completionHandler;

/* fetchGameDetails: Retrieve full game details
 */
+ (void) fetchGameDetails:(NSString*)placeID
               completion:(void(^)(RBXGameData* game))handler;

/* fetchGameGear: Retrieve game gear
 */
+ (void) fetchGameGear:(NSString*)placeID
            startIndex:(NSUInteger)startIndex
               maxRows:(NSUInteger)maxRows
            completion:(void(^)(NSArray* gear))handler;

/* fetchGamePasses: Retrieve game passes
 */
+ (void) fetchGamePasses:(NSString*)placeID
              startIndex:(NSUInteger)startIndex
                 maxRows:(NSUInteger)maxRows
              completion:(void(^)(NSArray* passes))handler;

/* fetchGameBadges: Retrieve game badges
 */
+ (void) fetchGameBadges:(NSString*)placeID
              completion:(void(^)(NSArray* badges))handler;

/* searchGames: Retrieves a list of games from the server
 */
+ (void) searchGames:(NSString*)keyword
		   fromIndex:(NSUInteger)startIndex
            numGames:(NSUInteger)numGames
           thumbSize:(CGSize)thumbSize
          completion:(void (^)(NSArray* games)) handler;

/* searchUsers: Retrieves a list of users from the server
 */
+ (void) searchUsers:(NSString*)keyword
           fromIndex:(NSUInteger)startIndex
            numUsers:(NSUInteger)numUsers
           thumbSize:(CGSize)thumbSize
          completion:(void (^)(NSArray* users)) handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Leaderboards API
//

/* fetchUserLeaderboardInfo: Retrieve the leaderboard info the the current logged in user.
                             Returns null if there is no session active or there was an error in the request.
 */
+ (void) fetchUserLeaderboardInfo:(RBXLeaderboardsTarget)target
                           userID:(NSNumber*)currentUserID
                    distributorID:(NSNumber*)distributorID
                       timeFilter:(RBXLeaderboardsTimeFilter)time
                       avatarSize:(CGSize)avatarSize
                       completion:(void(^)(RBXLeaderboardEntry* userEntry))handler;

/* fetchLeaderboards: Retrieve leaderboards
 */
+ (void) fetchLeaderboardsFor:(RBXLeaderboardsTarget)target
                   timeFilter:(RBXLeaderboardsTimeFilter)time
                distributorID:(NSNumber*)distributorID
                   startIndex:(NSUInteger)startIndex
                     numItems:(NSUInteger)numItems
                    lastEntry:(RBXLeaderboardEntry*)lastEntry
                   avatarSize:(CGSize)avatarSize
                   completion:(void(^)(NSArray* leaderboard))handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Profile API
//

/* fetchMyProfileWithSize: Retrieve current user profile info
 */
+ (void) fetchMyProfileWithSize:(CGSize)avatarSize
                     completion:(void(^)(RBXUserProfileInfo* profile))handler;

/* fetchUserProfile: Retrieve a user profile info
 */
+ (void) fetchUserProfile:(NSNumber*)userID
               avatarSize:(CGSize)avatarSize
			   completion:(void(^)(RBXUserProfileInfo* profile))handler;

/* fetchUserFriends
 */
+ (void) fetchUserFriends:(NSNumber*)userID
               friendType:(RBXFriendType)friendType
               startIndex:(NSUInteger)startIndex
                 numItems:(NSUInteger)numItems
               avatarSize:(CGSize)avatarSize
               completion:(void(^)(NSUInteger totalFriends, NSArray* friends))handler;

/* fetchUserBadges
 */
+ (void) fetchUserBadges:(RBXUserBadgeType)badgeType
                 forUser:(NSNumber*)userID
               badgeSize:(CGSize)badgeSize
              completion:(void(^)(NSArray* badges))handler;

/* acceptFriendRequest
 */
+ (void) acceptFriendRequest:(NSUInteger)invitationID
            withTargetUserID:(NSUInteger)targetUserID
                  completion:(void(^)(BOOL success))handler;


/* declineFriendRequest
 */
+ (void) declineFriendRequest:(NSUInteger)invitationID
             withTargetUserID:(NSUInteger)targetUserID
                   completion:(void(^)(BOOL success))handler;

/* sendFriendRequest
 */
+ (void) sendFriendRequest:(NSNumber*)userID
                completion:(void(^)(BOOL success))handler;

/* request friendship - API Proxy alternative to sending a friendship request
 */
+ (void) requestFrienshipWithUserID:(NSNumber*)friendUserID
                         completion:(void(^)(BOOL success))handler;

/* removeFriend
 */
+ (void) removeFriend:(NSNumber*)userID
           completion:(void(^)(BOOL success))handler;

/* unfriend - API proxy equivalent to removeFriend
 */
+ (void) unfriend:(NSNumber*)userID
       completion:(void(^)(BOOL success))handler;

/* makeBestFriend - DEPRECATED
 */
+ (void) makeBestFriend:(NSNumber*)userID
            completion:(void(^)(BOOL success))handler;

/* removeBestFriend - DEPRECATED
 */
+ (void) removeBestFriend:(NSNumber*)userID
               completion:(void(^)(BOOL success))handler;

/* followUser
 */
+ (void) followUser:(NSNumber*)userID
         completion:(void(^)(BOOL success))handler;

/* unfollowUser
 */
+ (void) unfollowUser:(NSNumber*)userID
           completion:(void(^)(BOOL success))handler;

/* fetch count of friends
 */
+ (void) fetchFriendCount:(NSNumber*)currentUserId
           withCompletion:(void(^)(BOOL success, NSString* message, int count))handler;

/* fetch a user's followers
 */
+ (void) fetchFollowersForUser:(NSNumber*)userID
                        atPage:(int)userPage
                    avatarSize:(CGSize)avatarSize
                withCompletion:(void(^)(bool success, NSArray* followers))handler;

/* fetch all the users that a user is following
 */
+ (void) fetchMyFollowingAtPage:(int)userPage
                     avatarSize:(CGSize)avatarSize
                withCompletion:(void(^)(bool success, NSArray* following))handler;

/* changeUserPassword
 */
+ (void) changeUserOldPassword:(NSString*)passwordOld
                 toNewPassword:(NSString*)passwordNew
              withConfirmation:(NSString*)passwordConfirm
                 andCompletion:(void(^)(BOOL success, NSString* message))handler;

/* changeUserEmail
 */
+ (void) changeUserEmail:(NSString*)email
            withPassword:(NSString*)password
           andCompletion:(void(^)(BOOL success, NSString* message))handler;

/* fetch My Account
 */
+ (void) fetchMyAccountWithCompletion:(void(^)(RBXUserAccount* account))handler;

/* fetch User Has Set Password
 */
+ (void) fetchUserHasSetPasswordWithCompletion:(void(^)(bool passwordSet))handler;

/* fetch My Account Notifications
 */
+ (void) fetchAccountNotificationsWithCompletion:(void(^)(RBXUserAccountNotifications* notifications))handler;

/* fetch User Presence
 */
+ (void) fetchPresenceForUser:(NSNumber*)userID
               withCompletion:(void(^)(RBXUserPresence*))handler;
//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Messages API
//

/* fetchMessages: Retrieve user messages for the specified message type
 */
+ (void) fetchMessages:(RBXMessageType)messageType
            pageNumber:(NSUInteger)pageNumber
              pageSize:(NSUInteger)pageSize
            completion:(void(^)(NSArray* messages))handler;

/* fetchNotificationsWithCompletion: Retrieve user notifications
 */
+ (void) fetchNotificationsWithCompletion:(void(^)(NSArray* messages))handler;

/* markMessageAsRead: Mark the specified message as read
 */
+ (void) markMessageAsRead:(NSUInteger)messageID completion:(void(^)(BOOL success))handler;

/* fetchTotalNotificationsForUser: WithCompletion: Retrieve a count of unread messages, and outstanding friend requests
 * userID : the UserInfo playerID of the player to retrieve to retrieve the notifications for
 * handler : an asynchronous callback block in which the retreived total notifications are returned
 */
+ (void) fetchTotalNotificationsForUser:(NSNumber*)userID WithCompletion:(void(^)(RBXNotifications* notifications))handler;


/* sendMessage: Send o reply a message to another user
 */
+ (void) sendMessage:(NSString*)body
             subject:(NSString*)subject
         recipientID:(NSNumber*)recipientID
      replyMessageID:(NSNumber*)replyMessageID
          completion:(void(^)(BOOL success, NSString* errorMessage))handler;

/* archiveMessage: Sends a message to the archived folder
 */
+ (void) archiveMessage:(NSUInteger)messageID
          completion:(void(^)(BOOL success))handler;

/* unarchiveMessage: Removes a message from the archived folder
 */
+ (void) unarchiveMessage:(NSUInteger)messageID
             completion:(void(^)(BOOL success))handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// More Page API
//
+ (void) fetchSponsoredEvents:(void(^)(NSArray* events))handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Store API
//

/* purchaseProduct: Purchase any product by its product ID
 */
+ (void) purchaseProduct:(NSUInteger)productID
            currencyType:(RBXCurrencyType)currencyType
           purchasePrice:(NSUInteger)purchasePrice
              completion:(void(^)(BOOL success, NSString* errorMessage))handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// General API
//

/* voteForAssetID: Vote for item
 */
+ (void) voteForAssetID:(NSString*)assetID
           votePositive:(BOOL)votePositive
             completion:(void(^)(BOOL success, NSString* message, NSUInteger totalPositives, NSUInteger totalNegatives, RBXUserVote userVote))handler;

/* favoriteToggleForAssetID: Toggle favorite for item
 */
+ (void) favoriteToggleForAssetID:(NSString*)assetID
                       completion:(void(^)(BOOL success, NSString* message))handler;

//
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@end
