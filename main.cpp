// ============================================================================
// GAME&GO - COMPOSITION ROOT TEMPLATE
// OWNER: MEMBER 5
//
// This file intentionally contains NO completed application logic.
// Its job is to eventually create the concrete database adapter, inject it into
// SessionManager, create ImGuiApp, create the GLFW/OpenGL window, and run the
// main loop after all team members finish their assigned TODOs.
// ============================================================================

#include "Core/SessionManager.h"
#include "Database/PostgresDB.h"
#include "Presentation/ImGuiApp.h"

// TODO [M5]: Implement main() only after the interfaces are agreed and the
//             PostgreSQL/CMake/ImGui dependencies are available.
// Implementation plan for M5:
//   1. Obtain the PostgreSQL connection string from the agreed local config.
//   2. Create PostgresDB as a concrete IDatabase object.
//   3. Inject it into SessionManager using dependency inversion.
//   4. Create the GLFW/OpenGL window.
//   5. Construct ImGuiApp with the SessionManager reference.
//   6. Initialize ImGui, run the application loop, then shut down cleanly.
// Do NOT put SQL, pqxx query code, billing logic, or GUI rendering code here.
int main();
