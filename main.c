//gcc -o main main.c -L/home/di0n/vcpkg/installed/x64-linux/lib -lraylib -lcjson -lm -lX11 -Wall
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <raylib.h>
#include <raymath.h>
#include <cjson/cJSON.h>

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
    Vertice *contour_data_head;
    Vertice *contour_data_tail;
} TextureBound;

void loadImage(TextureBound *texture_bound, Camera2D *camera, bool *scaleImage, bool *moveImage){
    const int SCALE_SIGN_SIZE = 50;
    DrawTextureV(texture_bound->texture, (Vector2){.x = texture_bound->rectangle.x, .y = texture_bound->rectangle.y}, WHITE);
    if (CheckCollisionPointRec(GetScreenToWorld2D(GetMousePosition(), *camera), texture_bound->rectangle)) {
        DrawRectangleLinesEx(texture_bound->rectangle, 10.0f, LIME);
        Vector2 trinaglePointA = {.x = (texture_bound->rectangle.x + texture_bound->rectangle.width) - SCALE_SIGN_SIZE, .y = texture_bound->rectangle.y + texture_bound->rectangle.height};
        Vector2 trinaglePointB = {.x = (texture_bound->rectangle.x + texture_bound->rectangle.width), .y = texture_bound->rectangle.y + texture_bound->rectangle.height};
        Vector2 trinaglePointC = {.x = texture_bound->rectangle.x + texture_bound->rectangle.width, .y = (texture_bound->rectangle.y + texture_bound->rectangle.height) - SCALE_SIGN_SIZE};
        DrawTriangle(trinaglePointA, trinaglePointB, trinaglePointC, LIME);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsKeyDown(KEY_LEFT_CONTROL)) {
            if (CheckCollisionPointTriangle(GetScreenToWorld2D(GetMousePosition(), *camera), trinaglePointA, trinaglePointB, trinaglePointC)) *scaleImage = true;
            else *moveImage = true;
        }
    }

    if (*scaleImage && texture_bound->rectangle.width > SCALE_SIGN_SIZE && texture_bound->rectangle.height > SCALE_SIGN_SIZE) {
        float old_width = texture_bound->rectangle.width;
        float old_height = texture_bound->rectangle.height;
        texture_bound->rectangle.width = GetScreenToWorld2D(GetMousePosition(), *camera).x - texture_bound->rectangle.x;
        texture_bound->rectangle.height = GetScreenToWorld2D(GetMousePosition(), *camera).y - texture_bound->rectangle.y;
        Vertice *current_vertice = texture_bound->contour_data_head;
        while(current_vertice != NULL) {
            int temp_count_arr[2] = {current_vertice->verticeCount, current_vertice->intersectionPointCount};
            Vector2 *temp_points_arr[2] = {current_vertice->verticeData, current_vertice->intersectionPoints};

        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < temp_count_arr[i]; j++) {
                temp_points_arr[i][j].x = texture_bound->rectangle.x + (temp_points_arr[i][j].x - texture_bound->rectangle.x) * (texture_bound->rectangle.width / old_width);
                temp_points_arr[i][j].y = texture_bound->rectangle.y + (temp_points_arr[i][j].y - texture_bound->rectangle.y) * (texture_bound->rectangle.height / old_height);
            }
        }
            current_vertice = current_vertice->next;
        }

        if (texture_bound->rectangle.width < SCALE_SIGN_SIZE) texture_bound->rectangle.width = SCALE_SIGN_SIZE;
        if (texture_bound->rectangle.height < SCALE_SIGN_SIZE) texture_bound->rectangle.height = SCALE_SIGN_SIZE;

        texture_bound->texture.width = texture_bound->rectangle.width;
        texture_bound->texture.height = texture_bound->rectangle.height;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsKeyReleased(KEY_LEFT_CONTROL)) *scaleImage = false;
    }
    if (*moveImage && !*scaleImage) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera->zoom);
        texture_bound->rectangle.x -= delta.x;
        texture_bound->rectangle.y -= delta.y;
        Vertice *current_vertice = texture_bound->contour_data_head;
        while(current_vertice != NULL) {
            int temp_count_arr[2] = {current_vertice->verticeCount, current_vertice->intersectionPointCount};
            Vector2 *temp_points_arr[2] = {current_vertice->verticeData, current_vertice->intersectionPoints};

            for(int i = 0; i < 2; i++) {
                for (int j = 0; j < temp_count_arr[i]; j++) {
                    temp_points_arr[i][j].x -= delta.x;
                    temp_points_arr[i][j].y -= delta.y;
                }
            } 
            current_vertice = current_vertice->next;
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) || IsKeyReleased(KEY_LEFT_CONTROL)) *moveImage = false;
    }
}

