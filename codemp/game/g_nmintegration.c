#include "g_local.h"

#define NM_AUTH_PROTOCOL		5
#define PUBLIC_KEY_FILENAME     "public_key.bin"
#define SECRET_KEY_FILENAME		"secret_key.bin"


void G_SendNMServerCommand( int clientNum, const char* cmd, const char* argsFmt, ... ) {
    if ( !VALIDSTRING( cmd ) ) {
        return;
    }

    if ( clientNum < 0 || clientNum > MAX_CLIENTS ) {
        return;
    }

    char buf[MAX_STRING_CHARS];
    size_t size = sizeof( buf );

    size -= Com_sprintf( buf, size, "kls -1 -1 \"%s\"", cmd );

    if ( size > 1 && VALIDSTRING( argsFmt ) ) {
        va_list argPtr;

        // add a space before writing arguments
        char* p = buf + size;
        *p++ = ' ';
        --size;

        va_start( argPtr, argsFmt );
	    Q_vsnprintf( p, size, argsFmt, argPtr );
	    va_end( argPtr );
    }

    trap->SendServerCommand( clientNum, buf );
}

void G_InitNMAuth( void ) {
    level.nmAuthEnabled = qtrue;

    if ( trap->Crypto_LoadKeysFromFS( &level.publicKey, PUBLIC_KEY_FILENAME, &level.secretKey, SECRET_KEY_FILENAME ) ) {
        trap->Print( "Loaded crypto key files from disk successfully\n" );
    } else {
        trap->Print( "Failed to read crypto key files from disk! (ignore if first run)\n" );

        if ( trap->Crypto_GenerateKeys( &level.publicKey, &level.secretKey ) ) {
            trap->Print( "Generated new crypto key pair successfully\n" );
            trap->Crypto_SaveKeysToFS( &level.publicKey, PUBLIC_KEY_FILENAME, &level.secretKey, SECRET_KEY_FILENAME ); // save to disk
        } else {
            trap->Print( "Failed to generate a new crypto key pair!\n" );
            level.nmAuthEnabled = qfalse;
        }
    }

	if ( !level.nmAuthEnabled ) {
		trap->Print( "Newmod authentication support was disabled. Some functionality will be unavailable for Newmod clients.\n" );
	}
}

void G_NMAuthAnnounce( gentity_t* ent ) {
    if ( ent->client->sess.nmAuthState != NM_AUTH_PENDING ) {
        return;
    }

    // we "announce" the server auth protocol along with our public key so that
    // the client knows that they should send an initial svauth cmd

     G_SendNMServerCommand( ent - g_entities, "clannounce", "%d \"%s\"", NM_AUTH_PROTOCOL, level.publicKey.keyHex );
	ent->client->sess.nmAuthState++;
#ifdef _DEBUG
	trap->Print( "Sent clannounce packet to client %d\n", ent - g_entities );
#endif
}

static qboolean GetKeyFromInfoString( const char* str, const char* keyName, int* outKey ) {
    char* s = Info_ValueForKey( str, keyName );

    if ( !VALIDSTRING( s ) ) {
        return qfalse;
    }

    *outKey = atoi( s );

    return qtrue;
}

void G_NMAuthSendVerification( gentity_t* ent, const char* encryptedMsg ) {
    if ( ent->client->sess.nmAuthState != NM_AUTH_CLANNOUNCE ) {
        return;
    }

    // we are processing the initial svauth cmd, which contains two keys that we
    // should xor to prove that we own the secret key associated with the public
    // key that we sent earlier.
    // we then send the result along with two keys that the client should also xor

    do {
        char decryptedMsg[CRYPTO_CIPHER_RAW_SIZE];

        if ( !trap->Crypto_DecryptString( &level.publicKey, &level.secretKey, encryptedMsg, decryptedMsg, sizeof( decryptedMsg ) ) ) {
            //
            break;
        }

        int clientKeys[2];
        
        if ( !GetKeyFromInfoString( decryptedMsg, "ck1", &clientKeys[0] ) ) {
            //
            break;
        }

        if ( !GetKeyFromInfoString( decryptedMsg, "ck2", &clientKeys[1] ) ) {
            //
            break;
        }

        #define RandomConfirmationKey()	( ( rand() << 16 ) ^ rand() ^ trap->Milliseconds() )
        ent->client->sess.nmAuthServerKeys[0] = RandomConfirmationKey();
		ent->client->sess.nmAuthServerKeys[1] = RandomConfirmationKey();
        #undef RandomConfirmationKey

        G_SendNMServerCommand( ent - g_entities, "clauth", "%d %d %d",
            clientKeys[0] ^ clientKeys[1],
            ent->client->sess.nmAuthServerKeys[0],
            ent->client->sess.nmAuthServerKeys[1]
        );

        ent->client->sess.nmAuthState++;

        return;
    } while ( 0 );

    // if we got here, we failed somewhere
    ent->client->sess.nmAuthState = NM_AUTH_FAILED;
}

void G_NMAuthFinalize( gentity_t* ent, const char* encryptedMsg ) {
    if ( ent->client->sess.nmAuthState != NM_AUTH_CLAUTH ) {
        return;
    }

    // we are processing the second and last svauth cmd, which must contain the
    // correct xor of the keys we sent earlier, along with the unique hardware
    // client id. If this is successful, then auth is complete for this client

    do {
        char decryptedMsg[CRYPTO_CIPHER_RAW_SIZE];

        if ( !trap->Crypto_DecryptString( &level.publicKey, &level.secretKey, encryptedMsg, decryptedMsg, sizeof( decryptedMsg ) ) ) {
            //
            break;
        }

        int serverKeysXor;

        if ( !GetKeyFromInfoString( decryptedMsg, "skx", &serverKeysXor ) ) {
            //
            break;
        }

        char* uniqueClientId = Info_ValueForKey( decryptedMsg, "cid" );

        if ( !VALIDSTRING( uniqueClientId ) ) {
            //
            break;
        }

        if ( ( ent->client->sess.nmAuthServerKeys[0] ^ ent->client->sess.nmAuthServerKeys[1] ) != serverKeysXor ) {
            //
			break;
		}
        
        trap->Crypto_Hash( uniqueClientId, ent->client->sess.nmCuidHash, sizeof( ent->client->sess.nmCuidHash ) );
        // print success
        ent->client->sess.nmAuthState++; // authentication is now complete
        ClientUserinfoChanged( ent - g_entities ); // trigger a userinfo change to broadcast the id

        return;
    } while ( 0 );

    // if we got here, we failed somewhere
    ent->client->sess.nmAuthState = NM_AUTH_FAILED;
}