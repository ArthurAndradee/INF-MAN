#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_PROJECTILES 10
#define BLOCK_SIZE 16
#define MAX_WIDTH 1000
#define MAX_HEIGHT 100
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600

typedef struct Player {
    Vector2 position;   // Coordenadas (x, y)
    Vector2 velocity;   // Velocidade (x, y)
    Rectangle rect;     // Retangulo pra colisao
    bool isGrounded;    // Determina se está no chao
    bool facingRight;   // Determina direcao que está encarando
    bool isShooting;
    int health;
    int points;

} Player;

typedef struct Enemy {
    Vector2 position;   // coordenadas (x, y)
    Vector2 velocity;   // velocidade (x, y)
    Rectangle rect;     // Retangulo pra colisao
    Vector2 minPosition; // posicao minima (x, y)
    Vector2 maxPosition; // posicao maxima (x, y)
    int health;         // pontos de vida
    bool active;        // flag para determinar se o inimigo está ativo
} Enemy;

typedef struct Projectile {
    Rectangle rect;  // Propriedades de posição e tamanho
    Vector2 speed;   // Velocidade do projétil
    bool active;     // Indica se o projétil está ativo
} Projectile;

// Le o mapa a partir de um arquivo
void LoadMap(const char* filename, char map[MAX_HEIGHT][MAX_WIDTH], int* rows, int* cols) {
    FILE* file = fopen(filename, "r");  // Le o arquivo
    if (!file) {
        perror("Failed to open file");
        return;
    }

    // Inicia contagem das linhas e colunas em 0
    *rows = 0;
    *cols = 0;

    char line[MAX_WIDTH];  // buffer pra armazenar cada linha e coluna
    while (fgets(line, sizeof(line), file)) {  // Le linha por linha
        size_t len = strlen(line);  // Recebe o comprimento da linha atual
        if (line[len - 1] == '\n') line[len - 1] = '\0';  // Remove o caractere de nova linha

        // Copia linha pro array
        strncpy(map[*rows], line, MAX_WIDTH);

        (*rows)++;  // Incrementa contagem da linha
        if (len > *cols) *cols = len - 1;  // Atualiza contagem da coluna se a linha atual é maior do que a anterior
    }

    fclose(file);
}

// Limpa a memória alocada pro mapa
void FreeMap(char** map, int rows) {
    for (int i = 0; i < rows; i++) {
        free(map[i]);
    }
    free(map);
}

// Verifica colisao entre jogador e blocos
bool CheckCollisionWithBlock(Rectangle player, Rectangle block, Vector2* correction) {
    if (CheckCollisionRecs(player, block)) {
        // TODO: Comentar o que fmin e fmax fazem e explicar código
        float overlapX = fmin(player.x + player.width, block.x + block.width) - fmax(player.x, block.x);
        float overlapY = fmin(player.y + player.height, block.y + block.height) - fmax(player.y, block.y);

        // Resolve com base na menor sobreposicao
        if (overlapX < overlapY) {
            correction->x = (player.x < block.x) ? -overlapX : overlapX;
        } else {
            correction->y = (player.y < block.y) ? -overlapY : overlapY;
        }

        return true;
    }

    return false;
}

// Atualiza e move os inimigos
void UpdateEnemies(Enemy* enemies, int enemyCount, float dt) {
    for (int i = 0; i < enemyCount; i++) {
        // Faz o inimigo ir e voltar
        if (enemies[i].position.x <= enemies[i].minPosition.x || enemies[i].position.x >= enemies[i].maxPosition.x) {
            enemies[i].velocity.x = -enemies[i].velocity.x; // Reverte direção
        }
        enemies[i].position.x += enemies[i].velocity.x * dt;
        enemies[i].rect.x = enemies[i].position.x;
        enemies[i].rect.y = enemies[i].position.y;
    }
}

// Colisao entre jogador e inimigo
void HandlePlayerEnemyCollision(Player* player, Enemy* enemies, int enemyCount) {
    for (int i = 0; i < enemyCount; i++) {
        if (CheckCollisionRecs(player->rect, enemies[i].rect)) {
            player->health -= 10;
            if (player->health <= 0) {
                player->health = 0;
                printf("Player has died!\n");
            }
        }
    }
}


// Verifica colisão entre o projétil e inimigo
void CheckProjectileEnemyCollision(Projectile* projectiles, int* enemyCount, Enemy* enemies) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            for (int j = 0; j < *enemyCount; j++) {
                if (enemies[j].active && CheckCollisionRecs(projectiles[i].rect, enemies[j].rect)) {
                    // Colisão detectada reduz a vida do inimigo
                    enemies[j].health -= 10;  // Diminui a vida
                    if (enemies[j].health <= 0) {
                        enemies[j].health = 0;
                        enemies[j].active = false; // Desativa o inimigo se a vida chegar a 0
                    }
                    projectiles[i].active = false;  // Desativa projetil após colisão
                    break;
                }
            }
        }
    }
}


