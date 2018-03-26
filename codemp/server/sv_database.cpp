#include "server.h"
#include "sqlite3/sqlite3.h"

static const char* serverDBFileName = "enhanced_data.db";

static sqlite3* db = nullptr;

static void SQLErrorCallback(void* userData, int code, const char* msg) {
	Com_Printf( "SQL error %d: %s\n", code, msg );
}

void SV_InitDB() {
    if ( db ) {
        return;
    }

	sqlite3_config( SQLITE_CONFIG_LOG, SQLErrorCallback, nullptr );
	sqlite3_config( SQLITE_CONFIG_SINGLETHREAD );

    int rc;

    if ( ( rc = sqlite3_initialize() ) != SQLITE_OK ) {
        Com_Printf( "Failed to initialize SQLite (error: %d). Database support will be unavailable during this session\n", rc );
        return;
    }

    if ( ( rc = sqlite3_open( serverDBFileName, &db ) ) != SQLITE_OK ) {
        Com_Printf( "Couldn't open database file '%s' (error: %d). Database support will be unavailable during this session\n", serverDBFileName, rc );
        db = nullptr;
        return;
    }

    Com_Printf( "Loaded database file successfully\n" );
}

void SV_CloseDB() {
    if ( db ) {
        sqlite3_close( db );
        db = nullptr;
    }

    sqlite3_shutdown();
}

typedef struct proxyUserData_t {
	DBResultCallback actualCallback;
	void* actualUserData;
} proxyUserData_s;

// this proxy callback allows us to have a nicer function sig in gamecode and potentially
// do more stuff with the data in the future
static int ProxyQueryCallback( void* data, int numCols, char** colValues, char** colNames ) {
	int ret = 0;
	proxyUserData_t* proxyData = ( proxyUserData_t* )data;

	if ( proxyData->actualCallback ) {
		ret = !proxyData->actualCallback( numCols, ( const char** )colNames, ( const char** )colValues, proxyData->actualUserData );
	}

	return ret;
}

qboolean SV_ExecDBQuery( const char* sql, DBResultCallback callback, void* userData ) {
    if ( !db ) {
        return qfalse;
    }

	proxyUserData_t proxyUserData;
	proxyUserData.actualCallback = callback;
	proxyUserData.actualUserData = userData;

	char* errorMsg = nullptr;

	int rc = sqlite3_exec( db, sql, ProxyQueryCallback, &proxyUserData, &errorMsg );

	if ( errorMsg ) {
		Com_Printf( "SQL error: %s\n", errorMsg );
		Com_Printf( "Query: %s\n", sql );
		sqlite3_free( errorMsg );
	}

	return rc == SQLITE_OK ? qtrue : qfalse;
}

// interfaces for the statement wrapper

static qboolean BindString_Internal( dbStmt_t* stmt, int colIndex, const char* value ) {
	return sqlite3_bind_text( ( sqlite3_stmt* )( stmt->handle ), colIndex, value, -1, nullptr ) == SQLITE_OK ? qtrue : qfalse;
}

static qboolean BindInt32_Internal( dbStmt_t* stmt, int colIndex, int32_t value ) {
	return sqlite3_bind_int( ( sqlite3_stmt* )( stmt->handle ), colIndex, value ) == SQLITE_OK ? qtrue : qfalse;
}

static qboolean BindInt64_Internal( dbStmt_t* stmt, int colIndex, int64_t value ) {
	return sqlite3_bind_int64( ( sqlite3_stmt* )( stmt->handle ), colIndex, value ) == SQLITE_OK ? qtrue : qfalse;
}

static qboolean BindDouble_Internal( dbStmt_t* stmt, int colIndex, double value ) {
	return sqlite3_bind_double( ( sqlite3_stmt* )( stmt->handle ), colIndex, value ) == SQLITE_OK ? qtrue : qfalse;
}

static qboolean BindNull_Internal( dbStmt_t* stmt, int colIndex ) {
	return sqlite3_bind_null( ( sqlite3_stmt* )( stmt->handle ), colIndex ) == SQLITE_OK ? qtrue : qfalse;
}

