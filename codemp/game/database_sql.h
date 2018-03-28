#pragma once

// contains SQL strings used for all database operations

// ------------ Database schema ------------

// A "connection" is defined as a unique pair of IP+CUID that connected
// to the server.
//
// Since any combination of IP/CUID may change over time for a given player,
// it is possible to link two different records. Server admins are given this
// possibility using a variety of commands. The original records are not deleted,
// thus it is possible to unlink them if a mistake has been made. Other tables
// must follow links as if it was a "redirection" so that the end result
// appears as if the players have been effectively "merged", while internally
// they are just linked.
//
// For a small sized player base, once the database is stable and the regular
// players are no longer temporary records, the amount of work required from
// admins should be minimal as linking should be a rare occurence. The only,
// and most frequent event, is handled automatically: an IP change with no CUID
// change. The new record is automatically linked to records with the same CUID,
// as doing so cannot possibly be a mistake. All other events could result in
// mistakes if handled automatically.
//
// Theoretically, this system can work without Newmod CUIDs; however, they
// both allow automatic merging in most cases and solving conflicts where
// different players share the same IP (VPN, same router...). For the best
// results, the player base should mostly use Newmod.
//
// When a new unique pair connects to the server, a new record is always
// created as temporary until anything that requires permanent storage happens.
// This avoids keeping unnecessary records for too long. Temporary records
// that haven't connected in 30 days are removed.
//
// A connection may point to an account record specifically created as
// part of the account system. This is what allows "auto-login" in that
// system, since multiple records can link to one record that is in turn
// linked to an account.
//
// In this system, a connection is the broadest identifier for a player.
// Ideally, other tables should refer to a connection ID when storing
// generic stuff, since everyone has such an ID. If needed, they can then
// use other fields such as the account to narrow it down.
static const char* sqlCreateConnectionsTable =
"CREATE TABLE IF NOT EXISTS [connections] ("
"    [connection_id] INTEGER NOT NULL,"
"    [ip_int] INTEGER NOT NULL,"
"    [cuid_hash] TEXT,"
"    [last_seen] INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),"
"    [playtime] INTEGER NOT NULL DEFAULT 0,"
"    [linked_connection] INTEGER DEFAULT NULL,"
"    [linked_account] INTEGER DEFAULT NULL,"
"    [temporary] INTEGER NOT NULL DEFAULT 1,"
"    PRIMARY KEY ( [connection_id] ),"
"    UNIQUE ( [ip_int], [cuid_hash] ),"
"    UNIQUE ( [linked_account] ),"
"    FOREIGN KEY ( [linked_connection] ) REFERENCES connections ( [connection_id] ) ON DELETE SET DEFAULT,"
"    FOREIGN KEY ( [linked_account] ) REFERENCES accounts ( [account_id] ) ON DELETE SET DEFAULT,"
"    CHECK ( ( [linked_connection] IS NULL ) OR ( [linked_account] IS NULL ) )"
");";

// An "account" is specifically tied to a physical player and not his IP/CUID.
//
// Accounts are entirely managed by admins. There is no register or login command.
// They are completely transparent to players who may not even be aware that they
// exist. They are simply here to identify more "permanent" community members and
// to abstract multiple connections into a single record that "unifies" players.
// For instance, detailed game statistics are only saved for accounts.
//
// On top of life quality improvements, accounts are given a group identifier.
// The meaning of this group is managed internally by the mod: permissions,
// admin commands, admin broadcasts...
//
// Since accounts point to a connection record, they are not subject to IP/CUID
// changes as long as connections are properly linked.
//
// Binding an account to a connection is one of the ways to make it permanent.
static const char* sqlCreateAccountsTable =
"CREATE TABLE IF NOT EXISTS [accounts] ("
"    [account_id] INTEGER NOT NULL,"
"    [linked_connection] INTEGER NOT NULL,"
"    [name] TEXT NOT NULL,"
"    [group] TEXT DEFAULT NULL,"
"    PRIMARY KEY ( [account_id] ),"
"    UNIQUE ( [linked_connection] ),"
"    FOREIGN KEY ( [linked_connection] ) REFERENCES connections ( [connection_id] ) ON DELETE RESTRICT"
");";

// Nicknames are recorded for all connections to identify them more easily
// if needed. The overhead should be minimal since these will be rarely be read
// and mostly just written to.
static const char* sqlCreateNicknamesTable =
"CREATE TABLE IF NOT EXISTS [nicknames] ("
"    [name] TEXT NOT NULL,"
"    [duration] INTEGER NOT NULL DEFAULT 0,"
"    [linked_connection] INTEGER NOT NULL,"
"    UNIQUE ( [name], [linked_connection] ),"
"    FOREIGN KEY ( [linked_connection] ) REFERENCES connections ( [connection_id] ) ON DELETE CASCADE"
");";

static const char* sqlCreateFastcapsTable =
"CREATE TABLE IF NOT EXISTS [fastcaps] ("
"    [fastcap_id] INTEGER NOT NULL,"
"    [mapname] TEXT NOT NULL,"
"    [type] INTEGER NOT NULL,"
"    [time] INTEGER NOT NULL,"
"    [linked_connection] INTEGER NOT NULL,"
"    [capture_time_ms] INTEGER NOT NULL,"
"    [whose_flag] INTEGER NOT NULL,"
"    [pickup_speed] INTEGER NOT NULL,"
"    [capture_speed] INTEGER NOT NULL,"
"    [max_speed] INTEGER NOT NULL,"
"    [average_speed] INTEGER NOT NULL,"
"    [demo_match_id] TEXT,"
"    [demo_client_name] TEXT NOT NULL,"
"    [demo_client_id] INTEGER NOT NULL,"
"    [demo_pickup_time] INTEGER NOT NULL,"
"    PRIMARY KEY ( [fastcap_id] ),"
"    FOREIGN KEY ( [linked_connection] ) REFERENCES connections ( [connection_id] ) ON DELETE RESTRICT"
");";