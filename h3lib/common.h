// Copyright [2019] [FORTH-ICS]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <regex.h>
#include <sys/stat.h>

#include <glib.h>

#include <uuid/uuid.h>

#include "h3lib.h"
#include "h3lib_config.h"
#include "kv_interface.h"


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE !TRUE
#endif

#ifndef UUID_STR_LEN
#define UUID_STR_LEN 36
#endif

#ifndef REG_NOERROR
#define REG_NOERROR 0
#endif

#define H3_PART_SIZE (1048576 * 1) // = 2Mb - Key - 4Kb kreon metadata
#define H3_CHUNK	 (H3_PART_SIZE * 16)
#define H3_SYSTEM_ID    0x00

#define H3_BUCKET_BATCH_SIZE   10
#define H3_PART_BATCH_SIZE   10

#define H3_USERID_SIZE      128
#define H3_MULIPARTID_SIZE  (UUID_STR_LEN + 1)


typedef char H3_UserId[H3_USERID_SIZE+1];
typedef char H3_BucketId[H3_BUCKET_NAME_SIZE+2];
typedef char H3_ObjectId[H3_BUCKET_NAME_SIZE + H3_OBJECT_NAME_SIZE + 1];
typedef char H3_UUID[UUID_STR_LEN];
typedef char H3_PartId[50];                                                 // '_' + UUID[36+1byte] + '#' + <part_number> + ['.' + <subpart_number>]

typedef enum {
    H3_STORE_FILESYSTEM = 0,    // Mounted filesystem
    H3_STORE_KREON,             // Kreon cluster
    H3_STORE_ROCKSDB,           // RocksDB server
    H3_STORE_REDIS,             // Redis
    H3_NumOfStores              // Not an option, used for iteration purposes
} H3_StoreType;

typedef enum {
	MoveReplace,	// Overwrite destination if exists
	MoveNoReplace,	// Do not overwrite destination if exists
	MoveExchange	// Swap data with destination (must exist)
}H3_MovePolicy;

typedef struct {
    H3_StoreType type;

    // Store specific
    KV_Handle handle;
    KV_Operations* operation;
}H3_Context;

typedef struct{
    uint nBuckets;
    H3_BucketId bucket[];
}H3_UserMetadata;

typedef struct{
    H3_UserId userId;
    struct timespec creation;
}H3_BucketMetadata;

typedef struct{
    uint number;
    int subNumber;
    size_t size;
    off_t offset;  // For multipart uploads, the offset is set when the upload completes
}H3_PartMetadata;

typedef struct{
    char isBad;
    H3_UserId userId;
    uuid_t uuid;
    struct timespec creation;
    struct timespec lastAccess;				// Access - the last time the file was read
    struct timespec lastModification;		// Modify - the last time the file was modified (content has been modified)
    struct timespec lastChange;				// Change - the last time meta data of the file was changed (e.g. permissions)
    mode_t mode;
    uid_t uid;
    gid_t gid;
    uint nParts;
    H3_PartMetadata part[];
}H3_ObjectMetadata;

typedef struct{
    H3_UserId userId;
    H3_ObjectId objectId;
}H3_MultipartMetadata;


H3_Status ValidBucketName(KV_Operations* op,char* name);
H3_Status ValidObjectName(KV_Operations* op,char* name);
H3_Status ValidPrefix(KV_Operations* op,char* name);
int GetUserId(H3_Token token, H3_UserId id);
int GetBucketId(H3_Name bucketName, H3_BucketId id);
int GetBucketIndex(H3_UserMetadata* userMetadata, H3_Name bucketName);
void GetObjectId(H3_Name bucketName, H3_Name objectName, H3_ObjectId id);
void GetMultipartObjectId(H3_Name bucketName, H3_Name objectName, H3_ObjectId id);
char* GetBucketFromId(H3_ObjectId objId, H3_BucketId bucketId);
void InitMode(H3_ObjectMetadata* objMeta);
H3_MultipartId GenerateMultipartId(uuid_t uuid);
void CreatePartId(H3_PartId partId, uuid_t uuid, int partNumber, int subPartNumber);
char* PartToId(H3_PartId partId, uuid_t uuid, H3_PartMetadata* part);
int GrantBucketAccess(H3_UserId id, H3_BucketMetadata* meta);
int GrantObjectAccess(H3_UserId id, H3_ObjectMetadata* meta);
int GrantMultipartAccess(H3_UserId id, H3_MultipartMetadata* meta);
char* ConvertToOdrinary(H3_ObjectId id);
H3_Status DeleteObject(H3_Context* ctx, H3_UserId userId, H3_ObjectId objId, char truncate);
KV_Status WriteData(H3_Context* ctx, H3_ObjectMetadata* meta, KV_Value value, size_t size, off_t offset);
KV_Status ReadData(H3_Context* ctx, H3_ObjectMetadata* meta, KV_Value value, size_t* size, off_t offset);
KV_Status CopyData(H3_Context* ctx, H3_UserId userId, H3_ObjectId srcObjId, H3_ObjectId dstObjId, off_t srcOffset, size_t* size, uint8_t noOverwrite, off_t dstOffset);
