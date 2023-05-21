#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

int main()
{
    const int WIDTH = 800;
    const int HEIGHT = 600;

    InitWindow(WIDTH, HEIGHT, "Window");

    Camera2D camera = {.offset = (Vector2){0, 0}, .target = (Vector2){0 - (float)GetScreenWidth() / 2, 0 - (float)GetScreenHeight() / 2}, .rotation = 0.0f, .zoom = 1.0f};

    SetTargetFPS(60);  
    const int CELL_SIZE = 50;
    const float ZOOM_INCREMENT = 0.125f;

    int algorithmStep = 0;

    typedef struct Vertice{
	Vector2 *verticeData;
	int verticeCount;
	struct Vertice* next;
    } Vertice;

    typedef struct IdenticalFunction{
	Vector2 start, end;
	Color color;
    } IdenticalFunction;
    typedef struct Point{
	Vector2 pos;
	Color color;
    } Point;

    Vertice *head = (Vertice*)malloc(sizeof(Vertice));
    head->verticeData = NULL;
    head->verticeCount = 0;
    head->next = NULL;

    Vertice *current_D = (Vertice*)malloc(sizeof(Vertice));
    Vertice *verticeIndex = head;

    Vector2 *verticeData_G = NULL; 
    int verticeCount_G = 0;

    while (!WindowShouldClose()) {
	BeginDrawing();
        ClearBackground(BLACK);
	BeginMode2D(camera);
    	
	Vector2 cameraPosition = {camera.offset.x, camera.offset.y};
	Vector2 worldTopLeft = GetScreenToWorld2D(cameraPosition, camera);
	Vector2 worldBottomRight = GetScreenToWorld2D((Vector2){ cameraPosition.x + GetScreenWidth() / camera.zoom, cameraPosition.y + GetScreenHeight() / camera.zoom}, camera);

	float gridStartX = floorf(worldTopLeft.x / CELL_SIZE) * CELL_SIZE;
        float gridStartY = floorf(worldTopLeft.y / CELL_SIZE) * CELL_SIZE;
	
	int cellCountVER = (worldBottomRight.x - worldTopLeft.x) / CELL_SIZE;
	int cellCountHOR = (worldBottomRight.y - worldTopLeft.y) / CELL_SIZE;

       	for (int i = 1; i < cellCountVER; i++) {
	    float x = gridStartX + (i * CELL_SIZE);
	    DrawLine(x, worldTopLeft.y, x, worldBottomRight.y, WHITE);
	    const char* numberText = TextFormat("%d", (int)x);
	    DrawText(numberText, x + 3, 3, 15, YELLOW);
        }
	for (int i = 1; i < cellCountHOR; i++) {
	    float y = gridStartY + (i * CELL_SIZE);
	    DrawLine(worldTopLeft.x, y, worldBottomRight.x, y, WHITE);
	    const char* numberText = TextFormat("%d", (int)-y);
	    DrawText(numberText, 3, y + 3, 15, YELLOW);
        }

	DrawLine(worldTopLeft.x, 0, worldBottomRight.x, 0, ORANGE);
	DrawLine(0, worldTopLeft.y, 0, worldBottomRight.y, ORANGE);

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && algorithmStep == 0) {
	    verticeData_G = (Vector2*)realloc(verticeData_G, (verticeCount_G + 1) * sizeof(Vector2));
	    verticeData_G[verticeCount_G] = GetScreenToWorld2D(GetMousePosition(), camera);
	    verticeCount_G++;
	}
	if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            Vertice *newVertice = (Vertice*)malloc(sizeof(Vertice));
            newVertice->verticeData = (Vector2*)malloc(verticeCount_G * sizeof(Vector2));
            memcpy(newVertice->verticeData, verticeData_G, verticeCount_G * sizeof(Vector2));
            newVertice->verticeCount = verticeCount_G;
            newVertice->next = verticeIndex->next;

            verticeIndex->next = newVertice;
            verticeIndex = newVertice;

	    verticeCount_G = 0;
	}
	if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_Z) && algorithmStep == 0){
	    if(verticeIndex != head){
		Vertice *current = head;
		while(current->next != verticeIndex) current = current->next;
		verticeIndex = current;
	    }
    	}
	if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_R) && algorithmStep == 0){
	    if(verticeIndex->next != NULL) verticeIndex = verticeIndex->next;
	}

	if(verticeData_G != NULL) DrawLineStrip(verticeData_G, verticeCount_G, GREEN);
	if(head->next != NULL){
	    current_D = head->next;
	    while(current_D != verticeIndex->next){
		DrawLineStrip(current_D->verticeData, current_D->verticeCount, GREEN);
		current_D = current_D->next;
	    }
	}

	if(algorithmStep == 1) {
	    const int IDENTICAL_FUNCTION_GAP = 50;
	    float identicalFunctionStartX = floorf(worldTopLeft.x / IDENTICAL_FUNCTION_GAP) * IDENTICAL_FUNCTION_GAP;
	    float identicalFunctionStartY = floorf(worldTopLeft.y / IDENTICAL_FUNCTION_GAP) * IDENTICAL_FUNCTION_GAP;
	    int identicalFunctionCountX = (worldBottomRight.x - worldTopLeft.x) / IDENTICAL_FUNCTION_GAP;
	    int identicalFunctionCountY = (worldBottomRight.y - worldTopLeft.y) / IDENTICAL_FUNCTION_GAP;

	    IdenticalFunction *identicalFunctions = (IdenticalFunction*)realloc(identicalFunctions, (identicalFunctionCountX + identicalFunctionCountY) * sizeof(IdenticalFunction));
	    for(int i = 1; i < identicalFunctionCountX; i++){
		float x  = identicalFunctionStartX + (i *IDENTICAL_FUNCTION_GAP);
	   	IdenticalFunction identicalFunction = {.start = (Vector2){x, worldTopLeft.y}, .end = (Vector2){x, worldBottomRight.y}, .color = RED};
		identicalFunctions[i -1] = identicalFunction;
	    }
	    for (int i = 1; i < identicalFunctionCountY; i++){
		float y = identicalFunctionStartY + (i * IDENTICAL_FUNCTION_GAP);
		IdenticalFunction identicalFunction = {.start = (Vector2){worldTopLeft.x, y}, .end = (Vector2){worldBottomRight.x, y}, .color = RED};
		identicalFunctions[i-1 + identicalFunctionCountX] = identicalFunction;
	    }
	    for(int i = 0; i < identicalFunctionCountX + identicalFunctionCountY; i++){
		DrawLineV(identicalFunctions[i].start, identicalFunctions[i].end, identicalFunctions[i].color);
	    }

	    int collidingPointCount = 0;
	    Point *collidingPoints = (Point*)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Point));
	    Vertice *iterator = head;
	    while(iterator != NULL){
		for(int i = 0; i < iterator->verticeCount; i++){
		    for(int j = 0; j < identicalFunctionCountX + identicalFunctionCountY; j++){
			if(CheckCollisionPointLine(iterator->verticeData[i], identicalFunctions[j].start, identicalFunctions[j].end, 10)){
			    collidingPoints[collidingPointCount] = (Point){.pos = iterator->verticeData[i], .color = BLUE};
			    collidingPointCount++;
			    collidingPoints = (Point*)realloc(collidingPoints, (collidingPointCount + 1) * sizeof(Point));
			}
		    }
		}
		iterator = iterator->next;
	    }
	    for(int i = 0; i < collidingPointCount; i++){
		DrawCircleV(collidingPoints[i].pos, 5, collidingPoints[i].color);
	    }

	    static int delayTimer = 600; 
	    if (delayTimer <= 0) {
		free(identicalFunctions);
		free(collidingPoints);
		algorithmStep++;
	    } else {
		delayTimer--;
	    }
	}

	EndDrawing();
    	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){
    	    Vector2 delta = GetMouseDelta();
	    delta = Vector2Scale(delta, -1.0f/camera.zoom);
	    camera.target = Vector2Add(camera.target, delta);
	}
	float wheel = GetMouseWheelMove();
        if (wheel != 0){	    
	    camera.zoom += (wheel*ZOOM_INCREMENT);
    	    if (camera.zoom < ZOOM_INCREMENT) camera.zoom = ZOOM_INCREMENT;
	    if (camera.zoom > 1) camera.zoom = 1;
	}
	if(IsKeyPressed(KEY_SPACE) && algorithmStep == 0){
	    algorithmStep = 1; 
	}

    }

    Vertice *current_F = head;
    Vertice *next_F;
    while(current_F != NULL){
	next_F = current_F->next;
	free(current_F->verticeData);
	free(current_F);
	current_F = next_F;
    }
    free(verticeData_G);

    CloseWindow();       
    return 0;
}


