// 
// Copyright 2020 The Matrix.org Foundation C.I.C
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#import <Foundation/Foundation.h>
#import "MXJSONModel.h"

/**
 Call version
 */
extern NSString *const kMXCallVersion;

/// Base class for event contents of call events.
@interface MXCallEventContent : MXJSONModel

/**
 The version of the VoIP specification this message adheres to. Can be nil. @see `version`.
 */
@property (nonatomic, copy) NSNumber *versionNumber;

/**
 The version of the VoIP specification this message adheres to. Can be nil. @see `version`.
 */
@property (nonatomic, copy) NSString *versionString;

/**
 The party id for the call event.
 */
@property (nonatomic, copy) NSString *partyId;

/// Parse base fields from the JSON
/// @param JSONDictionary The JSON to be parsed
- (void)parseJSON:(NSDictionary *)JSONDictionary;

/// Derived value from versionNumber or versionString
- (NSString *)version;

@end