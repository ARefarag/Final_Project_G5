// ============================================================================
// GAME&GO - DEAR IMGUI APPLICATION (IMPLEMENTATION)
// OWNER: MEMBER 2
// ============================================================================
#include "Presentation/ImGuiApp.h"
#include "Core/SessionManager.h"
#include "Core/Helpers.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <iostream>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "../../stb_image.h"

bool LoadTextureFromFile(const char *filename, unsigned int *out_texture)
{
    int image_width = 0, image_height = 0;
    unsigned char *image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    unsigned int image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    return true;
}

ImGuiApp::ImGuiApp(SessionManager &session_manager)
    : session_manager_(session_manager) {}

void ImGuiApp::run()
{
    if (!glfwInit())
        return;
    GLFWwindow *window = glfwCreateWindow(1024, 768, "GAME&GO", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!initialize(window))
        return;

    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        render();
        glfwSwapBuffers(window_);
    }

    shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool ImGuiApp::initialize(GLFWwindow *window)
{
    window_ = window;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    if (FILE *f = fopen("external/imgui/misc/fonts/Roboto-Medium.ttf", "r"))
    {
        fclose(f);
        io.Fonts->AddFontFromFileTTF("external/imgui/misc/fonts/Roboto-Medium.ttf", 16.0f);
    }
    else if (FILE *f = fopen("../external/imgui/misc/fonts/Roboto-Medium.ttf", "r"))
    {
        fclose(f);
        io.Fonts->AddFontFromFileTTF("../external/imgui/misc/fonts/Roboto-Medium.ttf", 16.0f);
    }

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    const char *sprite_files[] = {
        "Sprite-0001.png", "Sprite-0004.png", "Sprite-0005.png",
        "Sprite-0006.png", "Sprite-0007.png", "Sprite-0008.png", "Sprite-0009.png"};
    const int total_sprites = sizeof(sprite_files) / sizeof(sprite_files[0]);

    for (int i = 0; i < total_sprites; i++)
    {
        unsigned int tex = 0;
        if (LoadTextureFromFile(sprite_files[i], &tex))
        {
            bg_frames_.push_back(tex);
        }
        else
        {
            std::string fallback = std::string("../") + sprite_files[i];
            if (LoadTextureFromFile(fallback.c_str(), &tex))
                bg_frames_.push_back(tex);
        }
    }

    return true;
}

void ImGuiApp::shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiApp::refreshData()
{
    try
    {
        cached_branches_ = session_manager_.loadBranches();
        auto branch_id = session_manager_.getSelectedBranch();
        if (branch_id.has_value())
        {
            cached_stations_ = session_manager_.loadStations(branch_id.value(), std::nullopt);
            cached_reservations_ = session_manager_.loadReservations(branch_id.value(), std::nullopt);
        }
        cached_sessions_ = session_manager_.loadActiveSessions();
    }
    catch (const std::exception &e)
    {
        status_message_ = std::string("DB Error: ") + e.what();
    }
}

