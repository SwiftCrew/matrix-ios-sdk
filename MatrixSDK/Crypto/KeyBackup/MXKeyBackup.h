/*
 Copyright 2018 New Vector Ltd

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#import <Foundation/Foundation.h>

#import "MXRestClient.h"
#import "MXMegolmBackupCreationInfo.h"
#import "MXKeyBackupVersionTrust.h"

@class MXSession;
@class OLMPkEncryption;

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Constants definitions

/**
 * E2e keys backup states.
 *
 *                                 |
 *                                 V        deleteKeyBackupVersion (on current backup)
 *    +---------------------->  UNKNOWN  <-------------
 *    |                            |
 *    |                            | checkAndStartKeyBackup (at startup
 *    |                            |         or on new verified device
 *    |                            |         or a new detected backup)
 *    |                            V
 *    |                     CHECKING BACKUP
 *    | Network error              |
 *    |                            |
 *    +<---+------+----------------+-------> DISABLED <----------------------+
 *         |      |                |            |                            |
 *         |      |                |            |                            |
 *         V      |                |            |                            |
 *    BACKUP NOT  |                |            |                            |
 *     TRUSTED    |                |            |                            |
 *                |                |            | createKeyBackupVersion     |
 *                V                |            V                            |
 *           WRONG VERSION         |         ENABLING                        |
 *                ^                |            |                            |
 *                |                V       ok   |     error                  |
 *                |     +------> READY <--------+----------------------------+
 *                |     |          |
 *                |     |          | on new key
 *                |     |          V
 *                |     |     WILL BACK UP (waiting a random duration)
 *                |     |          |
 *                |     |          |
 *                |     | ok       V
 *                |     +----- BACKING UP
 *                |                |
 *                |      Error     |
 *                +<---------------+
 *
 */
typedef enum : NSUInteger
{
    // Need to check the current backup version on the homeserver
    MXKeyBackupStateUnknown = 0,

    // Making the check request on the homeserver
    MXKeyBackupStateCheckingBackUpOnHomeserver,

    // Backup has been stopped because a new backup version has been detected on
    // the homeserver
    MXKeyBackupStateWrongBackUpVersion,

    // Backup from this device is not enabled
    MXKeyBackupStateDisabled,

    // There is a backup available on the homeserver but it is not trusted.
    // It is not trusted because the signature is invalid or the device that created it is not verified
    // Use `trustForKeyBackupVersion` to get trust details.
    // Consequently, the backup from this device is not enabled.
    MXKeyBackupStateNotTrusted,

    // Backup is being enabled
    // The backup version is being created on the homeserver
    MXKeyBackupStateEnabling,

    // Backup is enabled and ready to send backup to the homeserver
    MXKeyBackupStateReadyToBackUp,

    // Backup is going to be send to the homeserver
    MXKeyBackupStateWillBackUp,

    // Backup is being sent to the homeserver
    MXKeyBackupStateBackingUp
} MXKeyBackupState;

/**
 Posted when the state of the MXKeyBackup instance changes.
 */
FOUNDATION_EXPORT NSString *const kMXKeyBackupDidStateChangeNotification;


/**
 A `MXKeyBackup` class instance manage incremental backup of e2e keys (megolm keys)
 to the user's homeserver.
 */
@interface MXKeyBackup : NSObject

#pragma mark - Backup management

/**
 Get information about a backup version defined on the homeserver.

 @param version the backup version.
        nil returns the current backup version. It can be different than `self.keyBackupVersion`.

 @param success A block object called when the operation succeeds.
 @param failure A block object called when the operation fails.

 @return a MXHTTPOperation instance.
 */
- (MXHTTPOperation*)version:(nullable NSString *)version
                    success:(void (^)(MXKeyBackupVersion * _Nullable keyBackupVersion))success
                    failure:(void (^)(NSError *error))failure;

/**
 Check trust on a key backup version.

 @param keyBackupVersion the backup version to check.
 @param onComplete block called when the operations completes.
 */
- (void)trustForKeyBackupVersion:(MXKeyBackupVersion*)keyBackupVersion onComplete:(void (^)(MXKeyBackupVersionTrust *keyBackupVersionTrust))onComplete;

/**
 Set up the data required to create a new backup version.

 The backup version will not be created and enabled until `createKeyBackupVersion`
 is called.
 The returned `MXMegolmBackupCreationInfo` object has a `recoveryKey` member with
 the user-facing recovery key string.

 @param password an optional passphrase string that can be entered by the user
        when restoring the backup as an alternative to entering the recovery key.

 @param success A block object called when the operation succeeds.
 @param failure A block object called when the operation fails
 */
