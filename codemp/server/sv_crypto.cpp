#include "server.h"
#include "sodium.h"

// sanity checks

#if CRYPTO_CIPHER_HEX_SIZE > 256 // compat limit: MAX_CVAR_VALUE_STRING
	#error CRYPTO_CIPHERHEX_LEN does not fit inside a cvar (max 255 chars)
#endif

#if CRYPTO_HASH_BIN_SIZE < crypto_generichash_BYTES_MIN 
	#error CRYPTO_HASH_BIN_LEN is too small
#endif

#if CRYPTO_HASH_BIN_SIZE > crypto_generichash_BYTES_MAX
	#error CRYPTO_HASH_BIN_LEN is too large
#endif


qboolean SV_InitCrypto( void ) {
    if ( sodium_init() == -1 ) {
        Com_Printf( "Failed to initialize libsodium crypto module!\n" );
        return qfalse;
    }

    Com_Printf( "Initialized libsodium crypto module successfully\n" );

	return qtrue;
}

static qboolean BinaryToHex( char* outHex, size_t hexMaxLen, const uint8_t* inBin, size_t binSize ) {
    if ( sodium_bin2hex( outHex, hexMaxLen, inBin, binSize ) != outHex ) {
        Com_Printf( "Binary -> Hexadecimal conversion failed!\n" );
        return qfalse;
    }
    
    return qtrue;
}

static qboolean HexToBinary( uint8_t* outBin, size_t binMaxLen, const char* inHex ) {
	if ( sodium_hex2bin( outBin, binMaxLen, inHex, strlen( inHex ), "", nullptr, nullptr ) != 0 ) {
		Com_Printf( "Hexadecimal -> Binary conversion failed!\n" );
		return qfalse;
	}

	return qtrue;
}

qboolean SV_GenerateCryptoKeys( publicKey_t* pk, secretKey_t* sk ) {
	if ( crypto_box_keypair( pk->keyBin, sk->keyBin ) ) {
        Com_Printf("Failed to generate public/secret key pair\n");
		return qfalse;
	}

    if ( !BinaryToHex( pk->keyHex, sizeof( pk->keyHex ), pk->keyBin, sizeof( pk->keyBin ) )
        || !BinaryToHex( sk->keyHex, sizeof( sk->keyHex ), sk->keyBin, sizeof( sk->keyBin ) ) ) {
        return qfalse;
    }

	return qtrue;
}

static qboolean LoadKeyFromFile( const char* filename, uint8_t* out, size_t outSize ) {
    fileHandle_t file = NULL_FILE;
    long size = FS_FOpenFileRead( filename, &file, qtrue );

    if ( !file ) {
        Com_Printf( "Failed to open key file for reading: %s\n", filename );
        return qfalse;
    }
    
    if ( size != ( long )outSize ) {
        // we need the key to have been saved with the exact same size
        Com_Printf( "Key file has incorrect size: %s. Expected %ld bytes, got %ld\n", filename, ( long )outSize, size );
        FS_FCloseFile( file );
        return qfalse;
    }

    FS_Read( out, outSize, file );
    FS_FCloseFile( file );

    return qtrue;
}

qboolean SV_LoadCryptoKeysFromFS( publicKey_t* pk, const char* pkFilename, secretKey_t* sk, const char* skFilename ) {
    qboolean success = qtrue;

    if ( pk ) {
        success = success
			&& LoadKeyFromFile( pkFilename, pk->keyBin, sizeof( pk->keyBin ) )
			&& BinaryToHex( pk->keyHex, sizeof( pk->keyHex ), pk->keyBin, sizeof( pk->keyBin ) )
			? qtrue : qfalse;
    }

    if ( sk ) {
        success = success
			&& LoadKeyFromFile( skFilename, sk->keyBin, sizeof( sk->keyBin ) )
			&& BinaryToHex( sk->keyHex, sizeof( sk->keyHex ), sk->keyBin, sizeof( sk->keyBin ) )
			? qtrue : qfalse;
    }

    return success;
}

