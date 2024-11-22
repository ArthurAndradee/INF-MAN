#include "raylib.h"
#include <stdio.h>
// Estrutura p/ jogador
//TODO: Substituir retângulo por imagem estática e/ou boneco do INF-MAN
typedef struct Player {
    Rectangle rect;  // Jogador, por enquanto: (Retângulo (tamanho e posição))
    Vector2 velocity; // Velocidade de movimento x e y
    bool isGrounded;  // Para determinar se o jogador está pisando ou não
} Player;

// Estrutura p/ plataformas
typedef struct Platform {
    Rectangle rect;  // Retângulo (tamanho e posição)
} Platform;

#define PLATFORM_COUNT 5

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 450;
    InitWindow(screenWidth, screenHeight, "INF-MAN");

    Player player = {{100, 300, 50, 50}, {0, 0}, false};
    const float gravity = 500.0f;
    const float jumpForce = -300.0f;
    const float moveSpeed = 200.0f;

    Platform platforms[PLATFORM_COUNT] = {
        {{0, 400, 800, 50}},       // Chão
        {{200, 300, 100, 20}},    // Plataformas
        {{400, 250, 100, 20}},
        {{600, 200, 100, 20}},
        {{100, 150, 100, 20}}
    };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Gravidade
        player.velocity.y += gravity * dt;
        if (IsKeyDown(KEY_RIGHT)) player.velocity.x = moveSpeed;
        else if (IsKeyDown(KEY_LEFT)) player.velocity.x = -moveSpeed;
        else player.velocity.x = 0;

        // Pulo
        if (IsKeyPressed(KEY_SPACE) && player.isGrounded) {
            player.velocity.y = jumpForce;
            player.isGrounded = false;
        }

        // Atualiza posição do jogador com variação do tempo
        player.rect.x += player.velocity.x * dt;
        player.rect.y += player.velocity.y * dt;

        // Colisões
        player.isGrounded = false;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            if (CheckCollisionRecs(player.rect, platforms[i].rect)) {
                if (player.velocity.y > 0 && player.rect.y + player.rect.height <= platforms[i].rect.y + 10) {
                    //Colisão de cima para baixo entre o jogador e a plataforma
                    player.rect.y = platforms[i].rect.y - player.rect.height;
                    player.velocity.y = 0;
                    player.isGrounded = true;
                } else if (player.velocity.y < 0 && player.rect.y >= platforms[i].rect.y + platforms[i].rect.height - 10) {
                    //Colisão de baixo para cima entre o jogador e a plataforma
                    player.rect.y = platforms[i].rect.y + platforms[i].rect.height;
                    player.velocity.y = 0;
                    player.isGrounded = true;
                } else {
                    //Colisão horizontal entre o jogador e a plataforma
                    if (player.velocity.x > 0) {
                        player.rect.x = platforms[i].rect.x - player.rect.width;
                    } else if (player.velocity.x < 0) {
                        player.rect.x = platforms[i].rect.x + platforms[i].rect.width;
                    }
                    player.velocity.x = 0;
                }
            }
        }

        //Teleporta jogador para início quando cai pra fora da tela
        if (player.rect.y > screenHeight) {
            player.rect.y = 300;
            player.velocity.y = 0;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawRectangleRec(player.rect, BLUE);

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRec(platforms[i].rect, DARKGRAY);
        }

        char deltaTime[300];
        sprintf(deltaTime, "%g", dt);

        DrawText(deltaTime, 10, 30, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
