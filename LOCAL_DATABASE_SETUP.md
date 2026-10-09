# Game&Go — real local PostgreSQL edition

This edition uses a **real PostgreSQL server** and the C++ `PostgresDB` adapter. It does not use an in-memory database as a fallback. The SQL schema and development records are defined in `game_and_go_database.sql`; they are not hardcoded into C++.

## Windows: first run is one click

1. Extract the ZIP into a normal folder.
2. Double-click `GameAndGo-Windows.bat`.
3. The first run downloads the MSYS2 toolchain, installs the compiler/build libraries and PostgreSQL binaries, builds `App.exe`, initializes a local PostgreSQL database, and runs `game_and_go_database.sql` against that database.
4. After setup, double-click `GameAndGo-Windows.bat` each time you want to use the app. The executable is reused and the database is started automatically.

The first run needs internet to download developer/runtime dependencies. Normally no manual terminal commands or database configuration are required. If Windows refuses access while installing MSYS2 to `C:\msys64`, run the launcher as administrator for that initial setup. The application itself and its database run as the current user afterward.

The launcher uses PostgreSQL bound only to `127.0.0.1` on port `55439`. Its files are stored outside the project at:

```text
%LOCALAPPDATA%\GameAndGo\postgresql\data
```

This is an actual PostgreSQL data directory. It is not a copied database embedded into source code. The C++ application connects through `libpqxx` using the `GAMEGO_DB_CONN` connection string set by the launcher.

### SQL file behavior and data safety

- On a new/empty `gameandgo` database, the launcher runs the project's `game_and_go_database.sql` with `psql -f`. This creates the tables, indexes, extension, and development seed records.
- It runs that destructive schema script only when the database has **no public tables**.
- On an existing database with the older user schema, it uses `migrations/001_auth_management.sql`, which adds login fields without dropping the tables.
- On subsequent launches it leaves the schema and all user-created data alone. Closing the app or rebuilding `App.exe` does not delete the database.
- If the database is partly initialized or has an unknown/incomplete schema, the launcher stops and explains the issue instead of dropping tables automatically.

### Development logins

All seed accounts in a fresh database use the temporary development password `Welcome123!`.

| Role | Username |
|---|---|
| Admin | `alice.admin` |
| Admin | `john.admin` |
| Admin | `jane.admin` |
| Staff | `bob.staff` |
| Staff | `malak.staff` |
| Staff | `eve.staff` |
| Staff | `frank.staff` |
| Staff | `grace.staff` |
| Staff | `heidi.staff` |
| Customer | `charlie.customer` |
| Customer | `adam.customer` |
| Customer | `sandy.customer` |
| Customer | `oscar.customer` |
| Customer | `diana.customer` |

## Linux / developer setup

Install PostgreSQL, CMake, a C++20 compiler, GLFW, libpqxx, and OpenGL development packages using your distribution's package manager. Create an empty database named `gameandgo`, then run `game_and_go_database.sql` **only for a fresh database**. For an existing schema, use `migrations/001_auth_management.sql` instead.

The regular Linux developer default connection is:

```text
host=localhost port=5432 dbname=gameandgo user=postgres password=postgres
```

Override it with `GAMEGO_DB_CONN` if your local PostgreSQL connection is different. The program now exits with a clear database error if PostgreSQL is unavailable; it no longer silently switches to temporary in-memory storage.

Build/run on Linux:

```bash
cmake -S . -B build
cmake --build build -j"$(nproc)"
./build/App
```

## Database requirements

The schema needs PostgreSQL and the `pgcrypto` extension, which is included in the PostgreSQL package used by the Windows launcher. Passwords are stored as hashes using `crypt()` / `gen_salt()`. The `StandaloneDB` implementation remains in the codebase for isolated development work, but the application entry point does not use it as a substitute for PostgreSQL.
