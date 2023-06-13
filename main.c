//g++ -o main main.cpp -L/home/di0n/vcpkg/installed/x64-linux/lib -lraylib -lrlImGui -limgui -lm -lX11
#include <raylib.h>
#include <raymath.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Vertice {
    Vector2 *verticeData;
    int verticeCount;
    struct Vertice *next;
    Vector2 *intersectionPoints;
    int intersectionPointCount;
} Vertice;

typedef struct IdenticalFunction {
    Vector2 start, end;
    Color color;
} IdenticalFunction;

typedef struct IFArray {
    IdenticalFunction *identicalFunctions;
    int identicalFunctionCount;
} IFArray;

typedef struct TextureBound {
    Texture2D texture;
    Rectangle rectangle;
} TextureBound;

IFArray *generateIdenticalFunctions(Vector2 worldTopLeft, Vector2 worldBottomRight) {
    const int IDENTICAL_FUNCTION_GAP = 50;
    float identicalFunctionStartX = floorf(worldTopLeft.x / IDENTICAL_FUNCTION_GAP) * IDENTICAL_FUNCTION_GAP;
    float identicalFunctionStartY = floorf(worldTopLeft.y / IDENTICAL_FUNCTION_GAP) * IDENTICAL_FUNCTION_GAP;
    int identicalFunctionCountX = (worldBottomRight.x - worldTopLeft.x) / IDENTICAL_FUNCTION_GAP;
    int identicalFunctionCountY = (worldBottomRight.y - worldTopLeft.y) / IDENTICAL_FUNCTION_GAP;

    IdenticalFunction *identicalFunctions = (IdenticalFunction *)malloc((identicalFunctionCountX + identicalFunctionCountY) * sizeof(IdenticalFunction));
    for (int i = 1; i < identicalFunctionCountX; i++) {
        float x = identicalFunctionStartX + (i * IDENTICAL_FUNCTION_GAP);
        IdenticalFunction identicalFunction = {.start = (Vector2){x, worldTopLeft.y}, .end = (Vector2){x, worldBottomRight.y}, .color = RED};
        identicalFunctions[i - 1] = identicalFunction;
    }
    for (int i = 1; i < identicalFunctionCountY; i++) {
        float y = identicalFunctionStartY + (i * IDENTICAL_FUNCTION_GAP);
        IdenticalFunction identicalFunction = {.start = (Vector2){worldTopLeft.x, y}, .end = (Vector2){worldBottomRight.x, y}, .color = RED};
        identicalFunctions[i - 1 + identicalFunctionCountX] = identicalFunction;
    }

    IFArray *ifArray = (IFArray *)malloc(sizeof(IFArray));
    ifArray->identicalFunctionCount = identicalFunctionCountX + identicalFunctionCountY;
    ifArray->identicalFunctions = identicalFunctions;

    return ifArray;
}

