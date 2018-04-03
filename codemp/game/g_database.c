#include "g_local.h"
#include "database_sql.h"

void G_InitDatabase( void ) {
    trap->DB_ExecQuery( sqlCreateConnectionsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateAccountsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateNicknamesTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateFastcapsTable, NULL, NULL );

    publicKey_t ptest;
    secretKey_t stest;

    trap->Crypto_GenerateKeys( &ptest, &stest );

    trap->Print("!!!!!!!!!!!!!!!PUB: %s\n", ptest.keyHex);
    trap->Print("!!!!!!!!!!!!!!!SEC: %s\n", stest.keyHex);
}