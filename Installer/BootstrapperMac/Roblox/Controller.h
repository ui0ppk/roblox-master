//
//  Controller.h
//  Roblox
//
//  Created by Dharmendra on 14/01/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "InstallSuccessController.h"

struct BootstrapperSettings;

@interface Controller : NSObject 
{
	IBOutlet id      progressPanel;
    IBOutlet id      progressIndicator;
    IBOutlet id      progressField;
    IBOutlet id      progressPanelImage;
    IBOutlet id      progressPanelButton;
    
    NSURLDownload   *updateDownload;
    NSURLResponse   *downloadResponse;
	
	NSString        *serverVersion;
    NSString        *serverStudioVersion;
    
	NSString        *baseServer;
	NSString        *setupServer;
    
    NSString        *browserTrackerId;
	
	NSString        *tempDownloadDir;
	NSString        *tempBackupDir;
    
    NSString        *urlHandlerString;
    NSMutableArray  *urlHandlerArguments;
	
	NSPanel         *updatePanel;
	NSTextField     *updateTextField;
	NSTimer         *updateTimer;
    
    struct BootstrapperSettings *bootstrapperSettings;
    InstallSuccessController *installSuccessController;
	
	unsigned  int    downloadBytes;
	unsigned  int    reportStatGuid;
	
	
	int              ticks;
	int              parentProcessId;
    
	BOOL             isInstalling;
	BOOL             isDownloadingBootstrapper;
	BOOL			 isDownloadingStudioBootstrapper;
	BOOL             launchedFromReadOnlyFS;
	
	BOOL             isClientAppChanged;
	
	BOOL             installSuccess;
	
	BOOL             isEmbedded;
	BOOL             justEmbedded;
	
	BOOL             isReportStatEnabled;
	BOOL             isAdminUser;
	BOOL			 isMac10_5;
    
    NSTimer         *semaphoreCleanupTimer;
    
    int              influxDbThrottle;
    NSDate          *startTime;
}

- (void) initData;
- (void) cleanUp;

- (NSString *) osVersion;
- (void) verifyVersion;

/* NSApplication Delegate Methods */
- (void) applicationDidFinishLaunching:(NSNotification*) aNotification;
- (void) applicationWillFinishLaunching:(NSNotification*) aNotification;

/* URL management*/
- (void) handleURLEvent:(NSAppleEventDescriptor*)event withReplyEvent:(NSAppleEventDescriptor*)replyEvent;

/* Command line options handling */
- (BOOL) handleCommandLineOptions;
- (BOOL) handleCheckOption;

/* Protocol options handling*/
- (BOOL) saveProtocolArguments;

- (void) initUpdateData;
- (void) onTimerTick:(NSTimer *)timer;
- (void) onUpdatePanelButtonClicked:(id)sender;

/* App Management Methods*/
- (void) updateApplications;
- (void) downloadClientApplication;
- (void) downloadBootstrapper;

- (void) downloadBootstrapperDidFinish;
- (void) downloadStudioBootstrapperDidFinish;
- (void) downloadClientApplicationDidFinish;

/* Download Management Methods */
- (BOOL) startDownload:(NSString *)source destination:(NSString *)destination;
- (void) endUpdateDownload;

/* NSURLDownload Delegate Methods */
- (void) download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response;
- (void) download:(NSURLDownload *)download didReceiveDataOfLength:(unsigned)length;
- (void) downloadDidFinish:(NSURLDownload *)download;
- (void) download:(NSURLDownload *)download didFailWithError:(NSError *)error;

/* Action Methods */
- (void) cancelUpdate:(id)sender;
- (IBAction)onButtonClicked:(CustomImageButton *)sender;
#ifndef TARGET_IS_STUDIO
- (void) installDialogClosing:(id)sender;
#endif

- (BOOL) deployClientApplication;
- (BOOL) deployBootstrapper;
- (BOOL) deployStudioBootstrapper;
- (void) downloadStudioBootstrapper;
- (BOOL) deployNPAPIPlugin;

- (void) unInstallNPAPIPlugin:(NSString *)pluginFolder;


- (BOOL) applicationExistsInDock:(NSString*)path checkLink:(BOOL)checkLink;
- (BOOL) addApplicationInDock:(NSString*)path;

- (void) relaunchApplication:(NSString *)appPath extraArguments:(NSArray *)extraArguments;

/*Analytic methods*/
- (void) setupInfluxDb;
- (void) reportToInfluxDb:(int)throttle json:(NSString*)json;
- (void) reportInstallMetricsToInfluxDb:(BOOL)isUpdate result:(NSString*)result message:(NSString*)message;

/*Utility methods*/

- (BOOL) isStudioInstalled;
- (NSString *) getStudioDirectory;
- (NSString *) getStudioApp;

- (BOOL) relauchRequired;
- (BOOL) isVersionChanged;
- (BOOL) isBootstrapperUpdateRequired;
- (BOOL) isNPAPIPluginDeploymentRequired;

- (BOOL) backupBootstrapper;
- (BOOL) embedBootstrapper;

- (NSString *) getURL:(NSString *)fileName;
- (NSString *) getStudioURL:(NSString *)fileName;
- (NSString *) getBootstrapperContentPath:(NSString *)fileName;
- (NSString *) getTempPath:(NSString *)fileName;
- (NSString *) getTempDownloadDir;
- (NSString *) getTempBackupPath:(NSString *)fileName;
- (NSString *) getTempBackupDir;

- (NSString *) getClientApplication;
- (NSString *) getBootstrapperApplication;
- (NSString *) getRelaunchDaemon;
- (NSString *) getBootstrapperPathToEmbed;

- (NSString *) stringWithContentsOfURL:(NSString *)urlString;

- (void) saveUpdateInfo:(BOOL)saveVersionInfo;
- (NSString *) getValueFromKey:(NSString *)keyToSearch;

- (void) modifyClientInfoList;

- (BOOL) unzipFile:(NSString *)zipFilePath unzipDestination:(NSString *)unzipDestination;

- (BOOL) copyDirectory:(NSString *)directoryName source:(NSString *)source destination:(NSString *)destination;
- (BOOL) renameDirectory:(NSString *)directoryName directoryPath:(NSString *)directoryPath;
- (BOOL) createDirectory:directoryPath;
- (BOOL) removeDirectory:directoryPath;
- (void) removeTemporaryDirectory;

- (void) showProgressDialog;

- (void) reportStat:(NSString *)requestId;
- (void) reportStat:(NSString *)requestId requestMessage:(NSString *)requestMessage;

- (void) exitApplication;

@end

@interface HttpRequestHandler : NSObject
{
	NSURLConnection *serverConnection;
}

+ (void) doPostAsync:(NSArray *)requestData;

- (void) dealloc;

- (void) doPost:(NSURL *)serverURL dataToPost:(NSData *)dataToPost contentType:(NSString*)contentType;

- (void) connection:(NSURLConnection *)connection didFailWithError:(NSError *)error;
- (void) connectionDidFinishLoading:(NSURLConnection *)connection;

@end
