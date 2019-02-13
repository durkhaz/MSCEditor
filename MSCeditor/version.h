#pragma once
#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VERSION_MAJOR               1
#define VERSION_MINOR               11

#define VER_FILE_DESCRIPTION_STR    "My Summer Car Save Editor"
#define VER_FILE_VERSION            VERSION_MAJOR, VERSION_MINOR
#define VER_FILE_VERSION_STR        STRINGIZE(VERSION_MAJOR)        \
                                    "." STRINGIZE(VERSION_MINOR)

#define VER_PRODUCTNAME_STR         "MSCEditor"
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   VER_PRODUCTNAME_STR ".exe"
#define VER_INTERNAL_NAME_STR       VER_ORIGINAL_FILENAME_STR
#define VER_COPYRIGHT_STR           "durkhaz 2019"

#ifdef _DEBUG
#define VER_VER_DEBUG				VS_FF_DEBUG
#else
#define VER_VER_DEBUG				0
#endif

#define VER_FILEOS                  VOS_NT_WINDOWS32
#define VER_FILEFLAGS               VER_VER_DEBUG
#define VER_FILETYPE                VFT_APP
