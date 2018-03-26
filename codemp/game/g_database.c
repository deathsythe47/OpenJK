#include "g_local.h"
#include "database_sql.h"

void G_InitDatabase( void ) {
    trap->DB_ExecQuery( sqlCreateConnectionsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateAccountsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateNicknamesTable, NULL, NULL );
}