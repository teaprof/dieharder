#ifndef _SKEIN_H_
#define _SKEIN_H_ 1
/**************************************************************************
**
** Interface declarations and internal definitions for Skein hashing.
**
** Source code author: Doug Whiting, 2008.
**
** This algorithm and source code is released to the public domain.
**
***************************************************************************
**
** The following compile-time switches may be defined to control some
** tradeoffs between speed, code size, error checking, and security.
**
** The "default" note explains what happens when the switch is not defined.
**
**  SKEIN_DEBUG            -- make callouts from inside Skein code
**                            to examine/display intermediate values.
**                            [default: no callouts (no overhead)]
**
**  SKEIN_ERR_CHECK        -- how error checking is handled inside Skein
**                            code. If not defined, most error checking
**                            is disabled (for performance). Otherwise,
**                            the switch value is interpreted as:
**                                0: use assert()      to flag errors
**                                1: return SKEIN_FAIL to flag errors
**
***************************************************************************/

#include <dieharder/skein_port.h> /* get platform-specific definitions */

#include <stddef.h> /* get size_t definition */

enum {
    SKEIN_SUCCESS = 0, /* return codes from Skein calls */
    SKEIN_FAIL = 1,
    SKEIN_BAD_HASHLEN = 2
};

#define SKEIN_MODIFIER_WORDS (2) /* number of modifier (tweak) words */

#define SKEIN_512_STATE_WORDS (8)
#define SKEIN_MAX_STATE_WORDS (16)

#define SKEIN_512_STATE_BYTES (8 * SKEIN_512_STATE_WORDS)

#define SKEIN_512_STATE_BITS (64 * SKEIN_512_STATE_WORDS)

#define SKEIN_512_BLOCK_BYTES (8 * SKEIN_512_STATE_WORDS)

typedef struct {
    size_t hashBitLen;              /* size of hash result, in bits */
    size_t bCnt;                    /* current byte count in buffer b[] */
    u64b_t T[SKEIN_MODIFIER_WORDS]; /* tweak words: T[0]=byte cnt, T[1]=flags */
} Skein_Ctxt_Hdr_t;

typedef struct /*  512-bit Skein hash context structure */
{
    Skein_Ctxt_Hdr_t h;              /* common header context variables */
    u64b_t X[SKEIN_512_STATE_WORDS]; /* chaining variables */
    u08b_t b[SKEIN_512_BLOCK_BYTES]; /* partial block buffer (8-byte aligned) */
} Skein_512_Ctxt_t;

typedef struct {
    u64b_t T[SKEIN_MODIFIER_WORDS];     // Tweak
    u64b_t Key[SKEIN_512_STATE_WORDS];  // Key
} Threefish_512_Ctxt_t;

void Threefish_512_Process_Blocks(Threefish_512_Ctxt_t* ctx, const u08b_t* input, void* output, size_t blkCnt);

void Threefish_512_Process_Blocks64(Threefish_512_Ctxt_t* ctx, const u08b_t* input, void* output, size_t blkCnt);

/*   Skein APIs for (incremental) "straight hashing" */
int Skein_512_Init(Skein_512_Ctxt_t* ctx, size_t hashBitLen);

int Skein_512_Update(Skein_512_Ctxt_t* ctx, const u08b_t* msg, size_t msgByteCnt);

int Skein_512_Final(Skein_512_Ctxt_t* ctx, u08b_t* hashVal);

/*
**   Skein APIs for "extended" initialization: MAC keys, tree hashing.
**   After an InitExt() call, just use Update/Final calls as with Init().
**
**   Notes: Same parameters as _Init() calls, plus treeInfo/key/keyBytes.
**          When keyBytes == 0 and treeInfo == SKEIN_SEQUENTIAL,
**              the results of InitExt() are identical to calling Init().
**          The function Init() may be called once to "precompute" the IV for
**              a given hashBitLen value, then by saving a copy of the context
**              the IV computation may be avoided in later calls.
**          Similarly, the function InitExt() may be called once per MAC key
**              to precompute the MAC IV, then a copy of the context saved and
**              reused for each new MAC computation.
**/
int Skein_512_InitExt(Skein_512_Ctxt_t* ctx, size_t hashBitLen, u64b_t treeInfo, const u08b_t* key, size_t keyBytes);

