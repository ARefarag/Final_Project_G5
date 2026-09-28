# Game&Go - Jira Task Breakdown

## Member 1 - Database Architect
- Create the PostgreSQL development database.
- Implement `branches`, `users`, `stations`, `reservations`, and `sessions`.
- Implement all PK/FK/NOT NULL/CHECK/ON DELETE rules from the SRS.
- Add and explain useful indexes.
- Insert development seed data.
- Implement/test CRUD queries.
- Implement/test required JOIN queries.
- Implement/test aggregate/report queries.
- Test invalid values and FK delete behavior in pgAdmin.
- Freeze the final schema contract before M4 integration.

## Member 2 - UI Developer
- Initialize Dear ImGui + GLFW + OpenGL.
- Implement login/user-selection screen.
- Implement dashboard.
- Implement stations screen.
- Implement reservations screen.
- Implement active sessions/tabs screen.
- Implement role-based controls through User capability methods.
- Implement status/error feedback.
- Test navigation and final GUI flow.

## Member 3 - Core Logic Engineer
- Implement enum conversion/parsing helpers.
- Implement time and money helpers.
- Implement User hierarchy and role permissions.
- Implement UserFactory.
- Implement StandardBillingStrategy.
- Implement VipBillingStrategy.
- Implement Reservation State Pattern.
- Test Factory, Strategy and State behavior independently.

## Member 4 - Database Integrator
- Implement DatabaseException.
- Implement PostgresDB connection lifecycle.
- Add pqxx only in `src/Database/PostgresDB.cpp`.
- Implement pqxx row-to-record mapping.
- Implement all IDatabase operations.
- Implement parameterized queries.
- Implement start/finish session transactions.
- Test NULLs, empty results, connection failures and database exceptions.

## Member 5 - Project Leader / Integrator
- Implement SessionManager dependency injection.
- Implement login/current-user handling.
- Implement branch/station/reservation/session loading calls.
- Implement reservation use cases using State Pattern.
- Implement session start/finish orchestration using Strategy Pattern.
- Implement `main.cpp` composition root.
- Complete CMake dependency wiring.
- Merge branches and resolve contract conflicts.
- Run end-to-end GUI -> Core -> IDatabase -> PostgreSQL tests.
