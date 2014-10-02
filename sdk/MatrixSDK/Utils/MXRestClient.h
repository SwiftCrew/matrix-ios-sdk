/*
 Copyright 2014 OpenMarket Ltd
 
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

#import "MXError.h"

/**
 `MXRestClient` is an abstraction layer for making HTTP requests to the home server.
*/
@interface MXRestClient : NSObject

-(id)initWithHomeServer:(NSString*)homeserver;

-(id)initWithHomeServer:(NSString*)homeserver andAccessToken:(NSString*)access_token;

-(id)requestWithMethod:(NSString *)httpMethod
                   path:(NSString *)path
             parameters:(id)parameters
                success:(void (^)(NSDictionary *JSONResponse))success
                failure:(void (^)(MXError *error))failure;
@end