IFArray *generateIdenticalFunctions(Vector2 worldTopLeft, Vector2 worldBottomRight, int identicalFunctionGap) {
    float identicalFunctionStartX = floorf(worldTopLeft.x / identicalFunctionGap) * identicalFunctionGap;
    float identicalFunctionStartY = floorf(worldTopLeft.y / identicalFunctionGap) * identicalFunctionGap;
    int identicalFunctionCountX = (abs(worldBottomRight.x - worldTopLeft.x)) / identicalFunctionGap;
    int identicalFunctionCountY = (abs(worldBottomRight.y - worldTopLeft.y)) / identicalFunctionGap;

    IdenticalFunction *identicalFunctions = (IdenticalFunction *)malloc((identicalFunctionCountX + identicalFunctionCountY) * sizeof(IdenticalFunction));
    for (int i = 1; i < identicalFunctionCountX; i++) {
        float x = identicalFunctionStartX + (i * identicalFunctionGap);
        IdenticalFunction identicalFunction = {.start = (Vector2){x, worldTopLeft.y}, .end = (Vector2){x, worldBottomRight.y}, .color = RED};
        identicalFunctions[i - 1] = identicalFunction;
    }
    for (int i = 1; i < identicalFunctionCountY; i++) {
        float y = identicalFunctionStartY + (i * identicalFunctionGap);
        IdenticalFunction identicalFunction = {.start = (Vector2){worldTopLeft.x, y}, .end = (Vector2){worldBottomRight.x, y}, .color = RED};
        identicalFunctions[i - 1 + identicalFunctionCountX] = identicalFunction;
    }

    IFArray *ifArray = (IFArray *)malloc(sizeof(IFArray));
    ifArray->identicalFunctionCount = identicalFunctionCountX + identicalFunctionCountY;
    ifArray->identicalFunctions = identicalFunctions;

    return ifArray;
}

int readJSON(char **filePath, int *identicalFunctionGap, Vertice **contour_head, Vertice **verticeIndex){
    FILE *file = fopen("config/settings.json", "r");
    if (file == NULL) { fprintf(stderr, "Failed to open JSON file.\n"); return 1;}
    

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *jsonBuffer = (char *)malloc(fileSize + 1);
    if (jsonBuffer == NULL) { fprintf(stderr, "Memory allocation error.\n"); fclose(file); return 1;}

    size_t bytesRead = fread(jsonBuffer, 1, fileSize, file);
    jsonBuffer[bytesRead] = '\0';

    fclose(file);

    cJSON *root = cJSON_Parse(jsonBuffer);
    if (root == NULL) { fprintf(stderr, "Failed to parse JSON data.\n"); free(jsonBuffer); return 1;}

    cJSON *filePathItem = cJSON_GetObjectItem(root, "filePath");
    if (filePathItem && filePathItem->type == cJSON_String) {*filePath = strdup(filePathItem->valuestring);}

    cJSON *identicalFunctionGapItem = cJSON_GetObjectItem(root, "identicalFunctionGap");
    if (identicalFunctionGapItem && identicalFunctionGapItem->type == cJSON_Number) {*identicalFunctionGap = identicalFunctionGapItem->valueint;}

    cJSON *contours_item = cJSON_GetObjectItem(root, "contour_data");
    if (contours_item == NULL || contours_item->type != cJSON_Array) {printf("Invalid or missing 'contour_data' array in the JSON.\n"); cJSON_Delete(root); return 1;}
    
    int contour_count = cJSON_GetArraySize(contours_item);

    Image temp_image = LoadImage(*filePath);
    int temp_image_width = temp_image.width;
    int temp_image_height = temp_image.height;
    UnloadImage(temp_image);

    IFArray *ifArray = generateIdenticalFunctions((Vector2){.x = 0, .y = -temp_image_height}, (Vector2){.x = temp_image_width, .y = 0}, *identicalFunctionGap); 

    for(int i = 0; i < contour_count; i++){
        cJSON *contour_object = cJSON_GetArrayItem(contours_item, i);
        cJSON *points_array = cJSON_GetObjectItem(contour_object, "points");

        int point_count = cJSON_GetArraySize(points_array);
        Vector2 *contour_verticeData = (Vector2 *)malloc(point_count * sizeof(Vector2));

        for(int j = 0; j < point_count; j++){
            cJSON *point_array = cJSON_GetArrayItem(points_array, j);
            cJSON *point = cJSON_GetArrayItem(point_array, 0);

            contour_verticeData[j] = (Vector2){.x = cJSON_GetArrayItem(point, 0)->valueint, .y = cJSON_GetArrayItem(point, 1)->valueint - temp_image_height};
        }
        int collidingPointCount = 0;
        Vector2 *collidingPoints = (Vector2 *)malloc(sizeof(Vector2));
        if(point_count == 1){
            for(int j = 0; j < ifArray->identicalFunctionCount; j++){
                if(CheckCollisionPointLine(contour_verticeData[0], ifArray->identicalFunctions[j].start, ifArray->identicalFunctions[j].end, 10)){
                    collidingPoints[collidingPointCount] = contour_verticeData[0];
                    collidingPointCount++;
                    collidingPoints = (Vector2 *)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Vector2));
                }
            }
        }else{
            for (int j = 0; j < point_count - 1; j++) {
                for (int k = 0; k < ifArray->identicalFunctionCount; k++) {
                    if (CheckCollisionLines(contour_verticeData[j], contour_verticeData[j + 1], ifArray->identicalFunctions[k].start, ifArray->identicalFunctions[k].end, &collidingPoints[collidingPointCount])) {
                        collidingPointCount++;
                        collidingPoints = (Vector2 *)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Vector2));
                    }
                }
            }
        } 

        Vertice *newVertice = (Vertice *)malloc(sizeof(Vertice));
        newVertice->verticeData = (Vector2 *)malloc(point_count * sizeof(Vector2));
        memcpy(newVertice->verticeData, contour_verticeData, point_count * sizeof(Vector2));
        newVertice->verticeCount = point_count;
        newVertice->next = (*verticeIndex)->next;
        newVertice->intersectionPoints = (Vector2 *)malloc(collidingPointCount * sizeof(Vector2));
        memcpy(newVertice->intersectionPoints, collidingPoints, collidingPointCount * sizeof(Vector2));
        newVertice->intersectionPointCount = collidingPointCount;

        (*verticeIndex)->next = newVertice;
        (*verticeIndex) = newVertice;

        free(contour_verticeData);
        free(collidingPoints);

    }
    free(ifArray->identicalFunctions);
    free(ifArray);

    cJSON_Delete(root);
    free(jsonBuffer);
    return 0;
}

