/* ====================================================================
 * Copyright (c) 2003 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "openssl/opensslconf.h"

#ifndef OPENSSL_FIPS
#error FIPS is disabled.
#endif

#ifdef OPENSSL_FIPS

#ifdef  __cplusplus
extern "C" {
#endif

struct dsa_st;
struct evp_pkey_st;
struct env_md_st;
struct evp_cipher_st;
struct evp_cipher_ctx_st;

int FIPS_mode_set(int onoff);
int FIPS_mode(void);
const void *FIPS_rand_check(void);
int FIPS_selftest_failed(void);
void FIPS_selftest_check(void);
void FIPS_corrupt_sha1(void);
int FIPS_selftest_sha1(void);
void FIPS_corrupt_aes(void);
int FIPS_selftest_aes(void);
void FIPS_corrupt_des(void);
int FIPS_selftest_des(void);
void FIPS_corrupt_rsa(void);
void FIPS_corrupt_rsa_keygen(void);
int FIPS_selftest_rsa(void);
void FIPS_corrupt_dsa(void);
void FIPS_corrupt_dsa_keygen(void);
int FIPS_selftest_dsa(void);
void FIPS_corrupt_rng(void);
void FIPS_rng_stick(void);
int FIPS_selftest_rng(void);
int FIPS_selftest_hmac(void);

int fips_pkey_signature_test(struct evp_pkey_st *pkey,
			const unsigned char *tbs, int tbslen,
			const unsigned char *kat, unsigned int katlen,
			const struct env_md_st *digest, unsigned int md_flags,
			const char *fail_str);

int fips_cipher_test(struct evp_cipher_ctx_st *ctx,
			const struct evp_cipher_st *cipher,
			const unsigned char *key,
			const unsigned char *iv,
			const unsigned char *plaintext,
			const unsigned char *ciphertext,
			int len);

/* BEGIN ERROR CODES */
/* The following lines are auto generated by the script mkerr.pl. Any changes
 * made after this point may be overwritten when the script is next run.
 */
void ERR_load_FIPS_strings(void);

/* Error codes for the FIPS functions. */

/* Function codes. */
#define FIPS_F_DH_BUILTIN_GENPARAMS			 100
#define FIPS_F_DSA_BUILTIN_PARAMGEN			 101
#define FIPS_F_DSA_DO_SIGN				 102
#define FIPS_F_DSA_DO_VERIFY				 103
#define FIPS_F_EVP_CIPHERINIT_EX			 124
#define FIPS_F_EVP_DIGESTINIT_EX			 125
#define FIPS_F_FIPS_CHECK_DSA				 104
#define FIPS_F_FIPS_CHECK_INCORE_FINGERPRINT		 105
#define FIPS_F_FIPS_CHECK_RSA				 106
#define FIPS_F_FIPS_DSA_CHECK				 107
#define FIPS_F_FIPS_MODE_SET				 108
#define FIPS_F_FIPS_PKEY_SIGNATURE_TEST			 109
#define FIPS_F_FIPS_SELFTEST_AES			 110
#define FIPS_F_FIPS_SELFTEST_DES			 111
#define FIPS_F_FIPS_SELFTEST_DSA			 112
#define FIPS_F_FIPS_SELFTEST_HMAC			 113
#define FIPS_F_FIPS_SELFTEST_RNG			 114
#define FIPS_F_FIPS_SELFTEST_SHA1			 115
#define FIPS_F_HASH_FINAL				 123
#define FIPS_F_RSA_BUILTIN_KEYGEN			 116
#define FIPS_F_RSA_EAY_PRIVATE_DECRYPT			 117
#define FIPS_F_RSA_EAY_PRIVATE_ENCRYPT			 118
#define FIPS_F_RSA_EAY_PUBLIC_DECRYPT			 119
#define FIPS_F_RSA_EAY_PUBLIC_ENCRYPT			 120
#define FIPS_F_RSA_X931_GENERATE_KEY_EX			 121
#define FIPS_F_SSLEAY_RAND_BYTES			 122

/* Reason codes. */
#define FIPS_R_CANNOT_READ_EXE				 103
#define FIPS_R_CANNOT_READ_EXE_DIGEST			 104
#define FIPS_R_CONTRADICTING_EVIDENCE			 114
#define FIPS_R_EXE_DIGEST_DOES_NOT_MATCH		 105
#define FIPS_R_FINGERPRINT_DOES_NOT_MATCH		 110
#define FIPS_R_FINGERPRINT_DOES_NOT_MATCH_NONPIC_RELOCATED 111
#define FIPS_R_FINGERPRINT_DOES_NOT_MATCH_SEGMENT_ALIASING 112
#define FIPS_R_FIPS_MODE_ALREADY_SET			 102
#define FIPS_R_FIPS_SELFTEST_FAILED			 106
#define FIPS_R_INVALID_KEY_LENGTH			 109
#define FIPS_R_KEY_TOO_SHORT				 108
#define FIPS_R_NON_FIPS_METHOD				 100
#define FIPS_R_PAIRWISE_TEST_FAILED			 107
#define FIPS_R_RSA_DECRYPT_ERROR			 115
#define FIPS_R_RSA_ENCRYPT_ERROR			 116
#define FIPS_R_SELFTEST_FAILED				 101
#define FIPS_R_TEST_FAILURE				 117
#define FIPS_R_UNSUPPORTED_PLATFORM			 113

#ifdef  __cplusplus
}
#endif
#endif
