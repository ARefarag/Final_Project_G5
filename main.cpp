#include "Core/SessionManager.h"
#include "Database/PostgresDB.h"
#include "Presentation/ImGuiApp.h"
#include <iostream>
#include <memory>
#include <cstdlib>
#include <exception>
#include <string>

int main()
{
    std::cout << "Starting Game&Go Application...\n";

    // LOCAL EDITION: always use a real PostgreSQL database. The Windows launcher
    // starts the local server, initializes it from game_and_go_database.sql on
    // first use, and sets GAMEGO_DB_CONN before launching this executable.
    // Linux/local developers can still override the connection with GAMEGO_DB_CONN.
    const char *env_conn = std::getenv("GAMEGO_DB_CONN");
    const std::string conn_string = env_conn
        ? env_conn
        : "host=localhost port=5432 dbname=gameandgo user=postgres password=postgres";

    try
    {
        auto db = std::make_unique<PostgresDB>(conn_string);
        std::cout << "[SUCCESS] Connected to PostgreSQL.\n";

        auto session_manager = std::make_unique<SessionManager>(std::move(db));
        ImGuiApp app(*session_manager);
        app.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "[FATAL] Game&Go requires a working PostgreSQL database.\n";
        std::cerr << "Database error: " << ex.what() << "\n";
        std::cerr << "On Windows, start the app with Start_GameAndGo_Windows.bat so the local database is started and initialized from game_and_go_database.sql.\n";
        return 1;
    }

    return 0;
}
