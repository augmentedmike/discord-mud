#ifndef COMPAT_H
#define COMPAT_H

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#elif __linux__
#include <openssl/sha.h>
#define CC_SHA256_DIGEST_LENGTH SHA256_DIGEST_LENGTH                                                                                                                                 
#define CC_SHA256 SHA256
#else
#error "Unsupported platform"
#endif



#endif