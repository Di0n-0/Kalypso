#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    const int WIDTH = 800;
    const int HEIGHT = 600;

    InitWindow(WIDTH, HEIGHT, "Window");

    Camera2D camera = {.offset = (Vector2){0, 0}, .target = (Vector2){0 - (float)GetScreenWidth() / 2, 0 - (float)GetScreenHeight() / 2}, .rotation = 0.0f, .zoom = 1.0f};

    SetTargetFPS(60);  
    const int CELL_SIZE = 50;
    const float ZOOM_INCREMENT = 0.125f;

    Vector2 *points = NULL;
    int pointCount = 0;


    
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

	if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
	    points = (Vector2*)realloc(points, (pointCount + 1) * sizeof(Vector2));
	    points[pointCount] = GetScreenToWorld2D(GetMousePosition(), camera);
	    pointCount++;
	}

	DrawLineStrip(points, pointCount, GREEN);

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


    }
    
    free(points);
    pointCount = 0;

    CloseWindow();       
    return 0;
}


