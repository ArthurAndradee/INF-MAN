#include "raylib.h"
#include <stdio.h>
#define MAX_PROJECTILES 10
#define PLATFORM_COUNT 5

// Estrutura pro jogador
typedef struct Player {
    Vector2 position; // Coordenadas do jogador (x, y)
    Vector2 velocity; // Velocidade de movimento (x, y)
    Rectangle rect;   // Retângulo para detecção de colisão
    bool isGrounded;  // Determina se o jogador está no chão
    bool facingRight; // Direção do jogador
    bool isShooting;
} Player;

// Estrutura para as plataformas
typedef struct Platform {
    Vector2 position; // Coordenadas da plataforma (x, y)
    Rectangle rect;   // Retângulo para detecção de colisão
} Platform;

// Estrutura pro projetil
typedef struct Projectile {
    Rectangle rect;  // Propriedades de posição e tamanho
    Vector2 speed;   // Velocidade do projétil
    bool active;     // Indica se o projétil está ativo
} Projectile;

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "INF-MAN");

    // Inicialização do jogador
    Player player = {
        {100, 300},
        {0, 0},
        {100, 300, 50, 50},
        false,
        true, // Inicia olhando para a direita
        false
    };

    // Variaveis das propriedades do projetil
    float projectileWidth = 20.0f;  // Largura do disparo
    float projectileHeight = 10.0f; // Altura do disparo

    // Variaveis de controle do mundo
    const float gravity = 500.0f;
    const float jumpForce = -300.0f;
    const float moveSpeed = 200.0f;
    const float projectileSpeed = 400.0f;

    // Inicialização das plataformas
    Platform platforms[PLATFORM_COUNT] = {
        {{0, 400}, {0, 400, 800, 50}},       // Chão
        {{200, 300}, {200, 300, 100, 20}},  // Plataformas (e tbm todas abaixo)
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

    // Configuração da textura do jogador
    Texture2D infmanTex = LoadTexture("player-sheet.png");

    // Configuração do frame do jogador
    int frameWidth = infmanTex.width / 12;
    Rectangle frameRec = {0.0f, 0.0f, (float)frameWidth, (float)infmanTex.height};

    // Variaveis pra controle da textura que apresenta o estado do jogador
    Vector2 textureOrigin = {0, 0};
    unsigned currentFrame = 0;
    float frameTimer = 0.0f;
    const float frameSpeed = 0.15f; // Velocidade de animação

    // Inicialização dos projeteis
    Projectile projectiles[MAX_PROJECTILES] = {0};
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Aplica efeito da gravidade
        player.velocity.y += gravity * dt;

        // Controle do jogador
        if (IsKeyDown(KEY_RIGHT)) {
            player.velocity.x = moveSpeed;
            player.facingRight = true;
        } else if (IsKeyDown(KEY_LEFT)) {
            player.velocity.x = -moveSpeed;
            player.facingRight = false;
        } else {
            player.velocity.x = 0;
        }

        // Pulo
        if (IsKeyPressed(KEY_SPACE) && player.isGrounded) {
            player.velocity.y = jumpForce;
            player.isGrounded = false;
        }

        // Disparo do projetil
        if (IsKeyPressed(KEY_Z)) {
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (!projectiles[i].active) {
                    projectiles[i].rect = (Rectangle){
                        player.position.x + (player.facingRight ? player.rect.width : -projectileWidth),
                        player.position.y + player.rect.height / 2 - projectileHeight / 2,
                        projectileWidth,
                        projectileHeight
                    };
                    projectiles[i].speed = (Vector2){player.facingRight ? projectileSpeed : -projectileSpeed, 0};
                    projectiles[i].active = true;
                    break;
                }
            }
        }
        // Animação do jogador disparando
        player.isShooting = IsKeyDown(KEY_Z);

        // Atualiza a posição do jogador
        player.position.x += player.velocity.x * dt;
        player.position.y += player.velocity.y * dt;

        // Atualiza o jogador pra colisão
        player.rect.x = player.position.x;
        player.rect.y = player.position.y;

        // Colisões com plataformas
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

        // Atualiza projéteis
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active) {
                projectiles[i].rect.x += projectiles[i].speed.x * dt;
                projectiles[i].rect.y += projectiles[i].speed.y * dt;

                // Desativa projétil se sair da tela
                if (projectiles[i].rect.x < 0 || projectiles[i].rect.x > screenWidth) {
                    projectiles[i].active = false;
                }
            }
        }

        // Teleporta o jogador pro início quando cai pra fora do mapa
        if (player.position.y > screenHeight) {
            player.position.y = 300;
            player.position.x = 30;
            player.velocity.y = 0;
            player.velocity.x = 0;
        }

        // Atualiza o centro da câmera
        camera.target = (Vector2){player.position.x + player.rect.width / 2, player.position.y + player.rect.height / 2};

        // Animação do jogador
        frameTimer += dt;
        if (frameTimer >= frameSpeed) {
            frameTimer = 0.0f;

            if (!player.isGrounded) {
                currentFrame = player.isShooting ? 10 : 5; // Pulando (atirando ou não)
            } else if (player.velocity.x != 0) {
                // Movendo
                if (player.isShooting) {
                    currentFrame = (currentFrame < 6 || currentFrame > 8) ? 7 : currentFrame + 1;
                } else {
                    currentFrame = (currentFrame < 1 || currentFrame > 3) ? 2 : currentFrame + 1;  // 1, 2, 3 (Ciclo)
                }
            } else {
                currentFrame = player.isShooting ? 7 : 0; // Parado (atirando ou não)
            }
        }

        // Define o quadro atual e ajusta a direção
        frameRec.x = frameWidth * currentFrame;
        frameRec.width = player.facingRight ? -frameWidth : frameWidth;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        DrawTexturePro(
            infmanTex, // Textura fonte
            frameRec,  // Retângulo da textura (fonte)
            player.rect, // Retângulo de destino (para o jogador)
            textureOrigin, // Origem
            0.0f, // Rotação
            WHITE // Cor
        );

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            DrawRectangleRec(platforms[i].rect, DARKGRAY);
        }

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active) {
                DrawRectangleRec(projectiles[i].rect, YELLOW);
            }
        }

        EndMode2D();

        DrawText("INF-MAN", 10, 10, 20, BLACK);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
