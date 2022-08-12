/*
   LZ4F - LZ4-Frame library
   Header File
   Copyright (C) 2011-2020, Yann Collet.
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - LZ4 source repository : https://github.com/lz4/lz4
   - LZ4 public forum : https://groups.google.com/forum/#!forum/lz4c
*/

/* LZ4F is a stand-alone API able to create and decode LZ4 frames
 * conformant with specification v1.6.1 in doc/lz4_Frame_format.md .
 * Generated frames are compatible with `lz4` CLI.
 *
 * LZ4F also offers streaming capabilities.
 *
 * lz4.h is not required when using lz4frame.h,
 * except to extract common constants such as LZ4_VERSION_NUMBER.
 * */

#ifndef LZ4F_H_09782039843
#define LZ4F_H_09782039843

#if defined (__cplusplus)
extern "C" {
#endif

/* ---   Dependency   --- */
#include <stddef.h>   /* size_t */


/**
 * Introduction
 *
 * lz4frame.h implements LZ4 frame specification: see doc/lz4_Frame_format.md .
 * LZ4 Frames are compatible with `lz4` CLI,
 * and designed to be interoperable with any system.
**/

/*-***************************************************************
 *  Compiler specifics
 *****************************************************************/
/*  LZ4_DLL_EXPORT :
 *  Enable exporting of functions when building a Windows DLL
 *  LZ4FLIB_VISIBILITY :
 *  Control library symbols visibility.
 */
#ifndef LZ4FLIB_VISIBILITY
#  if defined(__GNUC__) && (__GNUC__ >= 4)
#    define LZ4FLIB_VISIBILITY __attribute__ ((visibility ("default")))
#  else
#    define LZ4FLIB_VISIBILITY
#  endif
#endif
#if defined(LZ4_DLL_EXPORT) && (LZ4_DLL_EXPORT==1)
#  define LZ4FLIB_API __declspec(dllexport) LZ4FLIB_VISIBILITY
#elif defined(LZ4_DLL_IMPORT) && (LZ4_DLL_IMPORT==1)
#  define LZ4FLIB_API __declspec(dllimport) LZ4FLIB_VISIBILITY
#else
#  define LZ4FLIB_API LZ4FLIB_VISIBILITY
#endif

#ifdef LZ4F_DISABLE_DEPRECATE_WARNINGS
#  define LZ4F_DEPRECATE(x) x
#else
#  if defined(_MSC_VER)
#    define LZ4F_DEPRECATE(x) x   /* __declspec(deprecated) x - only works with C++ */
#  elif defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 6))
#    define LZ4F_DEPRECATE(x) x __attribute__((deprecated))
#  else
#    define LZ4F_DEPRECATE(x) x   /* no deprecation warning for this compiler */
#  endif
#endif


/*-************************************
 *  Error management
 **************************************/
typedef size_t LZ4F_errorCode_t;

LZ4FLIB_API unsigned    LZ4F_isError(LZ4F_errorCode_t code);   /**< tells when a function result is an error code */
LZ4FLIB_API const char* LZ4F_getErrorName(LZ4F_errorCode_t code);   /**< return error code string; for debugging */


/*-************************************
 *  Frame compression types
 ************************************* */
/* #define LZ4F_ENABLE_OBSOLETE_ENUMS   // uncomment to enable obsolete enums */
#ifdef LZ4F_ENABLE_OBSOLETE_ENUMS
#  define LZ4F_OBSOLETE_ENUM(x) , LZ4F_DEPRECATE(x) = LZ4F_##x
#else
#  define LZ4F_OBSOLETE_ENUM(x)
#endif

/* The larger the block size, the (slightly) better the compression ratio,
 * though there are diminishing returns.
 * Larger blocks also increase memory usage on both compression and decompression sides.
 */
typedef enum {
    LZ4F_default=0,
    LZ4F_max64KB=4,
    LZ4F_max256KB=5,
    LZ4F_max1MB=6,
    LZ4F_max4MB=7
    LZ4F_OBSOLETE_ENUM(max64KB)
    LZ4F_OBSOLETE_ENUM(max256KB)
    LZ4F_OBSOLETE_ENUM(max1MB)
    LZ4F_OBSOLETE_ENUM(max4MB)
} LZ4F_blockSizeID_t;

/* Linked blocks sharply reduce inefficiencies when using small blocks,
 * they compress better.
 * However, some LZ4 decoders are only compatible with independent blocks */
typedef enum {
    LZ4F_blockLinked=0,
    LZ4F_blockIndependent
    LZ4F_OBSOLETE_ENUM(blockLinked)
    LZ4F_OBSOLETE_ENUM(blockIndependent)
} LZ4F_blockMode_t;

typedef enum {
    LZ4F_noContentChecksum=0,
    LZ4F_contentChecksumEnabled
    LZ4F_OBSOLETE_ENUM(noContentChecksum)
    LZ4F_OBSOLETE_ENUM(contentChecksumEnabled)
} LZ4F_contentChecksum_t;

typedef enum {
    LZ4F_noBlockChecksum=0,
    LZ4F_blockChecksumEnabled
} LZ4F_blockChecksum_t;

typedef enum {
    LZ4F_frame=0,
    LZ4F_skippableFrame
    LZ4F_OBSOLETE_ENUM(skippableFrame)
} LZ4F_frameType_t;

#ifdef LZ4F_ENABLE_OBSOLETE_ENUMS
typedef LZ4F_blockSizeID_t blockSizeID_t;
typedef LZ4F_blockMode_t blockMode_t;
typedef LZ4F_frameType_t frameType_t;
typedef LZ4F_contentChecksum_t contentChecksum_t;
#endif

/*! LZ4F_frameInfo_t :
 *  makes it possible to set or read frame parameters.
 *  Structure must be first init to 0, using memset() or LZ4F_INIT_FRAMEINFO,
 *  setting all parameters to default.
 *  It's then possible to update selectively some parameters */
typedef struct {
  LZ4F_blockSizeID_t     blockSizeID;         /* max64KB, max256KB, max1MB, max4MB; 0 == default */
  LZ4F_blockMode_t       blockMode;           /* LZ4F_blockLinked, LZ4F_blockIndependent; 0 == default */
  LZ4F_contentChecksum_t contentChecksumFlag; /* 1: frame terminated with 32-bit checksum of decompressed data; 0: disabled (default) */
  LZ4F_frameType_t       frameType;           /* read-only field : LZ4F_frame or LZ4F_skippableFrame */
  unsigned long long     contentSize;         /* Size of uncompressed content ; 0 == unknown */
  unsigned               dictID;              /* Dictionary ID, sent by compressor to help decoder select correct dictionary; 0 == no dictID provided */
  LZ4F_blockChecksum_t   blockChecksumFlag;   /* 1: each block followed by a checksum of block's compressed data; 0: disabled (default) */
} LZ4F_frameInfo_t;

#define LZ4F_INIT_FRAMEINFO   { LZ4F_default, LZ4F_blockLinked, LZ4F_noContentChecksum, LZ4F_frame, 0ULL, 0U, LZ4F_noBlockChecksum }    /* v1.8.3+ */

/*! LZ4F_preferences_t :
 *  makes it possible to supply advanced compression instructions to streaming interface.
 *  Structure must be first init to 0, using memset() or LZ4F_INIT_PREFERENCES,
 *  setting all parameters to default.
 *  All reserved fields must be set to zero. */
typedef struct {
  LZ4F_frameInfo_t frameInfo;
  int      compressionLevel;    /* 0: default (fast mode); values > LZ4HC_CLEVEL_MAX count as LZ4HC_CLEVEL_MAX; values < 0 trigger "fast acceleration" */
  unsigned autoFlush;           /* 1: always flush; reduces usage of internal buffers */
  unsigned favorDecSpeed;       /* 1: parser favors decompression speed vs compression ratio. Only works for high compression modes (>= LZ4HC_CLEVEL_OPT_MIN) */  /* v1.8.2+ */
  unsigned reserved[3];         /* must be zero for forward compatibility */
} LZ4F_preferences_t;

#define LZ4F_INIT_PREFERENCES   { LZ4F_INIT_FRAMEINFO, 0, 0u, 0u, { 0u, 0u, 0u } }    /* v1.8.3+ */

/*-***********************************
*  Advanced compression functions
*************************************/

/*---   Resource Management   ---*/

#define LZ4F_VERSION 100    /* This number can be used to check for an incompatible API breaking change */
LZ4FLIB_API unsigned LZ4F_getVersion(void);

/*----    Compression    ----*/

#define LZ4F_HEADER_SIZE_MIN  7   /* LZ4 Frame header size can vary, depending on selected parameters */
#define LZ4F_HEADER_SIZE_MAX 19

/* Size in bytes of a block header in little-endian format. Highest bit indicates if block data is uncompressed */
#define LZ4F_BLOCK_HEADER_SIZE 4

/* Size in bytes of a block checksum footer in little-endian format. */
#define LZ4F_BLOCK_CHECKSUM_SIZE 4

/* Size in bytes of the content checksum. */
#define LZ4F_CONTENT_CHECKSUM_SIZE 4

/*-*********************************
*  Decompression functions
***********************************/
typedef struct LZ4F_dctx_s LZ4F_dctx;   /* incomplete type */
typedef LZ4F_dctx* LZ4F_decompressionContext_t;   /* compatibility with previous API versions */

typedef struct {
  unsigned stableDst;     /* pledges that last 64KB decompressed data will remain available unmodified between invocations.
                           * This optimization skips storage operations in tmp buffers. */
  unsigned skipChecksums; /* disable checksum calculation and verification, even when one is present in frame, to save CPU time.
                           * Setting this option to 1 once disables all checksums for the rest of the frame. */
  unsigned reserved1;     /* must be set to zero for forward compatibility */
  unsigned reserved0;     /* idem */
} LZ4F_decompressOptions_t;


/* Resource management */

/*! LZ4F_createDecompressionContext() :
 *  Create an LZ4F_dctx object, to track all decompression operations.
 *  @version provided MUST be LZ4F_VERSION.
 *  @dctxPtr MUST be valid.
 *  The function fills @dctxPtr with the value of a pointer to an allocated and initialized LZ4F_dctx object.
 *  The @return is an errorCode, which can be tested using LZ4F_isError().
 *  dctx memory can be released using LZ4F_freeDecompressionContext();
 *  Result of LZ4F_freeDecompressionContext() indicates current state of decompressionContext when being released.
 *  That is, it should be == 0 if decompression has been completed fully and correctly.
 */
LZ4FLIB_API LZ4F_errorCode_t LZ4F_createDecompressionContext(LZ4F_dctx** dctxPtr, unsigned version);
LZ4FLIB_API LZ4F_errorCode_t LZ4F_freeDecompressionContext(LZ4F_dctx* dctx);


/*-***********************************
*  Streaming decompression functions
*************************************/

#define LZ4F_MAGICNUMBER 0x184D2204U
#define LZ4F_MAGIC_SKIPPABLE_START 0x184D2A50U
#define LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH 5

/*! LZ4F_headerSize() : v1.9.0+
 *  Provide the header size of a frame starting at `src`.
 * `srcSize` must be >= LZ4F_MIN_SIZE_TO_KNOW_HEADER_LENGTH,
 *  which is enough to decode the header length.
 * @return : size of frame header
 *           or an error code, which can be tested using LZ4F_isError()
 *  note : Frame header size is variable, but is guaranteed to be
 *         >= LZ4F_HEADER_SIZE_MIN bytes, and <= LZ4F_HEADER_SIZE_MAX bytes.
 */
LZ4FLIB_API size_t LZ4F_headerSize(const void* src, size_t srcSize);

/*! LZ4F_getFrameInfo() :
 *  This function extracts frame parameters (max blockSize, dictID, etc.).
 *  Its usage is optional: user can also invoke LZ4F_decompress() directly.
 *
 *  Extracted information will fill an existing LZ4F_frameInfo_t structure.
 *  This can be useful for allocation and dictionary identification purposes.
 *
 *  LZ4F_getFrameInfo() can work in the following situations :
 *
 *  1) At the beginning of a new frame, before any invocation of LZ4F_decompress().
 *     It will decode header from `srcBuffer`,
 *     consuming the header and starting the decoding process.
 *
 *     Input size must be large enough to contain the full frame header.
 *     Frame header size can be known beforehand by LZ4F_headerSize().
 *     Frame header size is variable, but is guaranteed to be >= LZ4F_HEADER_SIZE_MIN bytes,
 *     and not more than <= LZ4F_HEADER_SIZE_MAX bytes.
 *     Hence, blindly providing LZ4F_HEADER_SIZE_MAX bytes or more will always work.
 *     It's allowed to provide more input data than the header size,
 *     LZ4F_getFrameInfo() will only consume the header.
 *
 *     If input size is not large enough,
 *     aka if it's smaller than header size,
 *     function will fail and return an error code.
 *
 *  2) After decoding has been started,
 *     it's possible to invoke LZ4F_getFrameInfo() anytime
 *     to extract already decoded frame parameters stored within dctx.
 *
 *     Note that, if decoding has barely started,
 *     and not yet read enough information to decode the header,
 *     LZ4F_getFrameInfo() will fail.
 *
 *  The number of bytes consumed from srcBuffer will be updated in *srcSizePtr (necessarily <= original value).
 *  LZ4F_getFrameInfo() only consumes bytes when decoding has not yet started,
 *  and when decoding the header has been successful.
 *  Decompression must then resume from (srcBuffer + *srcSizePtr).
 *
 * @return : a hint about how many srcSize bytes LZ4F_decompress() expects for next call,
 *           or an error code which can be tested using LZ4F_isError().
 *  note 1 : in case of error, dctx is not modified. Decoding operation can resume from beginning safely.
 *  note 2 : frame parameters are *copied into* an already allocated LZ4F_frameInfo_t structure.
 */
LZ4FLIB_API size_t
LZ4F_getFrameInfo(LZ4F_dctx* dctx,
                  LZ4F_frameInfo_t* frameInfoPtr,
            const void* srcBuffer, size_t* srcSizePtr);

/*! LZ4F_decompress() :
 *  Call this function repetitively to regenerate data compressed in `srcBuffer`.
 *
 *  The function requires a valid dctx state.
 *  It will read up to *srcSizePtr bytes from srcBuffer,
 *  and decompress data into dstBuffer, of capacity *dstSizePtr.
 *
 *  The nb of bytes consumed from srcBuffer will be written into *srcSizePtr (necessarily <= original value).
 *  The nb of bytes decompressed into dstBuffer will be written into *dstSizePtr (necessarily <= original value).
 *
 *  The function does not necessarily read all input bytes, so always check value in *srcSizePtr.
 *  Unconsumed source data must be presented again in subsequent invocations.
 *
 * `dstBuffer` can freely change between each consecutive function invocation.
 * `dstBuffer` content will be overwritten.
 *
 * @return : an hint of how many `srcSize` bytes LZ4F_decompress() expects for next call.
 *  Schematically, it's the size of the current (or remaining) compressed block + header of next block.
 *  Respecting the hint provides some small speed benefit, because it skips intermediate buffers.
 *  This is just a hint though, it's always possible to provide any srcSize.
 *
 *  When a frame is fully decoded, @return will be 0 (no more data expected).
 *  When provided with more bytes than necessary to decode a frame,
 *  LZ4F_decompress() will stop reading exactly at end of current frame, and @return 0.
 *
 *  If decompression failed, @return is an error code, which can be tested using LZ4F_isError().
 *  After a decompression error, the `dctx` context is not resumable.
 *  Use LZ4F_resetDecompressionContext() to return to clean state.
 *
 *  After a frame is fully decoded, dctx can be used again to decompress another frame.
 */
LZ4FLIB_API size_t
LZ4F_decompress(LZ4F_dctx* dctx,
                void* dstBuffer, size_t* dstSizePtr,
          const void* srcBuffer, size_t* srcSizePtr,
          const LZ4F_decompressOptions_t* dOptPtr);


/*! LZ4F_resetDecompressionContext() : added in v1.8.0
 *  In case of an error, the context is left in "undefined" state.
 *  In which case, it's necessary to reset it, before re-using it.
 *  This method can also be used to abruptly stop any unfinished decompression,
 *  and start a new one using same context resources. */
LZ4FLIB_API void LZ4F_resetDecompressionContext(LZ4F_dctx* dctx);   /* always successful */



#if defined (__cplusplus)
}
#endif

