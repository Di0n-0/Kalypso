// g++ -g -o settings settings.cpp -I/usr/include/python3.10 -L/home/di0n/vcpkg/installed/x64-linux/lib -lglfw3 -lGLEW -lGL -limgui -lImGuiFileDialog -lsoil -lcjson -lpython3.10 -lm -lX11 -Wall
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <ImGuiFileDialog.h>
#include <SOIL/SOIL.h>
#include <cjson/cJSON.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// GLFW3 window size
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 600;

int width_custom = WINDOW_WIDTH;
int height_custom = WINDOW_HEIGHT;

bool openedDialog, uploadImage = false;
std::string filePathName = "";
GLint imageTextureID = 0;
GLint identicalFunctionGap = 50;
GLint sensivity = 80;

std::string saveState = "Not Saved";
ImVec4 saveColor(1, 0, 0, 1);

int Preprocess() {
    std::string command = "python3 preprocess.py";
    int exitCode = system(command.c_str());
    if (exitCode != 0) {
        throw std::runtime_error("Python script execution failed.");
    }

    saveState = "Saved";
    saveColor = ImVec4(0, 1, 0, 1);
    return 0;
}

void GenerateJSON() {
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "filePath", filePathName.c_str());
    cJSON_AddNumberToObject(root, "identicalFunctionGap", identicalFunctionGap);
    cJSON_AddNumberToObject(root, "sensivity", 255 - sensivity);

    char *JSONstring = cJSON_Print(root);

    FILE *file = fopen("config/settings.json", "w");
    if (file == NULL) {
        fprintf(stderr, "[ERORR] Failed to create JSON file.\n");
        cJSON_Delete(root);
        free(JSONstring);
    }

    // Write JSON string to the file
    fwrite(JSONstring, 1, strlen(JSONstring), file);

    // Close the file
    fclose(file);

    // Clean up memory
    cJSON_Delete(root);
    free(JSONstring);

    printf("[INFO] JSON file created successfully.\n");
    if (filePathName == "") {
        saveState = "Saved";
        saveColor = ImVec4(0, 1, 0, 1);
    }
}

GLuint LoadTexture(const char *imagePath) {
    int width, height, channels;
    unsigned char *image = SOIL_load_image(imagePath, &width, &height, &channels, SOIL_LOAD_RGBA);
    if (!image) {
        fprintf(stderr, "Failed to load image: %s\n", imagePath);
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);

    return textureID;
}

int ReadJSON(){
    std::ifstream jsonFile("config/settings.json");
    std::stringstream buffer;
    buffer << jsonFile.rdbuf();
    std::string json_data = buffer.str();

    cJSON* root = cJSON_Parse(json_data.c_str());

    cJSON* a_value = cJSON_GetObjectItemCaseSensitive(root, "filePath");
    cJSON* b_value = cJSON_GetObjectItemCaseSensitive(root, "identicalFunctionGap");
    cJSON* c_value = cJSON_GetObjectItemCaseSensitive(root, "sensivity");

    if (a_value != nullptr && cJSON_IsString(a_value) &&
        b_value != nullptr && cJSON_IsNumber(b_value) &&
        c_value != nullptr && cJSON_IsNumber(c_value)) {
        std::string a = a_value->valuestring;
        int b = b_value->valueint;
        int c = c_value->valueint;

        filePathName = a;
        identicalFunctionGap = b;
        sensivity = 255 - c;

        if(filePathName != ""){
            imageTextureID = LoadTexture(filePathName.c_str());
            if(imageTextureID != 0) uploadImage = true;
        }
    }

    cJSON_Delete(root);
    
    return 0;
}

void InitializeImGui(GLFWwindow *window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void RenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width_custom, height_custom));
    ImGui::Begin(
        "User Dependencies", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (ImGui::Button("Upload Image", ImVec2(120, 25))) {
        saveState = "Not Saved";
        saveColor = ImVec4(1, 0, 0, 1);
        ImGuiFileDialog::Instance()->OpenDialog(
            "ChooseFileDlgKey", "Choose File",
            ".png,.jpeg,.tga,.bmp,.psd,.tga,.hdr,.pic,.pnm,.dds,.pvr,.pkm",
            ".");
        openedDialog = true;
    }
    ImGui::SameLine();
    ImGui::SetCursorPosX(WINDOW_WIDTH - 128);
    if (ImGui::Button("Save", ImVec2(120, 25))) {
        saveState = "Saving ...";
        saveColor = ImVec4(1, 0, 0, 1);
        GenerateJSON();
        if (filePathName != "") Preprocess();
    }
    if (openedDialog || uploadImage) {
        if (ImGuiFileDialog::Instance()->Display(
                "ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse,
                ImVec2(400, 400),
                ImVec2(WINDOW_WIDTH, (float)WINDOW_HEIGHT * 4 / 5))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                ImGui::SetWindowCollapsed(true);
                filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                imageTextureID = LoadTexture(filePathName.c_str());
                if (imageTextureID != 0) {
                    uploadImage = true;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if (uploadImage) {
            if (ImGui::Button("Remove Image", ImVec2(120, 25))) {
                saveState = "Not Saved";
                saveColor = ImVec4(1, 0, 0, 1);
                filePathName = "";
                uploadImage = false;
                openedDialog = false;
            }
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Text("%s", filePathName.c_str());
    if (uploadImage) ImGui::Image((void*)(intptr_t)imageTextureID, ImVec2(400, 300));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::SliderInt("Distance between consecutive identical functions",
                         &identicalFunctionGap, 1.0f, 250.0f)) {
        saveState = "Not Saved";
        saveColor = ImVec4(1, 0, 0, 1);
    }
    if (uploadImage) {
        if (ImGui::SliderInt("Sensivity of preprocessing", &sensivity, 0.0f,
                             255.0f)) {
            saveState = "Not Saved";
            saveColor = ImVec4(1, 0, 0, 1);
        }
    }
    ImGui::TextColored(saveColor, "%s", saveState.c_str());
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Settings", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) return -1;
    InitializeImGui(window);

    ReadJSON();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetWindowSize(window, &width_custom, &height_custom);

        RenderImGui();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
