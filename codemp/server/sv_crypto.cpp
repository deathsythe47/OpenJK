#include "server.h"
#include "sodium.h"

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
    /*fileHandle_t file = NULL_FILE;
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

    return qtrue;*/
    return qfalse;
}

qboolean SV_LoadCryptoKeysFromFS( publicKey_t* pk, const char* pkFilename, secretKey_t* sk, const char* skFilename ) {
    /*qboolean success = qtrue;

    if ( pk ) {
        success &= LoadKeyFromFile( pkFilename, pk->keyBin, ARRAY_LEN( pk->keyBin ) );
        success &= BinaryToHex( pk->keyHex, ARRAY_LEN( pk->keyHex ), pk->keyBin, ARRAY_LEN( pk->keyBin );
    }

    if ( sk ) {
        success &= LoadKeyFromFile( skFilename, sk->keyBin, ARRAY_LEN( sk->keyBin ) );
        success &= BinaryToHex( sk->keyHex, ARRAY_LEN( sk->keyHex ), sk->keyBin, ARRAY_LEN( sk->keyBin );
    }

    return success;*/
    return qfalse;
}

qboolean SV_SaveCryptoKeysToFS( publicKey_t* pk, const char* pkFilename, secretKey_t* sk, const char* skFilename ) {
    return qfalse;
}

qboolean SV_EncryptString( publicKey_t* pk, const char* inRaw, char* outHex, size_t outHexSize ) {
    return qfalse;
}

qboolean SV_DecryptString( publicKey_t* pk, secretKey_t* sk, const char* inHex, char* outRaw, size_t outRawSize ) {
    return qfalse;
}

qboolean SV_CryptoHash( const char* inRaw, char* outHex, size_t outHexSize ) {
    return qfalse;
}