#include "help_overlay.hpp"

HelpOverlay::~HelpOverlay() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void HelpOverlay::init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImGui::StyleColorsDark();
}

void HelpOverlay::render() {
    if (!show_help) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 20.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.07f, 0.14f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.8f, 0.5f));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |ImGuiWindowFlags_NoTitleBar;
    if (ImGui::Begin("Game Instructions", &show_help, window_flags)) {

        ImGui::PushFont(pixelFont);
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f), "GAME INSTRUCTIONS");
        ImGui::PopFont();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.843f, 0.0f, 1.0f));
        ImGui::TextWrapped("Kill all enemies after the remote location and progress to the next level");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::CollapsingHeader("Booster Items Guide", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("Booster##child", ImVec2(0, 180), true);
            ImGui::Columns(2);

            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Health Potion");
            ImGui::NextColumn();
            ImGui::TextWrapped("Regenerates player health instantly.");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "Energy Core");
            ImGui::NextColumn();
            ImGui::TextWrapped("Permanently increases the player's maximum health.");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "Robot Part");
            ImGui::NextColumn();
            ImGui::TextWrapped("Upgrades your companion robot, improving its abilities in combat.");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Speed Booster");
            ImGui::NextColumn();
            ImGui::TextWrapped("Grants a quick dash ability to escape enemies or reposition during combat.");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.6f, 1.0f), "Armor Plate");
            ImGui::NextColumn();
            ImGui::TextWrapped("Adds an armor layer that absorbs damage. The player takes no health damage until the armor breaks.");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.0f, 1.0f), "Key");
            ImGui::NextColumn();
            ImGui::TextWrapped("Unlocks doors to progress through the level or access hidden areas.");
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::EndChild();
        }


        if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("Movement##child", ImVec2(0, 100), true);
            ImGui::Columns(2);

            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "W A S D");
            ImGui::NextColumn();
            ImGui::Text("Move Character");
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Combat", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("Combat##child", ImVec2(0, 100), true);
            ImGui::Columns(2);

            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Left Click");
            ImGui::NextColumn();
            ImGui::Text("Attack");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Right Click");
            ImGui::NextColumn();
            ImGui::Text("Block");

            ImGui::Columns(1);
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Inventory", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("Inventory##child", ImVec2(0, 100), true);
            ImGui::Columns(2);

            ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "E");
            ImGui::NextColumn();
            ImGui::Text("Interact");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "I");
            ImGui::NextColumn();
            ImGui::Text("Open Inventory");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "Q");
            ImGui::NextColumn();
            ImGui::Text("Use Item");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.4f, 0.4f, 1.0f, 1.0f), "1-3");
            ImGui::NextColumn();
            ImGui::Text("Quick Slots");

            ImGui::Columns(1);
            ImGui::EndChild();
        }

        if (ImGui::CollapsingHeader("Other Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginChild("Other##child", ImVec2(0, 80), true);
            ImGui::Columns(2);

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "R");
            ImGui::NextColumn();
            ImGui::Text("Restart Game");
            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "ESC");
            ImGui::NextColumn();
            ImGui::Text("Exit Game");

            ImGui::Columns(1);
            ImGui::EndChild();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
     
        float window_width = ImGui::GetWindowSize().x;
        float button_width = 120.0f;
        ImGui::SetCursorPosX((window_width - button_width) * 0.5f);
        if (ImGui::Button("Close [H]", ImVec2(button_width, 30))) {
            show_help = false;
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}