// ============================================================================
// GAME&GO - DEAR IMGUI APPLICATION
// OWNER: MEMBER 2
// ============================================================================
#pragma once

#include <string>
#include <vector>
#include "../CoreData.h"

class GLFWwindow;
class SessionManager;

class ImGuiApp
{
private:
    SessionManager &session_manager_;
    GLFWwindow *window_{};
    bool show_login_{true};
    int active_screen_{0};

    // Presentation-only state
    std::string status_message_;
    int selected_branch_idx_{0};
    int input_start_session_user_id_{10}; // Default to Charlie Customer (ID 10 from seed data)
    int input_res_type_idx_{0};
    char input_res_time_[64] = "2026-06-15 18:00:00";
    char input_res_deposit_[32] = "10.00";

    // Data cache to avoid hammering PostgreSQL at 60 FPS
    bool needs_refresh_{true};
    std::vector<BranchRecord> cached_branches_;
    std::vector<StationRecord> cached_stations_;
    std::vector<ReservationRecord> cached_reservations_;
    std::vector<ActiveSessionView> cached_sessions_;

    // Background animation sprites
    std::vector<unsigned int> bg_frames_;

public:
    // Store SessionManager reference. Do not store PostgresDB.
    explicit ImGuiApp(SessionManager &session_manager);

    // Main application runner
    void run();

    // Initialize ImGui context/style/backends using the provided window.
    bool initialize(GLFWwindow *window);

    // Begin/end frame and route to the correct screen renderer.
    void render();

    // Show branch summary, station counts, active tabs, and role-allowed reports.
    void renderDashboard();

    // Show station type/rate/status and management controls when permitted.
    void renderStations();

    // Create/list/confirm/cancel reservations using SessionManager.
    void renderReservations();

    // Show active sessions, elapsed time, cost preview, and finish controls.
    void renderActiveSessions();

    // Demonstration login/user-selection screen.
    void renderLogin();

    // Use User capability methods instead of duplicating role if/else logic.
    void renderRoleControls();

    // Display error/success feedback without crashing the render loop.
    void renderStatusMessages();

    // Release ImGui resources. Member 5 controls GLFW lifetime.
    void shutdown();

private:
    void refreshData();
};