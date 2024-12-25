#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_ENEMIES 1000
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

// Move os inimigos
void UpdateEnemies(Enemy* enemies, int enemyCount, float dt) {
    for (int i = 0; i < enemyCount; i++) {
        // Faz o inimigo ir e voltar
        if (enemies[i].position.x <= enemies[i].minPosition.x || enemies[i].position.x >= enemies[i].maxPosition.x) {
            enemies[i].velocity.x = -enemies[i].velocity.x; // Inverte direção
        }
        enemies[i].position.x += enemies[i].velocity.x * dt;
        enemies[i].rect.x = enemies[i].position.x;
        enemies[i].rect.y = enemies[i].position.y;
    }
}

// Colisao entre jogador e inimigo
void HandlePlayerEnemyCollision(Player* player, Enemy* enemies, int enemyCount, int* currentFrame, float dt) {
    for (int i = 0; i < enemyCount; i++) {
        if (CheckCollisionRecs(player->rect, enemies[i].rect) && enemies[i].active) {
            player->health -= 1;
            *currentFrame = 11;

            player->position.y += 20;
            player->position.x -= 200;
            player->velocity = (Vector2){0, 0};

            if (player->health <= 0) {
                player->health = 0;
                printf("Player is dead\n");
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

bool FindPlayerSpawnPoint(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, Player* player) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'P') { // letra P no mapa encontrada
                player->position = (Vector2){x * BLOCK_SIZE, y * BLOCK_SIZE};
                player->rect.x = player->position.x;
                player->rect.y = player->position.y;
                return true; // Existe spawnpoint
            }
        }
    }
    return false; // Nenhuma letra P foi encontrada, nao existe spawnpoint
}

// Encontra instancias da letra "M" no arquivo e criar inimigos pra cada uma delas
int InitializeEnemies(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, Enemy enemies[MAX_WIDTH], float blockSize) {
    int enemyCount = 0;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'M') {
                enemies[enemyCount].position = (Vector2){x * blockSize, y * blockSize};
                enemies[enemyCount].velocity = (Vector2){50.0f, 0.0f}; // Enemy velocity
                enemies[enemyCount].rect = (Rectangle){enemies[enemyCount].position.x, enemies[enemyCount].position.y, blockSize, blockSize};
                enemies[enemyCount].minPosition = enemies[enemyCount].position; // Minimum position is spawn point
                enemies[enemyCount].maxPosition = (Vector2){enemies[enemyCount].position.x + 200.0f, enemies[enemyCount].position.y}; // Maximum position
                enemies[enemyCount].health = 1; // Starting health
                enemies[enemyCount].active = true; // Mark enemy as active
                enemyCount++;
            }
        }
    }

    return enemyCount;
}

void InitializeProjectiles(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }
}

void UpdatePlayerMovement(Player *player, float moveSpeed, float jumpForce) {
    player->velocity.x = 0;

    if (IsKeyDown(KEY_RIGHT)) {
        player->velocity.x = moveSpeed;
        player->facingRight = true;
    }
    if (IsKeyDown(KEY_LEFT)) {
        player->velocity.x = -moveSpeed;
        player->facingRight = false;
    }
    if (IsKeyPressed(KEY_SPACE) && player->isGrounded) {
        player->velocity.y = jumpForce;
        player->isGrounded = false;
    }
}

void FireProjectile(Player *player, Projectile projectiles[MAX_PROJECTILES], float projectileWidth, float projectileHeight, float projectileSpeed, float dt) {
    static float shootTimer = 0.0f;

    if (IsKeyPressed(KEY_Z)) {
        player->isShooting = true;

        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (!projectiles[i].active) {
                projectiles[i].rect = (Rectangle){
                    player->position.x + (player->facingRight ? player->rect.width : -projectileWidth),
                    player->position.y + player->rect.height / 2 - projectileHeight / 2,
                    projectileWidth,
                    projectileHeight
                };
                projectiles[i].speed = (Vector2){player->facingRight ? projectileSpeed : -projectileSpeed, 0};
                projectiles[i].active = true;
                break;
            }
        }
    }

    if (player->isShooting) {
        shootTimer += dt;
        if (shootTimer >= 0.5f) {
            player->isShooting = false;
            shootTimer = 0.0f;
        }
    }
}