/*
**   Skein APIs for tree hash:
**		Final_Pad:  pad, do final block, but no OUTPUT type
**		Output:     do just the output stage
*/
#ifndef SKEIN_TREE_HASH
#define SKEIN_TREE_HASH (1)
#endif
#if SKEIN_TREE_HASH
int Skein_512_Final_Pad(Skein_512_Ctxt_t* ctx, u08b_t* hashVal);

int Skein_512_Output(Skein_512_Ctxt_t* ctx, u08b_t* hashVal);
#endif

/*****************************************************************
** "Internal" Skein definitions
**    -- not needed for sequential hashing API, but will be
**           helpful for other uses of Skein (e.g., tree hash mode).
**    -- included here so that they can be shared between
**           reference and optimized code.
******************************************************************/

/* tweak word T[1]: bit field starting positions */
#define SKEIN_T1_BIT(BIT) ((BIT)-64) /* offset 64 because it's the second word  */

#define SKEIN_T1_POS_TREE_LVL SKEIN_T1_BIT(112) /* bits 112..118: level in hash tree       */
#define SKEIN_T1_POS_BIT_PAD SKEIN_T1_BIT(119)  /* bit  119     : partial final input byte */
#define SKEIN_T1_POS_BLK_TYPE SKEIN_T1_BIT(120) /* bits 120..125: type field               */
#define SKEIN_T1_POS_FIRST SKEIN_T1_BIT(126)    /* bits 126     : first block flag         */
#define SKEIN_T1_POS_FINAL SKEIN_T1_BIT(127)    /* bit  127     : final block flag         */

/* tweak word T[1]: flag bit definition(s) */
#define SKEIN_T1_FLAG_FIRST (((u64b_t)1) << SKEIN_T1_POS_FIRST)
#define SKEIN_T1_FLAG_FINAL (((u64b_t)1) << SKEIN_T1_POS_FINAL)
#define SKEIN_T1_FLAG_BIT_PAD (((u64b_t)1) << SKEIN_T1_POS_BIT_PAD)

/* tweak word T[1]: tree level bit field mask */
#define SKEIN_T1_TREE_LVL_MASK (((u64b_t)0x7F) << SKEIN_T1_POS_TREE_LVL)
#define SKEIN_T1_TREE_LEVEL(n) (((u64b_t)(n)) << SKEIN_T1_POS_TREE_LVL)

/* tweak word T[1]: block type field */
#define SKEIN_BLK_TYPE_KEY (0)    /* key, for MAC and KDF */
#define SKEIN_BLK_TYPE_CFG (4)    /* configuration block */
#define SKEIN_BLK_TYPE_PERS (8)   /* personalization string */
#define SKEIN_BLK_TYPE_PK (12)    /* public key (for digital signature hashing) */
#define SKEIN_BLK_TYPE_KDF (16)   /* key identifier for KDF */
#define SKEIN_BLK_TYPE_NONCE (20) /* nonce for PRNG */
#define SKEIN_BLK_TYPE_MSG (48)   /* message processing */
#define SKEIN_BLK_TYPE_OUT (63)   /* output stage */
#define SKEIN_BLK_TYPE_MASK (63)  /* bit field mask */

