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
#include <algorithm>

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
        // ADDED: populate the Management account table with every role, in ID order.
        cached_users_ = session_manager_.loadUsersByRole("Admin");
        auto staff_users = session_manager_.loadUsersByRole("Staff");
        cached_users_.insert(cached_users_.end(), staff_users.begin(), staff_users.end());
        cached_customers_ = session_manager_.loadUsersByRole("Customer");
        cached_users_.insert(cached_users_.end(), cached_customers_.begin(), cached_customers_.end());
        std::sort(cached_users_.begin(), cached_users_.end(),
                  [](const UserRecord& a, const UserRecord& b) { return a.id < b.id; });
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
            else if (active_screen_ == 4)
                renderManagement();
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
    // ADDED: compact username/password login. Labels sit above fields to avoid
    // Dear ImGui's default side-label clipping with full-width inputs.
    const float scale = ImGui::GetIO().FontGlobalScale;
    const float box_width = 390.0f * scale;
    const float box_height = 300.0f * scale;
    
    const ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;
    const ImVec2 child_pos((work_size.x - box_width) * 0.5f,
                           (work_size.y - box_height) * 0.5f);

    ImGui::SetCursorPos(child_pos);
    ImGui::BeginChild("LoginBox", ImVec2(box_width, box_height), true);
    ImGui::Text("GAME&GO - SYSTEM LOGIN");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextUnformatted("Username");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##login_username", login_username_, sizeof(login_username_));
    ImGui::Spacing();

    ImGui::TextUnformatted("Password");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##login_password", login_password_, sizeof(login_password_), ImGuiInputTextFlags_Password);
    ImGui::Spacing();

    if (ImGui::Button("Login", ImVec2(-1, 42.0f * scale)))
    {
        try
        {
            if (session_manager_.loginByCredentials(login_username_, login_password_))
            {
                show_login_ = false;
                active_screen_ = 0;
                needs_refresh_ = true;
                status_message_ = "Logged in successfully.";
                login_password_[0] = '\0';
            }
            else
            {
                status_message_ = "Login failed. Check username and password.";
            }
        }
        catch (const std::exception& e)
        {
            status_message_ = std::string("Login error: ") + e.what();
        }
    }

    ImGui::Spacing();
    ImGui::TextDisabled("Test login: alice.admin / Welcome123!");
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

    // ADDED: load customer choices from the active database backend instead
    // of hardcoding seed IDs, so newly created customers appear immediately.
    static int selected_cust_idx = 0;
    std::vector<std::string> customer_labels;
    std::vector<const char*> customer_names;
    std::vector<int> customer_ids;
    customer_labels.reserve(cached_customers_.size());
    customer_names.reserve(cached_customers_.size());
    customer_ids.reserve(cached_customers_.size());
    for (const auto& customer : cached_customers_)
    {
        customer_labels.push_back(customer.name + " (ID " + std::to_string(customer.id) + ")");
        customer_ids.push_back(customer.id);
    }
    for (const auto& label : customer_labels) customer_names.push_back(label.c_str());
    if (selected_cust_idx >= static_cast<int>(customer_names.size())) selected_cust_idx = 0;

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

                    if (customer_names.empty())
                    {
                        ImGui::TextDisabled("Add a customer first");
                        continue;
                    }
                    ImGui::SetNextItemWidth(140 * scale);
                    ImGui::Combo(("##cust" + std::to_string(st.id)).c_str(), &selected_cust_idx,
                                 customer_names.data(), static_cast<int>(customer_names.size()));
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
                status_message_ = "Reservation created and saved to the database.";
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