void ImGuiApp::render()
{
    if (needs_refresh_ && !show_login_)
    {
        refreshData();
        needs_refresh_ = false;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    int win_w, win_h;
    glfwGetFramebufferSize(window_, &win_w, &win_h);

    float scale = (float)win_h / 768.0f;
    if (scale < 0.5f)
        scale = 0.5f;

    static float last_scale = 0.0f;
    if (scale != last_scale)
    {
        ImGui::StyleColorsDark();
        ImGuiStyle &style = ImGui::GetStyle();

        style.WindowRounding = 8.0f;
        style.ChildRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.WindowBorderSize = 0.0f;

        ImVec4 *colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.10f, 0.15f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.14f, 0.20f, 0.85f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.12f, 0.14f, 0.20f, 1.00f);

        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.25f, 0.33f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.27f, 0.30f, 0.38f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.25f, 0.33f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.27f, 0.30f, 0.38f, 1.00f);

        colors[ImGuiCol_Button] = ImVec4(0.41f, 0.32f, 0.81f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.48f, 0.39f, 0.88f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.26f, 0.72f, 1.00f);

        colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.20f, 0.27f, 1.00f);
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);

        style.ScaleAllSizes(scale);
        ImGui::GetIO().FontGlobalScale = scale;
        last_scale = scale;
    }

    ImGui::NewFrame();

    // 3-SECOND BACKGROUND ANIMATION CYCLE
    if (!bg_frames_.empty())
    {
        double time = glfwGetTime();
        int frame_index = (int)(time / 3.0) % bg_frames_.size();

        ImGui::GetBackgroundDrawList()->AddImage((void *)(intptr_t)bg_frames_[frame_index], ImVec2(0, 0), ImVec2((float)win_w, (float)win_h));
        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), ImVec2((float)win_w, (float)win_h), IM_COL32(15, 17, 25, 175));
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::Begin("Main App", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleColor();

    if (show_login_)
    {
        renderLogin();
    }
    else
    {
        renderRoleControls();
        ImGui::SameLine();

        ImGui::BeginChild("MainContent", ImVec2(0, -40 * scale), false);
        {
            if (active_screen_ == 0)
                renderDashboard();
            else if (active_screen_ == 1)
                renderStations();
            else if (active_screen_ == 2)
                renderReservations();
            else if (active_screen_ == 3)
                renderActiveSessions();
        }
        ImGui::EndChild();
    }

    renderStatusMessages();
    ImGui::End();

    ImGui::Render();
    glClearColor(0.09f, 0.10f, 0.15f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiApp::renderLogin()
{
    float scale = ImGui::GetIO().FontGlobalScale;

    float box_width = 380.0f * scale;
    float box_height = 240.0f * scale;
    float center_x = (ImGui::GetMainViewport()->WorkSize.x - box_width) * 0.5f;
    float center_y = (ImGui::GetMainViewport()->WorkSize.y - box_height) * 0.5f;

    ImGui::SetCursorPos(ImVec2(center_x, center_y));
    ImGui::BeginChild("LoginBox", ImVec2(box_width, box_height), true);

    ImGui::Text("GAME&GO - SYSTEM LOGIN");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Select Demo User from Database:");

    // Exactly matches Member 1's seed data in PostgreSQL!
    const char *user_names[] = {
        "dev-Alice Admin (Admin - ID 1)",
        "dev-Bob Staff (Staff - Branch 1 - ID 4)",
        "dev-Charlie Customer (Customer - ID 10)",
        "dev-Eve Staff (Staff - Branch 2 - ID 6)",
        "dev-Adam Customer (Customer - ID 11)"};
    const int user_ids[] = {1, 4, 10, 6, 11};
    static int selected_user_idx = 0;

    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##user_select", &selected_user_idx, user_names, 5);

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Login", ImVec2(-1, 45 * scale)))
    {
        try
        {
            if (session_manager_.loginByUserId(user_ids[selected_user_idx]))
            {
                show_login_ = false;
                active_screen_ = 0;
                needs_refresh_ = true;
                status_message_ = "Logged in successfully!";
            }
            else
            {
                status_message_ = "ERROR: Login failed. User ID not found in database.";
            }
        }
        catch (const std::exception &e)
        {
            status_message_ = std::string("DB Error: ") + e.what();
        }
    }
    ImGui::EndChild();
}

void ImGuiApp::renderDashboard()
{
    ImGui::Text("DASHBOARD");
    ImGui::Separator();
    const User *current_user = session_manager_.getCurrentUser();
    if (!current_user)
        return;

    ImGui::Text("Branch:");
    ImGui::SameLine();

    if (cached_branches_.empty())
    {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No branches loaded from database.");
    }
    else
    {
        std::vector<const char *> combo_items;
        for (const auto &b : cached_branches_)
            combo_items.push_back(b.branch_name.c_str());

        ImGui::SetNextItemWidth(250);
        if (ImGui::Combo("##branch", &selected_branch_idx_, combo_items.data(), combo_items.size()))
        {
            session_manager_.setSelectedBranch(cached_branches_[selected_branch_idx_].id);
            needs_refresh_ = true;
        }

        if (!session_manager_.getSelectedBranch().has_value() && !cached_branches_.empty())
        {
            session_manager_.setSelectedBranch(cached_branches_[0].id);
            needs_refresh_ = true;
        }
    }

    ImGui::Spacing();

    auto branch_id = session_manager_.getSelectedBranch();
    if (branch_id.has_value())
    {
        int avail = 0, inuse = 0, maint = 0;
        for (const auto &st : cached_stations_)
        {
            if (st.status == "Available")
                avail++;
            else if (st.status == "InUse")
                inuse++;
            else
                maint++;
        }

        if (current_user->canManageStations())
        {
            ImGui::Text("Total Stations: %zu | Available: %d | In Use: %d | Maintenance: %d",
                        cached_stations_.size(), avail, inuse, maint);
        }
        else
        {
            ImGui::Text("Available Stations: %d | Currently In Use: %d", avail, inuse);
        }
    }

    if (current_user->canViewReports())
    {
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "ADMIN REPORTS:");
        ImGui::Text("Active Sessions Today: %zu", cached_sessions_.size());
    }
}
// =========================================================================
// STATIONS SCREEN (WITH CLEAN CUSTOMER DROPDOWN)
// =========================================================================
void ImGuiApp::renderStations()
{
    float scale = ImGui::GetIO().FontGlobalScale;
    ImGui::Text("STATIONS");
    ImGui::Separator();

    const User *current_user = session_manager_.getCurrentUser();
    auto branch_id = session_manager_.getSelectedBranch();

    if (!branch_id.has_value())
    {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Please select a branch on the Dashboard first.");
        return;
    }

    if (cached_stations_.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No stations found in the database for this branch.");
        return;
    }

    // List of real database customers from Member 1's seed script
    const char *customer_names[] = {
        "Charlie (ID 10)",
        "Adam (ID 11)",
        "Sandy (ID 12)",
        "Oscar (ID 13)",
        "Diana (ID 14)"};
    const int customer_ids[] = {10, 11, 12, 13, 14};
    static int selected_cust_idx = 0;

    if (ImGui::BeginTable("StationsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
    {
        ImGui::TableSetupColumn("Station ID");
        ImGui::TableSetupColumn("Type & Rate");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (const auto &st : cached_stations_)
        {
            if (!current_user->canManageStations() && st.status == "Maintenance")
                continue;

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", st.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s - %s/hr", st.type.c_str(), formatMoney(st.hourly_rate_cents).c_str());
            ImGui::TableSetColumnIndex(2);

            if (st.status == "Available")
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", st.status.c_str());
            else if (st.status == "InUse")
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "%s", st.status.c_str());
            else
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", st.status.c_str());

            ImGui::TableSetColumnIndex(3);

            if (current_user->canManageStations())
            {
                if (st.status == "Available")
                {

                    // CLEAN DROPDOWN MENU FOR CUSTOMERS:
                    ImGui::SetNextItemWidth(140 * scale);
                    ImGui::Combo(("##cust" + std::to_string(st.id)).c_str(), &selected_cust_idx, customer_names, 5);
                    ImGui::SameLine();

                    if (ImGui::Button(("Start Session##" + std::to_string(st.id)).c_str()))
                    {
                        try
                        {
                            int target_customer_id = customer_ids[selected_cust_idx];
                            if (session_manager_.startSession(target_customer_id, st.id))
                            {
                                status_message_ = "Session started! Station marked InUse in DB.";
                                needs_refresh_ = true;
                            }
                            else
                            {
                                status_message_ = "Error: Station is not Available.";
                            }
                        }
                        catch (const std::exception &e)
                        {
                            status_message_ = std::string("DB Error: ") + e.what();
                        }
                    }
                }
                else if (st.status == "InUse")
                {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Currently Playing");
                }
            }
            else
            {
                ImGui::TextDisabled("Read Only");
            }
        }
        ImGui::EndTable();
    }
}
void ImGuiApp::renderReservations()
{
    ImGui::Text("RESERVATIONS");
    ImGui::Separator();

    const User *current_user = session_manager_.getCurrentUser();
    auto branch_id = session_manager_.getSelectedBranch();

    if (!branch_id.has_value())
    {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Please select a branch on the Dashboard first.");
        return;
    }

    ImGui::Text("Create New Reservation:");
    const char *types[] = {"PC", "PS4", "PS5"};
    ImGui::Combo("Station Type", &input_res_type_idx_, types, 3);
    ImGui::InputText("Time (UTC)", input_res_time_, 64);
    ImGui::InputText("Deposit Amount (EGP)", input_res_deposit_, 32);

    if (ImGui::Button("Create Reservation"))
    {
        try
        {
            MoneyCents deposit = parseMoneyCents(input_res_deposit_);
            if (session_manager_.createReservation(current_user->getId(), branch_id.value(),
                                                   types[input_res_type_idx_], input_res_time_, deposit))
            {
                status_message_ = "Reservation created and saved to PostgreSQL!";
                needs_refresh_ = true;
            }
            else
            {
                status_message_ = "Failed to create reservation.";
            }
        }
        catch (const std::exception &e)
        {
            status_message_ = std::string("DB Error on Create: ") + e.what();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (cached_reservations_.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No reservations found for this branch.");
        return;
    }

    if (ImGui::BeginTable("ResTable", 6, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Deposit");
        ImGui::TableSetupColumn("Status");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (const auto &res : cached_reservations_)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", res.id);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", res.station_type.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", res.reserved_time.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", formatMoney(res.deposit_cents).c_str());
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", res.status.c_str());

            ImGui::TableSetColumnIndex(5);
            if (current_user->canManageReservations())
            {
                if (res.status == "Pending")
                {
                    if (ImGui::Button(("Confirm##" + std::to_string(res.id)).c_str()))
                    {
                        try
                        {
                            if (session_manager_.confirmReservation(res.id))
                            {
                                status_message_ = "Reservation Confirmed!";
                                needs_refresh_ = true;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            status_message_ = std::string("DB Error: ") + e.what();
                        }
                    }
                    ImGui::SameLine();
                }
                if (res.status != "Canceled")
                {
                    if (ImGui::Button(("Cancel##" + std::to_string(res.id)).c_str()))
                    {
                        try
                        {
                            if (session_manager_.cancelReservation(res.id))
                            {
                                status_message_ = "Reservation Canceled!";
                                needs_refresh_ = true;
                            }
                        }
                        catch (const std::exception &e)
                        {
                            status_message_ = std::string("DB Error: ") + e.what();
                        }
                    }
                }
            }
            else
            {
                ImGui::TextDisabled("None");
            }
        }
        ImGui::EndTable();
    }
}

void ImGuiApp::renderActiveSessions()
{
    float scale = ImGui::GetIO().FontGlobalScale;
    ImGui::Text("ACTIVE SESSIONS");
    ImGui::Separator();

    if (cached_sessions_.empty())
    {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1), "No active sessions. Lounge is empty.");
        return;
    }

    if (ImGui::BeginTable("ActiveTable", 6, ImGuiTableFlags_Borders))
    {
        ImGui::TableSetupColumn("Session ID");
        ImGui::TableSetupColumn("Customer");
        ImGui::TableSetupColumn("Station");
        ImGui::TableSetupColumn("Duration");
        ImGui::TableSetupColumn("Current Cost");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (const auto &ses : cached_sessions_)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", ses.session_id);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", ses.customer_name.c_str());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("ID:%d (%s)", ses.station_id, ses.station_type.c_str());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d mins", ses.elapsed_minutes);

            MoneyCents current_cost = session_manager_.previewCost(ses.hourly_rate_cents, ses.elapsed_minutes);
            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "%s", formatMoney(current_cost).c_str());

            ImGui::TableSetColumnIndex(5);
            if (ImGui::Button(("Finish Session##" + std::to_string(ses.session_id)).c_str()))
            {
                try
                {
                    if (session_manager_.finishSession(ses.session_id))
                    {
                        status_message_ = "Session finished! Transaction committed to PostgreSQL.";
                        needs_refresh_ = true;
                    }
                    else
                    {
                        status_message_ = "Failed to finish session.";
                    }
                }
                catch (const std::exception &e)
                {
                    status_message_ = std::string("DB Error: ") + e.what();
                }
            }
        }
        ImGui::EndTable();
    }
}