#define SKEIN_T1_BLK_TYPE(T) (((u64b_t)(SKEIN_BLK_TYPE_##T)) << SKEIN_T1_POS_BLK_TYPE)
#define SKEIN_T1_BLK_TYPE_KEY SKEIN_T1_BLK_TYPE(KEY)     /* key, for MAC and KDF */
#define SKEIN_T1_BLK_TYPE_CFG SKEIN_T1_BLK_TYPE(CFG)     /* configuration block */
#define SKEIN_T1_BLK_TYPE_PERS SKEIN_T1_BLK_TYPE(PERS)   /* personalization string */
#define SKEIN_T1_BLK_TYPE_PK SKEIN_T1_BLK_TYPE(PK)       /* public key (for digital signature hashing) */
#define SKEIN_T1_BLK_TYPE_KDF SKEIN_T1_BLK_TYPE(KDF)     /* key identifier for KDF */
#define SKEIN_T1_BLK_TYPE_NONCE SKEIN_T1_BLK_TYPE(NONCE) /* nonce for PRNG */
#define SKEIN_T1_BLK_TYPE_MSG SKEIN_T1_BLK_TYPE(MSG)     /* message processing */
#define SKEIN_T1_BLK_TYPE_OUT SKEIN_T1_BLK_TYPE(OUT)     /* output stage */
#define SKEIN_T1_BLK_TYPE_MASK SKEIN_T1_BLK_TYPE(MASK)   /* field bit mask */

#define SKEIN_T1_BLK_TYPE_CFG_FINAL (SKEIN_T1_BLK_TYPE_CFG | SKEIN_T1_FLAG_FINAL)
#define SKEIN_T1_BLK_TYPE_OUT_FINAL (SKEIN_T1_BLK_TYPE_OUT | SKEIN_T1_FLAG_FINAL)

#define SKEIN_VERSION (1)

#ifndef SKEIN_ID_STRING_LE              /* allow compile-time personalization */
#define SKEIN_ID_STRING_LE (0x33414853) /* "SHA3" (little-endian)*/
#endif

#define SKEIN_MK_64(hi32, lo32) ((lo32) + (((u64b_t)(hi32)) << 32))
#define SKEIN_SCHEMA_VER SKEIN_MK_64(SKEIN_VERSION, SKEIN_ID_STRING_LE)
#define SKEIN_KS_PARITY SKEIN_MK_64(0x55555555, 0x55555555)

/* bit field definitions in config block treeInfo word */
#define SKEIN_CFG_TREE_LEAF_SIZE_POS (0)
#define SKEIN_CFG_TREE_NODE_SIZE_POS (8)
#define SKEIN_CFG_TREE_MAX_LEVEL_POS (16)

#define SKEIN_CFG_TREE_LEAF_SIZE_MSK  ((u64b_t) 0xFF) << SKEIN_CFG_TREE_LEAF_SIZE_POS)
#define SKEIN_CFG_TREE_NODE_SIZE_MSK  ((u64b_t) 0xFF) << SKEIN_CFG_TREE_NODE_SIZE_POS)
#define SKEIN_CFG_TREE_MAX_LEVEL_MSK  ((u64b_t) 0xFF) << SKEIN_CFG_TREE_MAX_LEVEL_POS)

#define SKEIN_CFG_TREE_INFO_SEQUENTIAL (0) /* use as treeInfo in InitExt() call for sequential processing */
#define SKEIN_CFG_TREE_INFO(leaf, node, maxLevel) ((u64b_t)((leaf) | ((node) << 8) | ((maxLevel) << 16)))

/*
**   Skein macros for getting/setting tweak words, etc.
**   These are useful for partial input bytes, hash tree init/update, etc.
**/
#define Skein_Get_Tweak(ctxPtr, TWK_NUM) ((ctxPtr)->h.T[TWK_NUM])
#define Skein_Set_Tweak(ctxPtr, TWK_NUM, tVal) \
    { (ctxPtr)->h.T[TWK_NUM] = (tVal); }

#define Skein_Get_T0(ctxPtr) Skein_Get_Tweak(ctxPtr, 0)
#define Skein_Get_T1(ctxPtr) Skein_Get_Tweak(ctxPtr, 1)
#define Skein_Set_T0(ctxPtr, T0) Skein_Set_Tweak(ctxPtr, 0, T0)
#define Skein_Set_T1(ctxPtr, T1) Skein_Set_Tweak(ctxPtr, 1, T1)

/* set both tweak words at once */
#define Skein_Set_T0_T1(ctxPtr, T0, T1) \
    {                                   \
        Skein_Set_T0(ctxPtr, (T0));     \
        Skein_Set_T1(ctxPtr, (T1));     \
    }

