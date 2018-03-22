#include "server.h"
#include "sqlite3/sqlite3.h"

static const char* serverDBFileName = "enhanced_data.db";

static sqlite3* db = nullptr;

void SV_InitDB() {
    if ( db ) {
        return;
    }

    int rc;

    if ( ( rc = sqlite3_initialize() ) != SQLITE_OK ) {
        Com_Printf( "Failed to initialize SQLite (error: %d). Database support will be unavailable during this session\n", rc );
        return;
    }

    if ( ( rc = sqlite3_open( serverDBFileName, &db ) ) != SQLITE_OK ) {
        Com_Printf( "Coudln't open database file '%s' (error: %d). Database support will be unavailable during this session\n", serverDBFileName, rc );
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

void SV_ExecDBQuery( const char* query ) {
    if ( !db ) {
        return;
    }

    sqlite3_exec( db, query, nullptr, nullptr, nullptr );
}