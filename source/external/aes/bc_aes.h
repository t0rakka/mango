/*********************************************************************
* Filename:   aes.h
* Author:     Brad Conte (brad AT bradconte.com)
* Copyright:
* Disclaimer: This code is presented "as is" without any guarantees.
* Details:    Defines the API for the corresponding AES implementation.
*********************************************************************/

#ifndef AES_H
#define AES_H

/*************************** HEADER FILES ***************************/
#include <stddef.h>

/****************************** MACROS ******************************/
#define AES_BLOCK_SIZE 16               // AES operates on 16 aes_u8s at a time

/**************************** DATA TYPES ****************************/
typedef unsigned char aes_u8;            // 8-bit aes_u8
typedef unsigned int aes_u32;             // 32-bit aes_u32, change to "long" for 16-bit machines

/*********************** FUNCTION DECLARATIONS **********************/
///////////////////
// AES
///////////////////
// Key setup must be done before any AES en/de-cryption functions can be used.
void aes_key_setup(const aes_u8 key[],          // The key, must be 128, 192, or 256 bits
                   aes_u32 w[],                  // Output key schedule to be used later
                   int keysize);              // Bit length of the key, 128, 192, or 256

void aes_encrypt(const aes_u8 in[],             // 16 aes_u8s of plaintext
                 aes_u8 out[],                  // 16 aes_u8s of ciphertext
                 const aes_u32 key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

void aes_decrypt(const aes_u8 in[],             // 16 aes_u8s of ciphertext
                 aes_u8 out[],                  // 16 aes_u8s of plaintext
                 const aes_u32 key[],            // From the key setup
                 int keysize);                // Bit length of the key, 128, 192, or 256

///////////////////
// AES - CBC
///////////////////
int aes_encrypt_cbc(const aes_u8 in[],          // Plaintext
                    size_t in_len,            // Must be a multiple of AES_BLOCK_SIZE
                    aes_u8 out[],               // Ciphertext, same length as plaintext
                    const aes_u32 key[],         // From the key setup
                    int keysize,              // Bit length of the key, 128, 192, or 256
                    const aes_u8 iv[]);         // IV, must be AES_BLOCK_SIZE aes_u8s long

// Only output the CBC-MAC of the input.
int aes_encrypt_cbc_mac(const aes_u8 in[],      // plaintext
                        size_t in_len,        // Must be a multiple of AES_BLOCK_SIZE
                        aes_u8 out[],           // Output MAC
                        const aes_u32 key[],     // From the key setup
                        int keysize,          // Bit length of the key, 128, 192, or 256
                        const aes_u8 iv[]);     // IV, must be AES_BLOCK_SIZE aes_u8s long

int aes_decrypt_cbc(const aes_u8 in[], 
                    size_t in_len, 
                    aes_u8 out[], 
                    const aes_u32 key[], 
                    int keysize, 
                    const aes_u8 iv[]);

///////////////////
// AES - CTR
///////////////////
void increment_iv(aes_u8 iv[],                  // Must be a multiple of AES_BLOCK_SIZE
                  int counter_size);          // aes_u8s of the IV used for counting (low end)

void aes_encrypt_ctr(const aes_u8 in[],         // Plaintext
                     size_t in_len,           // Any aes_u8 length
                     aes_u8 out[],              // Ciphertext, same length as plaintext
                     const aes_u32 key[],        // From the key setup
                     int keysize,             // Bit length of the key, 128, 192, or 256
                     const aes_u8 iv[]);        // IV, must be AES_BLOCK_SIZE aes_u8s long

void aes_decrypt_ctr(const aes_u8 in[],         // Ciphertext
                     size_t in_len,           // Any aes_u8 length
                     aes_u8 out[],              // Plaintext, same length as ciphertext
                     const aes_u32 key[],        // From the key setup
                     int keysize,             // Bit length of the key, 128, 192, or 256
                     const aes_u8 iv[]);        // IV, must be AES_BLOCK_SIZE aes_u8s long

///////////////////
// AES - CCM
///////////////////
// Returns True if the input parameters do not violate any constraint.
int aes_encrypt_ccm(const aes_u8 plaintext[],              // IN  - Plaintext.
                    aes_u32 plaintext_len,                  // IN  - Plaintext length.
                    const aes_u8 associated_data[],        // IN  - Associated Data included in authentication, but not encryption.
                    unsigned short associated_data_len,  // IN  - Associated Data length in aes_u8s.
                    const aes_u8 nonce[],                  // IN  - The Nonce to be used for encryption.
                    unsigned short nonce_len,            // IN  - Nonce length in aes_u8s.
                    aes_u8 ciphertext[],                   // OUT - Ciphertext, a concatination of the plaintext and the MAC.
                    aes_u32 *ciphertext_len,                // OUT - The length of the ciphertext, always plaintext_len + mac_len.
                    aes_u32 mac_len,                        // IN  - The desired length of the MAC, must be 4, 6, 8, 10, 12, 14, or 16.
                    const aes_u32 key[],                    // IN  - The AES key for encryption.
                    int keysize);                        // IN  - The length of the key in bits. Valid values are 128, 192, 256.

// Returns True if the input parameters do not violate any constraint.
// Use mac_auth to ensure decryption/validation was preformed correctly.
// If authentication does not succeed, the plaintext is zeroed out. To overwride
// this, call with mac_auth = NULL. The proper proceedure is to decrypt with
// authentication enabled (mac_auth != NULL) and make a second call to that
// ignores authentication explicitly if the first call failes.
int aes_decrypt_ccm(const aes_u8 ciphertext[],             // IN  - Ciphertext, the concatination of encrypted plaintext and MAC.
                    aes_u32 ciphertext_len,                 // IN  - Ciphertext length in aes_u8s.
                    const aes_u8 assoc[],                  // IN  - The Associated Data, required for authentication.
                    unsigned short assoc_len,            // IN  - Associated Data length in aes_u8s.
                    const aes_u8 nonce[],                  // IN  - The Nonce to use for decryption, same one as for encryption.
                    unsigned short nonce_len,            // IN  - Nonce length in aes_u8s.
                    aes_u8 plaintext[],                    // OUT - The plaintext that was decrypted. Will need to be large enough to hold ciphertext_len - mac_len.
                    aes_u32 *plaintext_len,                 // OUT - Length in aes_u8s of the output plaintext, always ciphertext_len - mac_len .
                    aes_u32 mac_len,                        // IN  - The length of the MAC that was calculated.
                    int *mac_auth,                       // OUT - TRUE if authentication succeeded, FALSE if it did not. NULL pointer will ignore the authentication.
                    const aes_u32 key[],                    // IN  - The AES key for decryption.
                    int keysize);                        // IN  - The length of the key in BITS. Valid values are 128, 192, 256.

#endif   // AES_H
