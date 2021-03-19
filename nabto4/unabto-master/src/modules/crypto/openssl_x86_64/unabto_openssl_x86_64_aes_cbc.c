#include <unabto/unabto_aes_cbc.h>

#include "unabto_openssl_x86_64_aes.h"
#include "unabto_openssl_x86_64.h"

static AES_KEY ctx;

bool unabto_aes128_cbc_encrypt(const uint8_t* key, uint8_t* input, uint16_t input_len) {
    OPENSSL_cpuid_setup();
    if ((input_len < 16) || (input_len % 16 != 0)) {
        return false;
    }
    
    aesni_set_encrypt_key(key, 128, &ctx);

    while(input_len > 0)
    {
        int i;
        if (((uintptr_t)(const void *)input & 3) == 0) {
            for (i = 0; i < 4; i++) {
                uint32_t* ptr = (uint32_t*)input;
                ptr[i] = ptr[i] ^ ptr[i-4];
            }
        } else {
            for(i = 0; i < 16; i++)
            {
                input[i] = input[i] ^ input[i - 16];
            }
        }
            
        aesni_encrypt(input, input, &ctx);
        
        input += 16;
        input_len -= 16;
    }
    
    return true;
}

/**
 * we are running the algoritm backwards to eliminate the need to remember too much state.
 */
bool unabto_aes128_cbc_decrypt(const uint8_t* key, uint8_t* input, uint16_t input_len) {
    OPENSSL_cpuid_setup();
    if (input_len < 16) {
        return false; // the input contains at most the iv.
    }
    if ((input_len % 16) != 0) {
        return false; // the input_len should be a multiple of the block size.
    }
    
    aesni_set_decrypt_key(key, 128, &ctx);

    input += input_len - 16;
    
    while(input_len > 0)
    {
        int i;
        aesni_decrypt(input, input, &ctx);

        if (((uintptr_t)(const void *)input & 3) == 0) {
            for (i = 0; i < 4; i++) {
                uint32_t* ptr = (uint32_t*)input;
                ptr[i] = ptr[i] ^ ptr[i-4];
            }
        } else {
            for(i = 0; i < 16; i++)
            {
                input[i] = input[i] ^ input[i - 16];
            }
        }
        input -= 16;
        input_len -= 16;
    }
    return true;
}