void CheckProjectileBlockCollision(Projectile *projectile, Rectangle block) {
    if (CheckCollisionRecs(projectile->rect, block)) {
        // Collision detected, stop the projectile or deactivate it
        projectile->active = false;  // Deactivate the projectile when it hits a block
    }
}

void UpdateProjectiles(Projectile projectiles[MAX_PROJECTILES], float dt, int screenWidth, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Move the projectile
            projectiles[i].rect.x += projectiles[i].speed.x * dt;
            projectiles[i].rect.y += projectiles[i].speed.y * dt;

            // Deactivate projectile if it goes off-screen
            if (projectiles[i].rect.x < -600 || projectiles[i].rect.x > screenWidth) {
                projectiles[i].active = false;
            }

            // Check for collisions with blocks
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (map[y][x] == 'B') {  // Check for block
                        Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};
                        CheckProjectileBlockCollision(&projectiles[i], block);
                    }
                }
            }
        }
    }
}

void HandlePlayerBlockCollisions(Player *player, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    player->isGrounded = false;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'B') {
                Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};
                Vector2 correction = {0, 0};
                if (CheckCollisionWithBlock(player->rect, block, &correction)) {
                    if (correction.y != 0) { // Vertical correction
                        player->velocity.y = 0;
                        if (correction.y < 0) player->isGrounded = true;
                        player->position.y += correction.y;
                    } else if (correction.x != 0) { // Horizontal correction
                        player->velocity.x = 0;
                        player->position.x += correction.x;
                    }
                    player->rect.x = player->position.x;
                    player->rect.y = player->position.y;
                }
            }
        }
    }
}

void UpdatePlayerAnimation(Player *player, float *frameTimer, float frameSpeed, int *currentFrame) {
    if (*frameTimer >= frameSpeed) {
        *frameTimer = 0.0f;

        if (!player->isGrounded) {
            *currentFrame = player->isShooting ? 10 : 5; // Jumping (shooting or not)
        } else if (player->velocity.x != 0) {
            // Moving
            if (player->isShooting) {
                *currentFrame = (*currentFrame < 6 || *currentFrame > 8) ? 7 : *currentFrame + 1;
            } else {
                *currentFrame = (*currentFrame < 1 || *currentFrame > 3) ? 2 : *currentFrame + 1; // Cycle: 1, 2, 3
            }
        } else {
            *currentFrame = player->isShooting ? 7 : 0; // Idle (shooting or not)
        }
    }
}

// Renderiza projeteis
void RenderProjectiles(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            DrawRectangleRec(projectiles[i].rect, YELLOW);
        }
    }
}

// Renderiza mapa
void RenderMap(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'B') {
                DrawRectangle(x * blockSize, y * blockSize, blockSize, blockSize, DARKGRAY);
            }
        }
    }
}

// Renderiza inimigos
void RenderEnemies(Enemy enemies[MAX_ENEMIES], int enemyCount, float blockSize) {
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].active) { // Render only active enemies
            DrawRectangle(enemies[i].position.x, enemies[i].position.y, blockSize, blockSize, RED);
            DrawText(TextFormat("HP: %d", enemies[i].health), enemies[i].position.x, enemies[i].position.y - 20, 10, DARKGRAY);
        }
    }
}

Player InitializePlayer() {
    Player player = {
        {0, 0},
        {0, 0},
        {0, 0, 32, 32},
        false,
        true,
        false,
        3,
        0
    };
    return player;
}

int InitializeEnemiesFromMap(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, Enemy enemies[MAX_WIDTH], float blockSize) {
    return InitializeEnemies(map, rows, cols, enemies, blockSize);
}

Camera2D InitializeCamera(Player *player) {
    Camera2D camera = {0};
    camera.target = (Vector2){player->position.x + player->rect.width / 2, player->position.y + player->rect.height / 2};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.zoom = 1.5f;
    return camera;
}

void InitializeProjectileSystem(Projectile projectiles[MAX_PROJECTILES]) {
    InitializeProjectiles(projectiles);
}