int main(int argc, char **argv) {	
    const int WIDTH = 800;
    const int HEIGHT = 600;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WIDTH, HEIGHT, "Window");
    SetTargetFPS(60);

    Camera2D camera = {.offset = (Vector2){0, 0}, .target = (Vector2){0 - (float)GetScreenWidth() / 2, 0 - (float)GetScreenHeight() / 2}, .rotation = 0.0f, .zoom = 1.0f};

    const int CELL_SIZE = 50;
    const float ZOOM_INCREMENT = 0.125f;

    Vertice *head = (Vertice *)malloc(sizeof(Vertice));
    head->verticeData = NULL;
    head->verticeCount = 0;
    head->next = NULL;
    head->intersectionPoints = NULL;
    head->intersectionPointCount = 0;

    Vertice *current_D = head;
    Vertice *verticeIndex = head;

    Vector2 *verticeData_G = NULL;
    int verticeCount_G = 0;

    IFArray *ifArray_D = NULL;

    Image HED_image = LoadImage("image.png");
    Texture2D HED_texture = LoadTextureFromImage(HED_image);
    UnloadImage(HED_image);
    TextureBound HED_bound = {.texture = HED_texture, .rectangle = (Rectangle){.x = 0 - HED_texture.width / 2.0f, .y = 0 - HED_texture.height / 2.0f, .width = (float)HED_texture.width, .height = (float)HED_texture.height}};

    bool scaleImage = false;
    bool moveImage = false;
    const int SCALE_SIGN_SIZE = 50;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode2D(camera);

        Vector2 cameraPosition = {camera.offset.x, camera.offset.y};
        Vector2 worldTopLeft = GetScreenToWorld2D(cameraPosition, camera);
        Vector2 worldBottomRight = GetScreenToWorld2D((Vector2){cameraPosition.x + GetScreenWidth() / camera.zoom, cameraPosition.y + GetScreenHeight() / camera.zoom}, camera);

        float gridStartX = floorf(worldTopLeft.x / CELL_SIZE) * CELL_SIZE;
        float gridStartY = floorf(worldTopLeft.y / CELL_SIZE) * CELL_SIZE;

        int cellCountVER = (worldBottomRight.x - worldTopLeft.x) / CELL_SIZE;
        int cellCountHOR = (worldBottomRight.y - worldTopLeft.y) / CELL_SIZE;

        DrawTextureV(HED_bound.texture, (Vector2){.x = HED_bound.rectangle.x, .y = HED_bound.rectangle.y}, WHITE);
        if (CheckCollisionPointRec(GetScreenToWorld2D(GetMousePosition(), camera), HED_bound.rectangle)) {
            DrawRectangleLinesEx(HED_bound.rectangle, 10.0f, LIME);
            Vector2 trinaglePointA = {.x = (HED_bound.rectangle.x + HED_bound.rectangle.width) - SCALE_SIGN_SIZE, .y = HED_bound.rectangle.y + HED_bound.rectangle.height};
            Vector2 trinaglePointB = {.x = (HED_bound.rectangle.x + HED_bound.rectangle.width), .y = HED_bound.rectangle.y + HED_bound.rectangle.height};
            Vector2 trinaglePointC = {.x = HED_bound.rectangle.x + HED_bound.rectangle.width, .y = (HED_bound.rectangle.y + HED_bound.rectangle.height) - SCALE_SIGN_SIZE};
            DrawTriangle(trinaglePointA, trinaglePointB, trinaglePointC, LIME);
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_LEFT_CONTROL)) {
                if (CheckCollisionPointTriangle(GetScreenToWorld2D(GetMousePosition(), camera), trinaglePointA, trinaglePointB, trinaglePointC))
                    scaleImage = true;
                else
                    moveImage = true;
            }
        }

        if (scaleImage) {
            HED_bound.rectangle.width = GetScreenToWorld2D(GetMousePosition(), camera).x - HED_bound.rectangle.x;
            HED_bound.rectangle.height = GetScreenToWorld2D(GetMousePosition(), camera).y - HED_bound.rectangle.y;

            if (HED_bound.rectangle.width < SCALE_SIGN_SIZE) HED_bound.rectangle.width = SCALE_SIGN_SIZE;
            if (HED_bound.rectangle.height < SCALE_SIGN_SIZE) HED_bound.rectangle.height = SCALE_SIGN_SIZE;

            if (HED_bound.rectangle.width > GetScreenWidth() - HED_bound.rectangle.x) HED_bound.rectangle.width = GetScreenWidth() - HED_bound.rectangle.x;
            if (HED_bound.rectangle.height > GetScreenHeight() - HED_bound.rectangle.y) HED_bound.rectangle.height = GetScreenHeight() - HED_bound.rectangle.y;

            HED_bound.texture.width = HED_bound.rectangle.width;
            HED_bound.texture.height = HED_bound.rectangle.height;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsKeyReleased(KEY_LEFT_CONTROL)) scaleImage = false;
        }
        if (moveImage && !scaleImage) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            HED_bound.rectangle.x -= delta.x;
            HED_bound.rectangle.y -= delta.y;
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsKeyReleased(KEY_LEFT_CONTROL)) moveImage = false;
        }

        for (int i = 1; i < cellCountVER; i++) {
            float x = gridStartX + (i * CELL_SIZE);
            DrawLine(x, worldTopLeft.y, x, worldBottomRight.y, WHITE);
            const char *numberText = TextFormat("%d", (int)x);
            DrawText(numberText, x + 3, 3, 15, YELLOW);
        }
        for (int i = 1; i < cellCountHOR; i++) {
            float y = gridStartY + (i * CELL_SIZE);
            DrawLine(worldTopLeft.x, y, worldBottomRight.x, y, WHITE);
            const char *numberText = TextFormat("%d", (int)-y);
            DrawText(numberText, 3, y + 3, 15, YELLOW);
        }

        DrawLine(worldTopLeft.x, 0, worldBottomRight.x, 0, ORANGE);
        DrawLine(0, worldTopLeft.y, 0, worldBottomRight.y, ORANGE);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !scaleImage && !moveImage) {
            verticeData_G = (Vector2 *)realloc(verticeData_G, (verticeCount_G + 1) * sizeof(Vector2));
            verticeData_G[verticeCount_G] = GetScreenToWorld2D(GetMousePosition(), camera);
            verticeCount_G++;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && !scaleImage && !moveImage) {
            Vertice *newVertice = (Vertice *)malloc(sizeof(Vertice));
            newVertice->verticeData = (Vector2 *)malloc(verticeCount_G * sizeof(Vector2));
            memcpy(newVertice->verticeData, verticeData_G, verticeCount_G * sizeof(Vector2));
            newVertice->verticeCount = verticeCount_G;
            newVertice->next = verticeIndex->next;
            newVertice->intersectionPoints = NULL;
            newVertice->intersectionPointCount = 0;

            verticeIndex->next = newVertice;
            verticeIndex = newVertice;

            verticeCount_G = 0;

            IFArray *ifArray = generateIdenticalFunctions(worldTopLeft, worldBottomRight);
            int collidingPointCount = 0;
            Vector2 *collidingPoints = (Vector2 *)malloc(sizeof(Vector2));
            for (int i = 0; i < verticeIndex->verticeCount - 1; i++) {
                for (int j = 0; j < ifArray->identicalFunctionCount; j++) {
                    if (CheckCollisionLines(verticeIndex->verticeData[i], verticeIndex->verticeData[i + 1], ifArray->identicalFunctions[j].start, ifArray->identicalFunctions[j].end, &collidingPoints[collidingPointCount])) {
                        collidingPointCount++;
                        collidingPoints = (Vector2 *)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Vector2));
                    }
                }
            }

            verticeIndex->intersectionPoints = (Vector2 *)malloc(collidingPointCount * sizeof(Vector2));
            memcpy(verticeIndex->intersectionPoints, collidingPoints, collidingPointCount * sizeof(Vector2));
            verticeIndex->intersectionPointCount = collidingPointCount;

            free(collidingPoints);
            free(ifArray->identicalFunctions);
            free(ifArray);
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z)) {
            if (verticeIndex != head) {
                Vertice *current = head;
                while (current->next != verticeIndex) current = current->next;
                verticeIndex = current;
            }
        }
        if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R)) {
            if (verticeIndex->next != NULL) verticeIndex = verticeIndex->next;
        }

        if (IsKeyDown(KEY_SPACE)) {
            for (int i = 0; i < ifArray_D->identicalFunctionCount; i++) {
                DrawLineV(ifArray_D->identicalFunctions[i].start, ifArray_D->identicalFunctions[i].end, ifArray_D->identicalFunctions[i].color);
            }
        }
        if (verticeData_G != NULL) DrawLineStrip(verticeData_G, verticeCount_G, GREEN);
        for (int i = 0; i < verticeCount_G; i++) DrawCircleV(verticeData_G[i], 5, YELLOW);
        if (head->next != NULL) {
            current_D = head->next;
            while (current_D != verticeIndex->next) {
                DrawLineStrip(current_D->verticeData, current_D->verticeCount, GREEN);
                for (int i = 0; i < current_D->verticeCount; i++) DrawCircleV(current_D->verticeData[i], 5, YELLOW);

                if (IsKeyDown(KEY_SPACE)) {
                    for (int i = 0; i < current_D->intersectionPointCount; i++) {
                        DrawCircleV(current_D->intersectionPoints[i], 5, BLUE);
                    }
                }
                current_D = current_D->next;
            }
        }
        EndDrawing();
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
            if (ifArray_D != NULL) free(ifArray_D->identicalFunctions);
            free(ifArray_D);
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight);
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            camera.zoom += (wheel * ZOOM_INCREMENT);
            if (camera.zoom < ZOOM_INCREMENT) camera.zoom = ZOOM_INCREMENT;
            if (camera.zoom > 1) camera.zoom = 1;
            if (ifArray_D != NULL) free(ifArray_D->identicalFunctions);
            free(ifArray_D);
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            if (ifArray_D != NULL) free(ifArray_D->identicalFunctions);
            free(ifArray_D);
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight);
        }
    }


    UnloadTexture(HED_texture);
    if(ifArray_D != NULL){
        free(ifArray_D->identicalFunctions);
        free(ifArray_D);
    }
    Vertice *current_F = head;
    Vertice *next_F;
    while (current_F != NULL) {
        next_F = current_F->next;
        free(current_F->verticeData);
        free(current_F->intersectionPoints);
        free(current_F);
        current_F = next_F;
    }
    free(verticeData_G);

    CloseWindow();
    return 0;
}
