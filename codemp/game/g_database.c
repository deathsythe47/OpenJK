#include "g_local.h"
#include "database_sql.h"

void G_InitDatabase( void ) {
    trap->DB_ExecQuery( sqlCreateConnectionsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateAccountsTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateNicknamesTable, NULL, NULL );
    trap->DB_ExecQuery( sqlCreateFastcapsTable, NULL, NULL );

	int test = 1337;
	trap->DB_SetData( "test_stored_stuff", &test, sizeof( test ) );
	const int* test1 = ( const int* )trap->DB_GetData( "test_stored_stuff", NULL, qfalse ); // 1337
	const int* test2 = ( const int* )trap->DB_GetData( "test_stored_stuff", NULL, qtrue ); // 1337
	const int* test3 = ( const int* )trap->DB_GetData( "test_stored_stuff", NULL, qtrue ); // NULL
}