#endif  /* LZ4F_H_09782039843 */

#if defined(LZ4F_STATIC_LINKING_ONLY) && !defined(LZ4F_H_STATIC_09782039843)
#define LZ4F_H_STATIC_09782039843

#if defined (__cplusplus)
extern "C" {
#endif

/* These declarations are not stable and may change in the future.
 * They are therefore only safe to depend on
 * when the caller is statically linked against the library.
 * To access their declarations, define LZ4F_STATIC_LINKING_ONLY.
 *
 * By default, these symbols aren't published into shared/dynamic libraries.
 * You can override this behavior and force them to be published
 * by defining LZ4F_PUBLISH_STATIC_FUNCTIONS.
 * Use at your own risk.
 */
#ifdef LZ4F_PUBLISH_STATIC_FUNCTIONS
# define LZ4FLIB_STATIC_API LZ4FLIB_API
#else
# define LZ4FLIB_STATIC_API
#endif


/* ---   Error List   --- */
#define LZ4F_LIST_ERRORS(ITEM) \
        ITEM(OK_NoError) \
        ITEM(ERROR_GENERIC) \
        ITEM(ERROR_maxBlockSize_invalid) \
        ITEM(ERROR_blockMode_invalid) \
        ITEM(ERROR_contentChecksumFlag_invalid) \
        ITEM(ERROR_compressionLevel_invalid) \
        ITEM(ERROR_headerVersion_wrong) \
        ITEM(ERROR_blockChecksum_invalid) \
        ITEM(ERROR_reservedFlag_set) \
        ITEM(ERROR_allocation_failed) \
        ITEM(ERROR_srcSize_tooLarge) \
        ITEM(ERROR_dstMaxSize_tooSmall) \
        ITEM(ERROR_frameHeader_incomplete) \
        ITEM(ERROR_frameType_unknown) \
        ITEM(ERROR_frameSize_wrong) \
        ITEM(ERROR_srcPtr_wrong) \
        ITEM(ERROR_decompressionFailed) \
        ITEM(ERROR_headerChecksum_invalid) \
        ITEM(ERROR_contentChecksum_invalid) \
        ITEM(ERROR_frameDecoding_alreadyStarted) \
        ITEM(ERROR_compressionState_uninitialized) \
        ITEM(ERROR_parameter_null) \
        ITEM(ERROR_maxCode)

#define LZ4F_GENERATE_ENUM(ENUM) LZ4F_##ENUM,

/* enum list is exposed, to handle specific errors */
typedef enum { LZ4F_LIST_ERRORS(LZ4F_GENERATE_ENUM)
              _LZ4F_dummy_error_enum_for_c89_never_used } LZ4F_errorCodes;

LZ4FLIB_STATIC_API LZ4F_errorCodes LZ4F_getErrorCode(size_t functionResult);


/*! LZ4F_getBlockSize() :
 *  Return, in scalar format (size_t),
 *  the maximum block size associated with blockSizeID.
**/
LZ4FLIB_STATIC_API size_t LZ4F_getBlockSize(LZ4F_blockSizeID_t blockSizeID);

/**********************************
 *  Bulk processing dictionary API
 *********************************/

/*! LZ4F_decompress_usingDict() :
 *  Same as LZ4F_decompress(), using a predefined dictionary.
 *  Dictionary is used "in place", without any preprocessing.
**  It must remain accessible throughout the entire frame decoding. */
LZ4FLIB_STATIC_API size_t
LZ4F_decompress_usingDict(LZ4F_dctx* dctxPtr,
                          void* dstBuffer, size_t* dstSizePtr,
                    const void* srcBuffer, size_t* srcSizePtr,
                    const void* dict, size_t dictSize,
                    const LZ4F_decompressOptions_t* decompressOptionsPtr);


/*! Custom memory allocation :
 *  These prototypes make it possible to pass custom allocation/free functions.
 *  LZ4F_customMem is provided at state creation time, using LZ4F_create*_advanced() listed below.
 *  All allocation/free operations will be completed using these custom variants instead of regular <stdlib.h> ones.
 */
typedef void* (*LZ4F_AllocFunction) (void* opaqueState, size_t size);
typedef void* (*LZ4F_CallocFunction) (void* opaqueState, size_t size);
typedef void  (*LZ4F_FreeFunction) (void* opaqueState, void* address);
typedef struct {
    LZ4F_AllocFunction customAlloc;
    LZ4F_CallocFunction customCalloc; /* optional; when not defined, uses customAlloc + memset */
    LZ4F_FreeFunction customFree;
    void* opaqueState;
} LZ4F_CustomMem;
static
#ifdef __GNUC__
__attribute__((__unused__))
#endif
LZ4F_CustomMem const LZ4F_defaultCMem = { NULL, NULL, NULL, NULL };  /**< this constant defers to stdlib's functions */

LZ4FLIB_STATIC_API LZ4F_dctx* LZ4F_createDecompressionContext_advanced(LZ4F_CustomMem customMem, unsigned version);


#if defined (__cplusplus)
}
#endif

#endif  /* defined(LZ4F_STATIC_LINKING_ONLY) && !defined(LZ4F_H_STATIC_09782039843) */
