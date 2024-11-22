#include "raylib.h"

// Estrutura para o jogador
typedef struct Player {
    Rectangle rect;  // Retângulo do jogador (posição e tamanho)
    Vector2 velocity; // Velocidade de movimento
    bool isGrounded;  // Status do jogador sobre o chão
} Player;

// Estrutura para as plataformas
typedef struct Platform {
    Rectangle rect;  // Retângulo da plataforma (posição e tamanho)
} Platform;

#define PLATFORM_COUNT 5

int main(void) {
    InitWindow(800, 450, "Jogo de Plataforma Básico");

    // Instância do jogador
    Player player = {{100, 300, 50, 50}, {0, 0}, false};
    const float gravity = 500.0f;       // Gravidade
    const float jumpForce = -300.0f;    // Força do pulo
    const float moveSpeed = 200.0f;     // Velocidade de movimento do jogador

    // Configuração das plataformas
    Platform platforms[PLATFORM_COUNT] = {
        {{0, 400, 800, 50}},       // Chão
        {{200, 300, 100, 20}},    // Plataformas
        {{400, 250, 100, 20}},
        {{600, 200, 100, 20}},
        {{100, 150, 100, 20}}
    };

    SetTargetFPS(60);

    // Loop principal do jogo
    while (!WindowShouldClose()) {
        // Variavel para manter o movimento consistente
        float variacaoTempo = GetFrameTime();

        // Movimento e gravidade
        player.velocity.y += gravity * variacaoTempo;
        if (IsKeyDown(KEY_RIGHT)) player.velocity.x = moveSpeed;
        else if (IsKeyDown(KEY_LEFT)) player.velocity.x = -moveSpeed;
        else player.velocity.x = 0;

        // Pulo
        if (IsKeyPressed(KEY_SPACE)) {
            player.velocity.y = jumpForce;
            player.isGrounded = false;
        }

        // Atualiza a posição do jogador
        player.rect.x += player.velocity.x * variacaoTempo;
        player.rect.y += player.velocity.y * variacaoTempo;

        // Detecção de colisão
        player.isGrounded = false;
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            if (CheckCollisionRecs(player.rect, platforms[i].rect)) {
                // Lida com colisão por cima
                if (player.velocity.y > 0 && player.rect.y + player.rect.height <= platforms[i].rect.y + 10) {
                    player.rect.y = platforms[i].rect.y - player.rect.height;
                    player.velocity.y = 0;
                    player.isGrounded = true;
                } else {
                    // Empurra o jogador horizontalmente
                    if (player.velocity.x > 0) {
                        player.rect.x = platforms[i].rect.x - player.rect.wivariacaoTempoh;
                    } else if (player.velocity.x < 0) {
                        player.rect.x = platforms[i].rect.x + platforms[i].rect.wivariacaoTempoh;
                    }
                    player.velocity.x = 0;
                }
            }
        }

        // Evitar que o jogador caia p/ fora da tela
        if (player.rect.y > 850) {
            player.rect.y = 300;
            player.velocity.y = 0;
        }

        // Desenho
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Desenha jogador
        DrawRectangleRec(player.rect, RED);

        // Desenha plataformas
        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRec(platforms[i].rect, DARKGRAY);
        }

        DrawText("Jogo de Plataforma Básico", 10, 10, 20, BLACK);
        EndDrawing();
    }

    //Fecha janela
    CloseWindow();

    return 0;
}