static qboolean SaveKeyToFile( const char* filename, const uint8_t* in, size_t inSize ) {
	fileHandle_t file = FS_FOpenFileWrite( filename );

	if ( !file ) {
		Com_Printf( "Failed to open key file for writing: %s\n", filename );
		return qfalse;
	}

	FS_Write( in, inSize, file );
	FS_FCloseFile( file );

	return qtrue;
}

qboolean SV_SaveCryptoKeysToFS( publicKey_t* pk, const char* pkFilename, secretKey_t* sk, const char* skFilename ) {
	qboolean success = qtrue;

	if ( pk ) {
		success = success
			&& SaveKeyToFile( pkFilename, pk->keyBin, sizeof( pk->keyBin ) )
			? qtrue : qfalse;
	}

	if ( sk ) {
		success = success
			&& SaveKeyToFile( skFilename, sk->keyBin, sizeof( sk->keyBin ) )
			? qtrue : qfalse;
	}

	return success;
}

qboolean SV_EncryptString( publicKey_t* pk, const char* inRaw, char* outHex, size_t outHexSize ) {
	if ( strlen( inRaw ) > CRYPTO_CIPHER_RAW_SIZE - 1 ) {
		Com_Printf( "String is too large to be encrypted: %s\n", inRaw );
		return qfalse;
	}

	if ( outHexSize < CRYPTO_CIPHER_HEX_SIZE ) {
		Com_Printf( "Encryption output buffer is too small\n" );
		return qfalse;
	}

	uint8_t cipherBin[CRYPTO_CIPHER_BIN_SIZE];

	if ( crypto_box_seal( cipherBin, ( const unsigned char* )inRaw, CRYPTO_CIPHER_RAW_SIZE - 1, pk->keyBin ) != 0 ) {
		Com_Printf( "Failed to encrypt string: %s\n", inRaw );
		return qfalse;
	}

	if ( !BinaryToHex( outHex, outHexSize, cipherBin, sizeof( cipherBin ) ) ) {
		return qfalse;
	}

    return qtrue;
}

qboolean SV_DecryptString( publicKey_t* pk, secretKey_t* sk, const char* inHex, char* outRaw, size_t outRawSize ) {
	if ( strlen( inHex ) > CRYPTO_CIPHER_HEX_SIZE - 1 ) {
		Com_Printf( "String is too large to be decrypted: %s\n", inHex );
		return qfalse;
	}

	if ( outRawSize < CRYPTO_CIPHER_RAW_SIZE ) {
		Com_Printf( "Decryption output buffer is too small\n" );
		return qfalse;
	}

	uint8_t cipherBin[CRYPTO_CIPHER_BIN_SIZE];

	if ( !HexToBinary( cipherBin, sizeof( cipherBin ), inHex ) ) {
		return qfalse;
	}

	if ( crypto_box_seal_open( ( unsigned char* )outRaw, cipherBin, sizeof( cipherBin ), pk->keyBin, sk->keyBin ) != 0 ) {
		Com_Printf( "Failed to decrypt string: %s\n", inHex );
		return qfalse;
	}

    return qtrue;
}

qboolean SV_CryptoHash( const char* inRaw, char* outHex, size_t outHexSize ) {
	if ( outHexSize < CRYPTO_HASH_HEX_SIZE ) {
		Com_Printf( "Hash output buffer is too small\n" );
		return qfalse;
	}

	uint8_t hashBin[CRYPTO_HASH_BIN_SIZE];

	if ( crypto_generichash( hashBin, sizeof( hashBin ), ( const unsigned char* )inRaw, strlen( inRaw ), NULL, 0 ) != 0 ) {
		Com_Printf( "Failed to hash string: %s\n", inRaw );
		return qfalse;
	}

	if ( !BinaryToHex( outHex, outHexSize, hashBin, sizeof( hashBin ) ) ) {
		return qfalse;
	}

	return qtrue;
}