#include "raylib.h"
#include <stdio.h>

// Estrutura pro jogador
typedef struct Player {
    Vector2 position; // Coordenadas do jogador (x, y)
    Vector2 velocity; // Velocidade de movimento (x, y)
    Rectangle rect;   // Retângulo para detecção de colisão
    bool isGrounded;  // Determina se o jogador está no chão
} Player;

// Estrutura paraa as plataformas
typedef struct Platform {
    Vector2 position; // Coordenadas da plataforma (x, y)
    Rectangle rect;   // Retângulo para detecção de colisão
} Platform;

#define PLATFORM_COUNT 5

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "INF-MAN");

    // Inicialização do jogador
    Player player = {
        {100, 300}, 
        {0, 0}, 
        {100, 300, 50, 50}, 
        false
    };

    const float gravity = 500.0f;
    const float jumpForce = -300.0f;
    const float moveSpeed = 200.0f;

    // Inicialização das plataformas
    Platform platforms[PLATFORM_COUNT] = {
        {{0, 400}, {0, 400, 800, 50}},       // Chão
        {{200, 300}, {200, 300, 100, 20}},  // Plataformas
        {{400, 250}, {400, 250, 100, 20}},
        {{600, 200}, {600, 200, 100, 20}},
        {{100, 150}, {100, 150, 100, 20}}
    };

    // Configuração da câmera
    Camera2D camera = {0};
    camera.target = (Vector2){player.position.x + player.rect.width / 2, player.position.y + player.rect.height / 2};
    camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    camera.zoom = 1.0f;
    camera.rotation = 0.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Aplica efeito da gravidade
        player.velocity.y += gravity * dt;

        // Controle do jogador
        if (IsKeyDown(KEY_RIGHT)) player.velocity.x = moveSpeed;
        else if (IsKeyDown(KEY_LEFT)) player.velocity.x = -moveSpeed;
        else player.velocity.x = 0;

        // Pulo
        if (IsKeyPressed(KEY_SPACE) && player.isGrounded) {
            player.velocity.y = jumpForce;
            player.isGrounded = false;
        }

        // Atualiza a posição do jogador
        player.position.x += player.velocity.x * dt;
        player.position.y += player.velocity.y * dt;

        // Atualiza o retângulo do jogador pra colisão
        player.rect.x = player.position.x;
        player.rect.y = player.position.y;

        // Lida com as colisões com plataformas
        player.isGrounded = false;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            platforms[i].rect.x = platforms[i].position.x;
            platforms[i].rect.y = platforms[i].position.y;

            if (CheckCollisionRecs(player.rect, platforms[i].rect)) {
                if (player.velocity.y > 0 && player.position.y + player.rect.height <= platforms[i].position.y + 10) {
                    // Colisão de cima para baixo
                    player.position.y = platforms[i].position.y - player.rect.height;
                    player.velocity.y = 0;
                    player.isGrounded = true;
                } else if (player.velocity.y < 0 && player.position.y >= platforms[i].position.y + platforms[i].rect.height - 10) {
                    // Colisão de baixo para cima
                    player.position.y = platforms[i].position.y + platforms[i].rect.height;
                    player.velocity.y = 0;
                } else {
                    // Colisão horizontal
                    if (player.velocity.x > 0) {
                        player.position.x = platforms[i].position.x - player.rect.width;
                    } else if (player.velocity.x < 0) {
                        player.position.x = platforms[i].position.x + platforms[i].rect.width;
                    }
                    player.velocity.x = 0;
                }
            }
        }

        // Teleporta o jogador pro início quando cai pra fora da tela
        if (player.position.y > screenHeight) {
            player.position.y = 300;
            player.velocity.y = 0;
        }

        // Atualizar o alvo da câmera
        camera.target = (Vector2){player.position.x + player.rect.width / 2, player.position.y + player.rect.height / 2};

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        DrawRectangleRec(player.rect, BLUE);

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRec(platforms[i].rect, DARKGRAY);
        }

        EndMode2D();

        DrawText("INF-MAN", 10, 10, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}