// ADDED: tab-based management UI styled to match the online edition's layout.
// All operations still go through SessionManager, and role checks remain in the core.
// The ## suffixes keep internal ImGui IDs unique without changing visible labels.
void ImGuiApp::renderManagement()
{
    const User* current_user = session_manager_.getCurrentUser();
    if (!current_user)
        return;

    const bool is_admin = current_user->getRole() == UserRole::Admin;
    ImGui::Text("MANAGEMENT");
    ImGui::Separator();
    ImGui::TextWrapped(is_admin
        ? "Create staff/customer accounts, branches, and stations."
        : "Staff can register customers. Branch and station management is admin-only.");
    ImGui::Spacing();

    // Match the online edition: separate tabs across the top, with one form shown at a time.
    if (ImGui::BeginTabBar("ManagementTabs"))
    {
        if (ImGui::BeginTabItem(is_admin ? "Add Account" : "Add Customer"))
        {
            ImGui::Text("New account details");
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputText("Full name", new_user_name_, sizeof(new_user_name_));
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputText("Username", new_user_username_, sizeof(new_user_username_));
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputText("Password (min. 8 characters)", new_user_password_, sizeof(new_user_password_), ImGuiInputTextFlags_Password);
            ImGui::SetNextItemWidth(360.0f);
            ImGui::InputText("Phone (optional)", new_user_phone_, sizeof(new_user_phone_));

            if (is_admin)
            {
                const char* roles[] = {"Customer", "Staff"};
                if (new_user_role_idx_ < 0 || new_user_role_idx_ > 1)
                    new_user_role_idx_ = 0;
                ImGui::SetNextItemWidth(200.0f);
                ImGui::Combo("Account role", &new_user_role_idx_, roles, 2);
            }
            else
            {
                new_user_role_idx_ = 0;
                ImGui::Text("Account role: Customer");
            }

            std::vector<std::string> branch_labels;
            std::vector<const char*> branch_names;
            branch_labels.reserve(cached_branches_.size());
            branch_names.reserve(cached_branches_.size());
            for (const auto& branch : cached_branches_)
                branch_labels.push_back(branch.branch_name + " (ID " + std::to_string(branch.id) + ")");
            for (const auto& label : branch_labels)
                branch_names.push_back(label.c_str());

            if (branch_names.empty())
            {
                ImGui::TextDisabled("No branches exist yet. An admin can add one in the Add Branch tab.");
                new_user_branch_idx_ = 0;
            }
            else
            {
                if (new_user_branch_idx_ < 0 || new_user_branch_idx_ >= static_cast<int>(branch_names.size()))
                    new_user_branch_idx_ = 0;
                ImGui::SetNextItemWidth(360.0f);
                ImGui::Combo("Assigned branch (optional for customers)", &new_user_branch_idx_,
                             branch_names.data(), static_cast<int>(branch_names.size()));
            }

            if (ImGui::Button("Create Account##create_account_submit", ImVec2(200.0f, 38.0f)))
            {
                try
                {
                    UserRecord user{};
                    user.name = new_user_name_;
                    user.username = new_user_username_;
                    user.role = (is_admin && new_user_role_idx_ == 1) ? "Staff" : "Customer";
                    user.phone = new_user_phone_;
                    if (!cached_branches_.empty() && new_user_branch_idx_ >= 0 &&
                        new_user_branch_idx_ < static_cast<int>(cached_branches_.size()))
                        user.branch_id = cached_branches_[new_user_branch_idx_].id;

                    if (session_manager_.createUserAccount(user, new_user_password_))
                    {
                        status_message_ = "Account created successfully.";
                        new_user_name_[0] = new_user_username_[0] = new_user_password_[0] = new_user_phone_[0] = '\0';
                        needs_refresh_ = true;
                    }
                    else
                    {
                        status_message_ = "Could not create account. Check fields, permissions, username uniqueness, branch, and password length.";
                    }
                }
                catch (const std::exception& e)
                {
                    status_message_ = std::string("Account creation failed: ") + e.what();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Existing accounts");
            if (ImGui::BeginTable("AccountsTable", 5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX,
                                  ImVec2(0.0f, 190.0f)))
            {
                ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Username", ImGuiTableColumnFlags_WidthFixed, 140.0f);
                ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 95.0f);
                ImGui::TableSetupColumn("Phone", ImGuiTableColumnFlags_WidthFixed, 135.0f);
                ImGui::TableHeadersRow();
                for (const auto& user : cached_users_)
                {
                    if (!is_admin && user.role != "Customer")
                        continue;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::Text("%d", user.id);
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(user.name.c_str());
                    ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(user.username.c_str());
                    ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(user.role.c_str());
                    ImGui::TableSetColumnIndex(4); ImGui::TextUnformatted(user.phone.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        if (is_admin && ImGui::BeginTabItem("Add Branch"))
        {
            ImGui::Text("New branch details");
            ImGui::SetNextItemWidth(360.0f); ImGui::InputText("Branch name", new_branch_name_, sizeof(new_branch_name_));
            ImGui::SetNextItemWidth(360.0f); ImGui::InputText("Street address", new_branch_address_, sizeof(new_branch_address_));
            ImGui::SetNextItemWidth(360.0f); ImGui::InputText("District", new_branch_district_, sizeof(new_branch_district_));
            ImGui::SetNextItemWidth(360.0f); ImGui::InputText("City", new_branch_city_, sizeof(new_branch_city_));
            ImGui::SetNextItemWidth(360.0f); ImGui::InputText("Country", new_branch_country_, sizeof(new_branch_country_));

            if (ImGui::Button("Create Branch##create_branch_submit", ImVec2(200.0f, 38.0f)))
            {
                try
                {
                    BranchRecord branch{};
                    branch.branch_name = new_branch_name_;
                    branch.street_address = new_branch_address_;
                    branch.district = new_branch_district_;
                    branch.location_city = new_branch_city_;
                    branch.location_country = new_branch_country_;
                    if (session_manager_.createBranch(branch))
                    {
                        status_message_ = "Branch created successfully.";
                        new_branch_name_[0] = new_branch_address_[0] = new_branch_district_[0] = new_branch_city_[0] = '\0';
                        needs_refresh_ = true;
                    }
                    else
                        status_message_ = "Could not create branch. Complete all branch fields.";
                }
                catch (const std::exception& e)
                {
                    status_message_ = std::string("Branch creation failed: ") + e.what();
                }
            }
            ImGui::EndTabItem();
        }

        if (is_admin && ImGui::BeginTabItem("Add Station"))
        {
            if (cached_branches_.empty())
            {
                ImGui::Text("Create a branch before adding a station.");
            }
            else
            {
                std::vector<std::string> branch_labels;
                std::vector<const char*> branch_names;
                branch_labels.reserve(cached_branches_.size());
                branch_names.reserve(cached_branches_.size());
                for (const auto& branch : cached_branches_)
                    branch_labels.push_back(branch.branch_name + " (ID " + std::to_string(branch.id) + ")");
                for (const auto& label : branch_labels)
                    branch_names.push_back(label.c_str());

                if (new_station_branch_idx_ < 0 || new_station_branch_idx_ >= static_cast<int>(branch_names.size()))
                    new_station_branch_idx_ = 0;
                ImGui::Text("New station details");
                ImGui::SetNextItemWidth(360.0f);
                ImGui::Combo("Branch##new_station_branch", &new_station_branch_idx_,
                             branch_names.data(), static_cast<int>(branch_names.size()));
                const char* types[] = {"PC", "PS4", "PS5"};
                ImGui::SetNextItemWidth(200.0f);
                ImGui::Combo("Station type", &new_station_type_idx_, types, 3);
                ImGui::SetNextItemWidth(200.0f);
                ImGui::InputText("Hourly rate (EGP)", new_station_rate_, sizeof(new_station_rate_));
                ImGui::TextDisabled("New stations start with status Available.");

                if (ImGui::Button("Create Station##create_station_submit", ImVec2(200.0f, 38.0f)))
                {
                    try
                    {
                        StationRecord station{};
                        station.branch_id = cached_branches_[new_station_branch_idx_].id;
                        station.type = types[new_station_type_idx_];
                        station.hourly_rate_cents = parseMoneyCents(new_station_rate_);
                        station.status = "Available";
                        if (session_manager_.createStation(station))
                        {
                            status_message_ = "Station created successfully.";
                            needs_refresh_ = true;
                        }
                        else
                            status_message_ = "Could not create station. Check branch, type, rate, and permissions.";
                    }
                    catch (const std::exception& e)
                    {
                        status_message_ = std::string("Station creation failed: ") + e.what();
                    }
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
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
                        status_message_ = "Session finished and saved to the database.";
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

    // ADDED: staff and admins can register customers; only admins see branch/station creation.
    if (current_user->getRole() == UserRole::Admin || current_user->getRole() == UserRole::Staff)
    {
        if (ImGui::Button("Management", ImVec2(-1, 40 * scale)))
        {
            active_screen_ = 4;
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