int main(int argc, char **argv) {	
    const int WIDTH = 800;
    const int HEIGHT = 600;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WIDTH, HEIGHT, "Window");
    SetTargetFPS(144);

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

    int identicalFunctionGap = 50;
    IFArray *ifArray_D = NULL;

    char *fileLocation = NULL;
    bool importImage = false;
    bool scaleImage = false;
    bool moveImage = false;
    TextureBound texture_bound;
    Texture2D texture;


    if(!readJSON(&fileLocation, &identicalFunctionGap, &head, &verticeIndex)){
        Image image = LoadImage(fileLocation);
        free(fileLocation);
        fileLocation = NULL;
        texture = LoadTextureFromImage(image);
        UnloadImage(image);
        texture_bound.texture = texture; 
        texture_bound.rectangle = (Rectangle){.x = 0, .y = 0 - texture.height, .width = (float)texture.width, .height = (float)texture.height};
        texture_bound.contour_data_head = head->next;
        texture_bound.contour_data_tail = verticeIndex;
        importImage = true;
    }
    
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

        if(importImage) {loadImage(&texture_bound, &camera, &scaleImage, &moveImage);} 

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

            IFArray *ifArray = generateIdenticalFunctions(worldTopLeft, worldBottomRight, identicalFunctionGap);
            int collidingPointCount = 0;
            Vector2 *collidingPoints = (Vector2 *)malloc(sizeof(Vector2));
            if(verticeIndex->verticeCount == 1){
                for(int j = 0; j < ifArray->identicalFunctionCount; j++){
                    if(CheckCollisionPointLine(verticeIndex->verticeData[0], ifArray->identicalFunctions[j].start, ifArray->identicalFunctions[j].end, 10)){
                        collidingPoints[collidingPointCount] = verticeIndex->verticeData[0];
                        collidingPointCount++;
                        collidingPoints = (Vector2 *)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Vector2));
                    }
                }
            }else{
                for (int i = 0; i < verticeIndex->verticeCount - 1; i++) {
                    for (int j = 0; j < ifArray->identicalFunctionCount; j++) {
                        if (CheckCollisionLines(verticeIndex->verticeData[i], verticeIndex->verticeData[i + 1], ifArray->identicalFunctions[j].start, ifArray->identicalFunctions[j].end, &collidingPoints[collidingPointCount])) {
                            collidingPointCount++;
                            collidingPoints = (Vector2 *)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Vector2));
                        }
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
        for (int i = 0; i < verticeCount_G; i++) DrawCircleV(verticeData_G[i], 5, YELLOW);
        if (verticeData_G != NULL) DrawLineStrip(verticeData_G, verticeCount_G, GREEN);
        if (head->next != NULL) {
            current_D = head->next;
            while (current_D != verticeIndex->next) {
                for (int i = 0; i < current_D->verticeCount; i++) DrawCircleV(current_D->verticeData[i], 5, YELLOW);
                DrawLineStrip(current_D->verticeData, current_D->verticeCount, GREEN);

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
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight, identicalFunctionGap);
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            camera.zoom += (wheel * ZOOM_INCREMENT);
            if (camera.zoom < ZOOM_INCREMENT) camera.zoom = ZOOM_INCREMENT;
            if (camera.zoom > 1) camera.zoom = 1;
            if (ifArray_D != NULL) free(ifArray_D->identicalFunctions);
            free(ifArray_D);
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight, identicalFunctionGap);
        }
        if (IsKeyPressed(KEY_SPACE)) {
            if (ifArray_D != NULL) free(ifArray_D->identicalFunctions);
            free(ifArray_D);
            ifArray_D = generateIdenticalFunctions(worldTopLeft, worldBottomRight, identicalFunctionGap);
        }
    }


    UnloadTexture(texture);
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
