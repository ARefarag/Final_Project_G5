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

    // ADDED: username/password login fields (labels render above the fields).
    char login_username_[64] = "";
    char login_password_[128] = "";

    // ADDED: account-management form state.
    char new_user_name_[100] = "";
    char new_user_username_[64] = "";
    char new_user_password_[128] = "";
    char new_user_phone_[32] = "";
    int new_user_role_idx_{0};
    int new_user_branch_idx_{0};

    // ADDED: branch creation form state.
    char new_branch_name_[100] = "";
    char new_branch_address_[150] = "";
    char new_branch_district_[100] = "";
    char new_branch_city_[100] = "";
    char new_branch_country_[100] = "Egypt";

    // ADDED: station creation form state.
    int new_station_branch_idx_{0};
    int new_station_type_idx_{0};
    char new_station_rate_[32] = "10.00";

    // Data cache to avoid hammering PostgreSQL at 60 FPS
    bool needs_refresh_{true};
    std::vector<BranchRecord> cached_branches_;
    std::vector<UserRecord> cached_users_;      // ADDED: staff/customer account list
    std::vector<UserRecord> cached_customers_;  // ADDED: live customers for station sessions
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

    // ADDED: normal username/password login screen.
    void renderLogin();

    // ADDED: create staff/customers, branches, and stations according to role.
    void renderManagement();

    // Use User capability methods instead of duplicating role if/else logic.
    void renderRoleControls();

    // Display error/success feedback without crashing the render loop.
    void renderStatusMessages();

    // Release ImGui resources. Member 5 controls GLFW lifetime.
    void shutdown();

private:
    void refreshData();
};