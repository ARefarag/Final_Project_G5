#include "Core/SessionManager.h"
#include "Database/PostgresDB.h"
#include "Database/StandaloneDB.h" // Includes our fallback adapter!
#include "Presentation/ImGuiApp.h"
#include <iostream>
#include <memory>
#include <cstdlib>

int main()
{
    std::cout << "Starting Game&Go Application...\n";

    const char *env_conn = std::getenv("GAMEGO_DB_CONN");
    std::string conn_string = env_conn ? env_conn : "host=localhost port=5432 dbname=gameandgo user=postgres password=postgres";

    std::unique_ptr<IDatabase> db = nullptr;

    // TRY CONNECTING TO POSTGRESQL FIRST
    try
    {
        db = std::make_unique<PostgresDB>(conn_string);
        std::cout << "[SUCCESS] Connected to live PostgreSQL server!\n";
    }
    catch (const std::exception &ex)
    {
        // IF POSTGRESQL IS NOT RUNNING, DO NOT CRASH! FALL BACK AUTOMATICALLY!
        std::cout << "[NOTICE] PostgreSQL server not running (" << ex.what() << ").\n";
        std::cout << "[NOTICE] Falling back to Portable Standalone Mode...\n";
        db = std::make_unique<StandaloneDB>();
    }

    try
    {
        auto session_manager = std::make_unique<SessionManager>(std::move(db));
        ImGuiApp app(*session_manager);
        app.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Application Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}