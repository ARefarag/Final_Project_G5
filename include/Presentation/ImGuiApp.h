// ============================================================================
// GAME&GO - DEAR IMGUI APPLICATION
// OWNER: MEMBER 2
// ============================================================================
#pragma once

class GLFWwindow;
class SessionManager;

class ImGuiApp {
private:
    SessionManager& session_manager_;
    GLFWwindow* window_{};
    bool show_login_{};
    int active_screen_{};

    // TODO [M2]: Add presentation-only state here: text buffers, selected IDs,
    //             filters, modal flags, and transient notification data.

public:
    // TODO [M2]: Store SessionManager reference. Do not store PostgresDB.
    explicit ImGuiApp(SessionManager& session_manager);

    // TODO [M2]: Initialize ImGui context/style/backends using the provided window.
    bool initialize(GLFWwindow* window);

    // TODO [M2]: Begin/end frame and route to the correct screen renderer.
    void render();

    // TODO [M2]: Show branch summary, station counts, active tabs, and role-allowed reports.
    void renderDashboard();

    // TODO [M2]: Show station type/rate/status and management controls when permitted.
    void renderStations();

    // TODO [M2]: Create/list/confirm/cancel reservations using SessionManager.
    void renderReservations();

    // TODO [M2]: Show active sessions, elapsed time, cost preview, and finish controls.
    void renderActiveSessions();

    // TODO [M2]: Demonstration login/user-selection screen.
    void renderLogin();

    // TODO [M2]: Use User capability methods instead of duplicating role if/else logic.
    void renderRoleControls();

    // TODO [M2]: Display error/success feedback without crashing the render loop.
    void renderStatusMessages();

    // TODO [M2]: Release ImGui resources. Member 5 controls GLFW lifetime.
    void shutdown();
};
