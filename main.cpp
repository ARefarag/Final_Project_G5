#include "Core/SessionManager.h"
#include "Database/PostgresDB.h"
#include "Presentation/ImGuiApp.h"
#include <iostream>
#include <memory>
#include <cstdlib>

int main() {
    std::cout << "Starting Game&Go Application...\n";

    const char* env_conn = std::getenv("GAMEGO_DB_CONN");
    std::string conn_string = env_conn ? env_conn : "host=localhost port=5432 dbname=gameandgo user=postgres password=postgres";

    try {
        auto db = std::make_unique<PostgresDB>(conn_string);
        auto session_manager = std::make_unique<SessionManager>(std::move(db));

        ImGuiApp app(*session_manager);
        app.run();

    } catch (const std::exception& ex) {
        std::cerr << "Initialization Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