int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "INF-MAN");

    // Declare the map as a static array
    char map[MAX_HEIGHT][MAX_WIDTH];
    int rows, cols;

    // Load the map from the file
    LoadMap("map.txt", map, &rows, &cols);
    if (rows == 0 || cols == 0) {  // Verifica se o carregamento do mapa deu ruim
        CloseWindow();
        return 1;
    }

    // Inicialização do jogador
    Player player = {
        {0, 0},
        {0, 0},
        {0, 0, 32, 32},
        false,
        true,
        false,
        100,
        0
    };

    // Inicialização dos inimigos
    int enemyCount = 0;
    Enemy enemies[MAX_WIDTH];

    // Inicialização dos disparos
    float projectileWidth = 20.0f;  // Largura do disparo
    float projectileHeight = 10.0f; // Altura do disparo

    // Encontrar instancias da letra "M" no arquivo e criar inimigos pra cada uma delas
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'M') {
                enemies[enemyCount].position = (Vector2){x * BLOCK_SIZE, y * BLOCK_SIZE};
                enemies[enemyCount].velocity = (Vector2){50.0f, 0.0f}; // TODO: Criar variavel pra velocidade dos inimigos
                enemies[enemyCount].rect = (Rectangle){enemies[enemyCount].position.x, enemies[enemyCount].position.y, BLOCK_SIZE, BLOCK_SIZE};
                enemies[enemyCount].minPosition = enemies[enemyCount].position; // Posição minima é o ponto de spawn
                enemies[enemyCount].maxPosition = (Vector2){enemies[enemyCount].position.x + 200.0f, enemies[enemyCount].position.y}; // Posicao máxima é o destino
                enemies[enemyCount].health = 100;
                enemies[enemyCount].active = true; // Starting health
                enemyCount++;
            }
        }
    }

    // Variaveis de controle do mundo
    const float gravity = 500.0f;
    const float jumpForce = -300.0f;
    const float moveSpeed = 200.0f;
    const float projectileSpeed = 600.0f;

    // Configuração da câmera
    Camera2D camera = {0};
    camera.target = (Vector2){player.position.x + player.rect.width / 2, player.position.y + player.rect.height / 2};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
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

        // Gravidade
        player.velocity.y += gravity * dt;

        // Controles
        player.velocity.x = 0;
        if (IsKeyDown(KEY_RIGHT)) {
            player.velocity.x = moveSpeed;
            player.facingRight = true;
        }
        if (IsKeyDown(KEY_LEFT)) {
            player.velocity.x = -moveSpeed;
            player.facingRight = false;
        }
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

        // Atualiza projéteis
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active) {
                projectiles[i].rect.x += projectiles[i].speed.x * dt;
                projectiles[i].rect.y += projectiles[i].speed.y * dt;

                // Desativa projétil se sair da tela
                if (projectiles[i].rect.x < -600 || projectiles[i].rect.x > SCREEN_WIDTH) {
                    projectiles[i].active = false;
                }
            }
        }

        // Animação do jogador disparando
        player.isShooting = IsKeyDown(KEY_Z);

        // Atualiza posição do jogador
        player.position.x += player.velocity.x * dt;
        player.position.y += player.velocity.y * dt;
        player.rect.x = player.position.x;
        player.rect.y = player.position.y;

        // Colisao entre jogador e blocos
        player.isGrounded = false;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (map[y][x] == 'B') {
                    Rectangle block = {x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
                    Vector2 correction = {0, 0};
                    if (CheckCollisionWithBlock(player.rect, block, &correction)) {
                        if (correction.y != 0) { // Vertical correction
                            player.velocity.y = 0;
                            if (correction.y < 0) player.isGrounded = true;
                            player.position.y += correction.y;
                        } else if (correction.x != 0) { // Horizontal correction
                            player.velocity.x = 0;
                            player.position.x += correction.x;
                        }
                        player.rect.x = player.position.x;
                        player.rect.y = player.position.y;
                    }
                }
            }
        }

        // Respawna o jogador caso caia pra fora do mapa
        // TODO: RESPAWNAR PRA LOCALIZAÇÃO DOS 5 SEGUNDOS PRÉVIOS
        if (player.position.y > SCREEN_HEIGHT) {
            player.position.y = 300;
            player.position.x -= 100;
            player.velocity = (Vector2){0, 0};
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

        // Atualiza inimigos
        UpdateEnemies(enemies, enemyCount, dt);

        // Lida com projeteis batendo em inimigos
        CheckProjectileEnemyCollision(projectiles, &enemyCount, enemies);

        // Chamada pra lidar colisões entre jogador e inimigos
        HandlePlayerEnemyCollision(&player, enemies, enemyCount);

        // Renderização
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

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active) {
                DrawRectangleRec(projectiles[i].rect, YELLOW);
            }
        }

        // Renderiza mapa
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (map[y][x] == 'B') {
                    DrawRectangle(x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, DARKGRAY);
                }
            }
        }

        // Renderiza inimigos
        for (int i = 0; i < enemyCount; i++) {
            if (enemies[i].active) { // Only render active enemies
                DrawRectangle(enemies[i].position.x, enemies[i].position.y, BLOCK_SIZE, BLOCK_SIZE, RED);
                DrawText(TextFormat("HP: %d", enemies[i].health), enemies[i].position.x, enemies[i].position.y - 20, 10, DARKGRAY);
            }
        }

        EndMode2D();

        DrawText(TextFormat("Health: %d", player.health), 10, 40, 20, BLACK);
        DrawText(TextFormat("Points: %d", player.points), 10, 70, 20, BLACK);

        DrawText("INF-MAN", 10, 10, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}