void InitializePlayerTextureAndAnimation(Texture2D *infmanTex, Rectangle *frameRec, int *frameWidth) {
    *infmanTex = LoadTexture("player-sheet.png");
    *frameWidth = infmanTex->width / 12;
    *frameRec = (Rectangle){0.0f, 0.0f, (float)(*frameWidth), (float)infmanTex->height};
}

void ApplyGravity(Player *player, float gravity, float dt) {
    player->velocity.y += gravity * dt;
}

void UpdatePlayer(Player *player, float moveSpeed, float jumpForce, float dt) {
    UpdatePlayerMovement(player, moveSpeed, jumpForce);
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;
    player->rect.x = player->position.x;
    player->rect.y = player->position.y;
}

void UpdatePlayerCamera(Camera2D *camera, Player *player) {
    camera->target = (Vector2){player->position.x + player->rect.width / 2, player->position.y + player->rect.height / 2};
}

void UpdatePlayerAnimationState(Player *player, float *frameTimer, float frameSpeed, unsigned *currentFrame, Rectangle *frameRec, int frameWidth) {
    *frameTimer += GetFrameTime();
    UpdatePlayerAnimation(player, frameTimer, frameSpeed, currentFrame);
    frameRec->x = frameWidth * (*currentFrame);
    frameRec->width = player->facingRight ? -frameWidth : frameWidth;
}

void HandleCollisions(Player* player, Enemy* enemies, int enemyCount, Projectile projectiles[MAX_PROJECTILES], char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize, int *currentFrame, float dt) {
    HandlePlayerBlockCollisions(player, map, rows, cols, blockSize);
    HandlePlayerEnemyCollision(player, enemies, enemyCount, currentFrame, dt);
    CheckProjectileEnemyCollision(projectiles, &enemyCount, enemies);
}


void HandleRespawn(Player *player, float screenHeight) {
    if (player->position.y > screenHeight) {
        player->position.y = 300;
        player->position.x -= 100;
        player->velocity = (Vector2){0, 0};
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "INF-MAN");

    int rows, cols;
    char map[MAX_HEIGHT][MAX_WIDTH];
    LoadMap("map.txt", map, &rows, &cols);
    if (rows == 0 || cols == 0) {
            CloseWindow();
            return 1;
    }

    Player player = InitializePlayer();
    if (!FindPlayerSpawnPoint(map, rows, cols, &player)) {
        CloseWindow();
        return 1;
    }

    Enemy enemies[MAX_WIDTH];
    int enemyCount = InitializeEnemiesFromMap(map, rows, cols, enemies, BLOCK_SIZE);

    Projectile projectiles[MAX_PROJECTILES];
    InitializeProjectileSystem(projectiles);

    Camera2D camera = InitializeCamera(&player);

    Texture2D infmanTex;
    Rectangle frameRec;
    int frameWidth;
    InitializePlayerTextureAndAnimation(&infmanTex, &frameRec, &frameWidth);

    float frameTimer = 0.0f;
    unsigned currentFrame = 0;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        ApplyGravity(&player, 500.0f, dt);

        UpdatePlayer(&player, 200.0f, -300.0f, dt);
        UpdatePlayerCamera(&camera, &player);
        UpdatePlayerAnimationState(&player, &frameTimer, 0.15f, &currentFrame, &frameRec, frameWidth);

        HandleCollisions(&player, enemies, enemyCount, projectiles, map, rows, cols, BLOCK_SIZE, &currentFrame, dt);
        HandleRespawn(&player, SCREEN_HEIGHT);

        UpdateEnemies(enemies, enemyCount, dt);

        FireProjectile(&player, projectiles, 20.0f, 10.0f, 600.0f, dt);
        UpdateProjectiles(projectiles, dt, SCREEN_WIDTH, map, rows, cols, BLOCK_SIZE);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        //Renderiza jogador
        DrawTexturePro(infmanTex, frameRec, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

        RenderMap(map, rows, cols, BLOCK_SIZE);
        RenderProjectiles(projectiles);
        RenderEnemies(enemies, enemyCount, BLOCK_SIZE);

        EndMode2D();

        //Painel de informações do jogador
        DrawText(TextFormat("Health: %d", player.health), 10, 40, 20, BLACK);
        DrawText(TextFormat("Points: %d", player.points), 10, 70, 20, BLACK);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
