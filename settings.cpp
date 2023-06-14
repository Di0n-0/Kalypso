//g++ -g -o settings settings.cpp -L/home/di0n/vcpkg/installed/x64-linux/lib -lglfw3 -lGLEW -lGL -limgui -lImGuiFileDialog -lsoil -lcjson -lm -lX11
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstddef>
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <ImGuiFileDialog.h>
#include <SOIL/SOIL.h>
#include <cjson/cJSON.h>

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

void GenerateJSON(){
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "filePath", filePathName.c_str());
    cJSON_AddNumberToObject(root, "identicalFunctionGap", identicalFunctionGap);
    cJSON_AddNumberToObject(root, "sensivity", sensivity);

    char *JSONstring = cJSON_Print(root);
    
    FILE *file = fopen("config/settings.json", "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to create JSON file.\n");
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

    printf("JSON file created successfully.\n");
}

GLuint LoadTexture(const char* imagePath) {
    int width, height, channels;
    unsigned char* image = SOIL_load_image(imagePath, &width, &height, &channels, SOIL_LOAD_RGBA);
    if (!image)
    {
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

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    SOIL_free_image_data(image);

    return textureID;
}

// Function to initialize ImGui
void InitializeImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Function to render ImGui content
void RenderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width_custom, height_custom));
    ImGui::Begin("User Dependencies", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (ImGui::Button("Upload Image", ImVec2(120, 25))) {
        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".png,.jpeg,.tga,.bmp,.psd,.tga,.hdr,.pic,.pnm,.dds,.pvr,.pkm", ".");
        openedDialog = true;
    }
    if(openedDialog) {
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse, ImVec2(400, 400), ImVec2(WINDOW_WIDTH, (float)WINDOW_HEIGHT * 4 / 5))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                ImGui::SetWindowCollapsed(true);
                filePathName = ImGuiFileDialog::Instance()->GetFilePathName(); 
                imageTextureID = LoadTexture(filePathName.c_str());
                if(imageTextureID != 0) {
                    GenerateJSON(); 
                    uploadImage = true;
                }
            }
            ImGuiFileDialog::Instance()->Close();
        }
        if(uploadImage){
            if (ImGui::Button("Remove Image", ImVec2(120, 25))) {
                filePathName = "";
                GenerateJSON(); 
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
    if(uploadImage) ImGui::Image((void*)(intptr_t)imageTextureID, ImVec2(400, 300));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Separator();
    ImGui::Spacing();
    if(ImGui::SliderInt("Distance between consecutive identical functions", &identicalFunctionGap, 1.0f, 250.0f)) GenerateJSON();
    if(ImGui::SliderInt("Sensivity of preprocessing", &sensivity, 1.0f, 255.0f)) GenerateJSON();
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) return -1;

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Settings", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) return -1;

    // Set up ImGui
    InitializeImGui(window);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetWindowSize(window, &width_custom, &height_custom);

        // Render ImGui content
        RenderImGui();

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}