static qboolean Step_Internal( dbStmt_t* stmt ) {
	int rc = sqlite3_step( ( sqlite3_stmt* )( stmt->handle ) );

	if ( rc == SQLITE_ROW ) {
		return qtrue; // this resulted in a row to process
	} else if ( rc == SQLITE_DONE ) {
		return qfalse; // no row/no more rows
	}/* else if ( rc == SQLITE_BUSY ) {
		// TODO: do we need to handle this case and rollback the transaction?
	} */else {
		Com_Printf( "An error occured while stepping through SQL statement (error: %d)\n", rc );
		return qfalse;
	}
}

static qboolean StepAll_Internal( dbStmt_t* stmt, DBResultCallback callback, void* userData ) {
	int rc;
	sqlite3_stmt* handle = ( sqlite3_stmt* )stmt->handle;

	while ( ( rc = sqlite3_step( handle ) ) == SQLITE_ROW ) {
		if ( !callback ) {
			continue;
		}

		int numCols = sqlite3_column_count( handle );

		if ( numCols ) {
			const char** colNames = ( const char** )Z_Malloc( sizeof( char* ) * numCols, TAG_GENERAL );
			const char** colValues = ( const char** )Z_Malloc( sizeof( char* ) * numCols, TAG_GENERAL );

			for ( int i = 0; i < numCols; ++i ) {
				colNames[i] = sqlite3_column_name( handle, i );
				colValues[i] = ( const char* )sqlite3_column_text( handle, i + 1 );
			}

			qboolean doContinue = callback( numCols, colNames, colValues, userData );

			Z_Free( colNames );
			Z_Free( colValues );

			if ( !doContinue ) {
				rc = SQLITE_ABORT;
				break;
			}
		}
	}

	if ( rc != SQLITE_DONE ) {
		Com_Printf( "An error occured while stepping through SQL statement (error: %d)\n", rc );
		return qfalse;
	}

	return qtrue;
}

static const char* GetString_Internal( dbStmt_t* stmt, int colIndex ) {
	return ( const char* )sqlite3_column_text( ( sqlite3_stmt* )( stmt->handle ), colIndex );
}

static int32_t GetInt32_Internal( dbStmt_t* stmt, int colIndex ) {
	return sqlite3_column_int( ( sqlite3_stmt* )( stmt->handle ), colIndex );
}

static int64_t GetInt64_Internal( dbStmt_t* stmt, int colIndex ) {
	return sqlite3_column_int64( ( sqlite3_stmt* )( stmt->handle ), colIndex );
}

static double GetDouble_Internal( dbStmt_t* stmt, int colIndex ) {
	return sqlite3_column_double( ( sqlite3_stmt* )( stmt->handle ), colIndex );
}

static void Reset_Internal( dbStmt_t* stmt ) {
	sqlite3_reset( ( sqlite3_stmt* )( stmt->handle ) );
}

static void Clear_Internal( dbStmt_t* stmt ) {
	sqlite3_clear_bindings( ( sqlite3_stmt* )( stmt->handle ) );
}

dbStmt_t* SV_CreateDBStatement( const char* sql ) {
	if ( !db ) {
		return nullptr;
	}

	int rc;
	sqlite3_stmt* statement;

	if ( ( rc = sqlite3_prepare_v2( db, sql, -1, &statement, nullptr ) ) != SQLITE_OK ) {
		Com_Printf( "Failed to prepare SQL statement (error: %d)\n", rc );
		return nullptr;
	}

	if ( !statement ) {
		return nullptr; // this can happen when an empty sql string is passed
	}

	// prepare the statement wrapper and return it

	dbStmt_t* result = ( dbStmt_t* )Z_Malloc( sizeof( dbStmt_t ), TAG_GENERAL );
	result->handle = statement;

	result->BindString = BindString_Internal;
	result->BindInt32 = BindInt32_Internal;
	result->BindInt64 = BindInt64_Internal;
	result->BindDouble = BindDouble_Internal;
	result->BindNull = BindNull_Internal;
	result->Step = Step_Internal;
	result->StepAll = StepAll_Internal;
	result->GetString = GetString_Internal;
	result->GetInt32 = GetInt32_Internal;
	result->GetInt64 = GetInt64_Internal;
	result->GetDouble = GetDouble_Internal;
	result->Reset = Reset_Internal;
	result->Clear = Clear_Internal;
	
	return result;
}

void SV_FreeDBStatement( dbStmt_t* stmt ) {
	if ( !stmt ) {
		return;
	}

	if ( stmt->handle ) {
		sqlite3_finalize( ( sqlite3_stmt* )stmt->handle );
	}

	Z_Free( stmt );
}