- (void)prepareKeyBackupVersionWithPassword:(nullable NSString *)password
                                    success:(void (^)(MXMegolmBackupCreationInfo *keyBackupCreationInfo))success
                                    failure:(nullable void (^)(NSError *error))failure;

/**
 Create a new key backup version and enable it, using the information return from
`prepareKeyBackupVersion`.

 @param keyBackupCreationInfo the info object from `prepareKeyBackupVersion`.

 @param success A block object called when the operation succeeds.
 @param failure A block object called when the operation fails.

 @return a MXHTTPOperation instance.
 */
- (MXHTTPOperation*)createKeyBackupVersion:(MXMegolmBackupCreationInfo*)keyBackupCreationInfo
                                   success:(void (^)(MXKeyBackupVersion *keyBackupVersion))success
                                   failure:(nullable void (^)(NSError *error))failure;

/**
 Delete a key backup version.

 If we are backing up to this version. Backup will be stopped.

 @param version the backup version to delete.

 @param success A block object called when the operation succeeds.
 @param failure A block object called when the operation fails.

 @return a MXHTTPOperation instance.
 */
- (MXHTTPOperation*)deleteKeyBackupVersion:(NSString*)version
                                   success:(void (^)(void))success
                                   failure:(nullable void (^)(NSError *error))failure;


#pragma mark - Backup storing

/**
 Start to back up keys immediately.

 @param success A block object called when the operation complets.
 @param progress A block object called to indicate operation progress based on number of backed up keys.
 @param failure A block object called when the operation fails.
 */
- (void)backupAllGroupSessions:(nullable void (^)(void))success
                      progress:(nullable void (^)(NSProgress *backupProgress))progress
                       failure:(nullable void (^)(NSError *error))failure;

/**
 Get the current backup progress.

 Can be called at any `MXKeyBackup` state.
 `backupProgress.totalUnitCount` represents the total number of (group sessions) keys.
 `backupProgress.completedUnitCount` is the number of keys already backed up.

 @param backupProgress the current backup progress
 */
- (void)backupProgress:(void (^)(NSProgress *backupProgress))backupProgress;


#pragma mark - Backup restoring

/**
 Check if a key is a valid recovery key.

 @param recoveryKey the string to valid.
 @return YES if valid
 */
+ (BOOL)isValidRecoveryKey:(NSString*)recoveryKey;

/**
 Restore a backup with a recovery key from a given backup version stored on the homeserver.

 @param version the backup version to restore from.
 @param recoveryKey the recovery key to decrypt the retrieved backup.
 @param roomId the id of the room to get backup data from.
 @param sessionId the id of the session to restore.

 @param success A block object called when the operation succeeds.
                It provides the number of found keys and the number of successfully imported keys.
 @param failure A block object called when the operation fails.
 
 @return a MXHTTPOperation instance.
 */
- (MXHTTPOperation*)restoreKeyBackup:(NSString*)version
                     withRecoveryKey:(NSString*)recoveryKey
                                room:(nullable NSString*)roomId
                             session:(nullable NSString*)sessionId
                             success:(nullable void (^)(NSUInteger total, NSUInteger imported))success
                             failure:(nullable void (^)(NSError *error))failure;

/**
 Restore a backup with a password from a given backup version stored on the homeserver.

 @param version the backup version to restore from.
 @param password the password to decrypt the retrieved backup.
 @param roomId the id of the room to get backup data from.
 @param sessionId the id of the session to restore.

 @param success A block object called when the operation succeeds.
 It provides the number of found keys and the number of successfully imported keys.
 @param failure A block object called when the operation fails.

 @return a MXHTTPOperation instance.
 */
- (MXHTTPOperation*)restoreKeyBackup:(NSString*)version
                        withPassword:(NSString*)password
                                room:(nullable NSString*)roomId
                             session:(nullable NSString*)sessionId
                             success:(nullable void (^)(NSUInteger total, NSUInteger imported))success
                             failure:(nullable void (^)(NSError *error))failure;

#pragma mark - Backup state

/**
 The backup state.
 */
@property (nonatomic, readonly) MXKeyBackupState state;

/**
 Indicate if the backup is enabled.
 */
@property (nonatomic, readonly) BOOL enabled;

/**
 The backup version.
 */
@property (nonatomic, readonly, nullable) MXKeyBackupVersion *keyBackupVersion;

/**
 The backup key being used.
 */
@property (nonatomic, readonly, nullable) OLMPkEncryption *backupKey;

@end

NS_ASSUME_NONNULL_END