void ImGuiApp::renderRoleControls()
{
    float scale = ImGui::GetIO().FontGlobalScale;

    const User *current_user = session_manager_.getCurrentUser();
    if (!current_user)
        return;

    ImGui::BeginChild("Sidebar", ImVec2(200 * scale, 0), true);

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", current_user->getName().c_str());
    ImGui::TextDisabled("%s", toString(current_user->getRole()).c_str());
    ImGui::Separator();

    if (ImGui::Button("Dashboard", ImVec2(-1, 40 * scale)))
    {
        active_screen_ = 0;
        needs_refresh_ = true;
    }
    if (ImGui::Button("Stations", ImVec2(-1, 40 * scale)))
    {
        active_screen_ = 1;
        needs_refresh_ = true;
    }
    if (ImGui::Button("Reservations", ImVec2(-1, 40 * scale)))
    {
        active_screen_ = 2;
        needs_refresh_ = true;
    }

    if (current_user->canManageStations())
    {
        if (ImGui::Button("Active Sessions", ImVec2(-1, 40 * scale)))
        {
            active_screen_ = 3;
            needs_refresh_ = true;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("Logout", ImVec2(-1, 40 * scale)))
    {
        show_login_ = true;
        status_message_ = "";
    }

    ImGui::EndChild();
}

void ImGuiApp::renderStatusMessages()
{
    if (status_message_.empty())
        return;

    ImGui::TextColored(ImVec4(1, 1, 0, 1), "SYSTEM: %s", status_message_.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Dismiss [X]"))
        status_message_ = "";
}