#define Skein_Set_Type(ctxPtr, BLK_TYPE) Skein_Set_T1(ctxPtr, SKEIN_T1_BLK_TYPE_##BLK_TYPE)

/* set up for starting with a new type: h.T[0]=0; h.T[1] = NEW_TYPE; h.bCnt=0; */
#define Skein_Start_New_Type(ctxPtr, BLK_TYPE)                                          \
    {                                                                                   \
        Skein_Set_T0_T1(ctxPtr, 0, SKEIN_T1_FLAG_FIRST | SKEIN_T1_BLK_TYPE_##BLK_TYPE); \
        (ctxPtr)->h.bCnt = 0;                                                           \
    }

#define Skein_Clear_First_Flag(hdr) \
    { (hdr).T[1] &= ~SKEIN_T1_FLAG_FIRST; }
#define Skein_Set_Bit_Pad_Flag(hdr) \
    { (hdr).T[1] |= SKEIN_T1_FLAG_BIT_PAD; }

#define Skein_Set_Tree_Level(hdr, height) \
    { (hdr).T[1] |= SKEIN_T1_TREE_LEVEL(height); }

/*****************************************************************
** "Internal" Skein definitions for debugging and error checking
******************************************************************/
#ifdef SKEIN_DEBUG /* examine/display intermediate values? */
#include "skein_debug.h"
#else /* default is no callouts */
#define Skein_Show_Block(bits, ctx, X, blkPtr, wPtr, ksEvenPtr, ksOddPtr)
#define Skein_Show_Round(bits, ctx, r, X)
#define Skein_Show_R_Ptr(bits, ctx, r, X_ptr)
#define Skein_Show_Final(bits, ctx, cnt, outPtr)
#define Skein_Show_Key(bits, ctx, key, keyBytes)
#endif

#ifndef SKEIN_ERR_CHECK          /* run-time checks (e.g., bad params, uninitialized context)? */
#define Skein_Assert(x, retCode) /* default: ignore all Asserts, for performance */
#define Skein_assert(x)
#elif defined(SKEIN_ASSERT)
#include <assert.h>
#define Skein_Assert(x, retCode) assert(x)
#define Skein_assert(x) assert(x)
#else
#include <assert.h>
#define Skein_Assert(x, retCode)  \
    {                             \
        if (!(x)) return retCode; \
    }                             /*  caller  error */
#define Skein_assert(x) assert(x) /* internal error */
#endif

/*****************************************************************
** Skein block function constants (shared across Ref and Opt code)
******************************************************************/
enum {
    /* Skein_512 round rotation constants */
    R_512_0_0 = 38,
    R_512_0_1 = 30,
    R_512_0_2 = 50,
    R_512_0_3 = 53,
    R_512_1_0 = 48,
    R_512_1_1 = 20,
    R_512_1_2 = 43,
    R_512_1_3 = 31,
    R_512_2_0 = 34,
    R_512_2_1 = 14,
    R_512_2_2 = 15,
    R_512_2_3 = 27,
    R_512_3_0 = 26,
    R_512_3_1 = 12,
    R_512_3_2 = 58,
    R_512_3_3 = 7,
    R_512_4_0 = 33,
    R_512_4_1 = 49,
    R_512_4_2 = 8,
    R_512_4_3 = 42,
    R_512_5_0 = 39,
    R_512_5_1 = 27,
    R_512_5_2 = 41,
    R_512_5_3 = 14,
    R_512_6_0 = 29,
    R_512_6_1 = 26,
    R_512_6_2 = 11,
    R_512_6_3 = 9,
    R_512_7_0 = 33,
    R_512_7_1 = 51,
    R_512_7_2 = 39,
    R_512_7_3 = 35
};

#ifndef SKEIN_ROUNDS
#define SKEIN_512_ROUNDS_TOTAL (72)
#else /* allow command-line define in range 8*(5..14)   */
#define SKEIN_512_ROUNDS_TOTAL (8 * ((((SKEIN_ROUNDS / 10) + 5) % 10) + 5))
#endif

#endif /* ifndef _SKEIN_H_ */
