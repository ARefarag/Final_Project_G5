GAME&GO — LOCAL POSTGRESQL EDITION

To start on Windows, double-click:
    GameAndGo-Windows.bat

First launch:
- Downloads the MSYS2 UCRT64 compiler/build tools and PostgreSQL package if missing.
- Builds the C++ GUI once for this computer.
- Starts a local PostgreSQL server on 127.0.0.1:55439.
- Creates the gameandgo database and runs game_and_go_database.sql only if the database is empty.

Next launches reuse the compiled executable and stored PostgreSQL data.
Do not delete %LOCALAPPDATA%\GameAndGo\postgresql\data unless you intend to delete this computer's local database.

Seed development accounts use password: Welcome123!
Admin: alice.admin
Staff: bob.staff
Customer: charlie.customer

If setup fails, the launcher prints the first error. PostgreSQL server log:
%LOCALAPPDATA%\GameAndGo\logs\postgresql.log
Schema setup log:
%LOCALAPPDATA%\GameAndGo\logs\schema-initialization.log

If first-time setup reports a pacman database lock (db.lck), the launcher now waits for an active MSYS2 package transaction. If no pacman process is running and the lock is stale, it removes only that stale lock and retries. Close any other MSYS2/UCRT64 update windows before retrying.
