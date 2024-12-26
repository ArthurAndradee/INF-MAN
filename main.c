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

typedef struct Coin {
    Vector2 position;   // Coordenadas (x, y)
    Rectangle rect;     // Retângulo para colisão
    bool active;        // Se a moeda está ativa ou não
    int points;         // Quantidade de pontos que a moeda dá
} Coin;

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

    if (*rows <= 10 || *cols <= 200) {
        printf("Mapa menor do que 200x10");
        exit(1);
    }
}

// Limpa a memória alocada pro mapa
void FreeMap(char** map, int rows) {
    for (int i = 0; i < rows; i++) {
        free(map[i]);
    }
    free(map);
}

// Aplica calculo da gravidade
void ApplyGravity(Player *player, float gravity, float dt) {
    player->velocity.y += gravity * dt;
}

void HandleRespawn(Player *player, float screenHeight) {
    if (player->position.y > screenHeight) {
        player->position.y = 300;
        player->position.x -= 100;
        player->velocity = (Vector2){0, 0};
    }
}

// Determina ou não se existe um spawnpoint para o jogador, caso sim, aplica as coordenadas encontradas no array x e y como ponto inicial do jogador
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

// Renderiza moedas
void RenderCoins(Coin coins[MAX_WIDTH], int coinCount) {
    for (int i = 0; i < coinCount; i++) {
        if (coins[i].active) {
            DrawRectangleRec(coins[i].rect, YELLOW);  // Draw coin as a rectangle (yellow color)
        }
    }
}

// Renderiza projeteis
void RenderProjectiles(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            DrawRectangleRec(projectiles[i].rect, BLUE);
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

// Inicializacao do jogador
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

// Inicializacao da camera
Camera2D InitializeCamera(Player *player) {
    Camera2D camera = {0};
    camera.target = (Vector2){player->position.x + player->rect.width / 2, player->position.y + player->rect.height / 2};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.zoom = 1.5f;
    return camera;
}

// Inicializacao das texturas do jogador
void InitializePlayerTextureAndAnimation(Texture2D *infmanTex, Rectangle *frameRec, int *frameWidth) {
    *infmanTex = LoadTexture("player-sheet.png");
    *frameWidth = infmanTex->width / 12;
    *frameRec = (Rectangle){0.0f, 0.0f, (float)(*frameWidth), (float)infmanTex->height};
}

// Inicializacao dos projeteis
void InitializeProjectiles(Projectile projectiles[MAX_PROJECTILES]) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }
}

// Encontra instancias da letra "M" no arquivo e criar inimigos pra cada uma delas
int InitializeEnemies(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, Enemy enemies[MAX_WIDTH], float blockSize, float enemySpeedX, float enemySpeedY, float offset) {
    int enemyCount = 0;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'M') {
                enemies[enemyCount].position = (Vector2){x * blockSize, y * blockSize};
                enemies[enemyCount].velocity = (Vector2){enemySpeedX, enemySpeedY}; // Velocidade do inimigo
                enemies[enemyCount].rect = (Rectangle){enemies[enemyCount].position.x, enemies[enemyCount].position.y, blockSize, blockSize};
                enemies[enemyCount].minPosition = enemies[enemyCount].position; // Posicao minimia é o spawnpoint
                enemies[enemyCount].maxPosition = (Vector2){enemies[enemyCount].position.x + offset, enemies[enemyCount].position.y}; // Posicao maxima
                enemies[enemyCount].health = 1; // Vida que começa
                enemies[enemyCount].active = true; // Inimigo é ativado
                enemyCount++;
            }
        }
    }

    return enemyCount;
}

int InitializeCoins(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, Coin coins[MAX_WIDTH], float blockSize) {
    int coinCount = 0;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'C') { // Moeda no mapa
                coins[coinCount].position = (Vector2){x * blockSize, y * blockSize};
                coins[coinCount].rect = (Rectangle){coins[coinCount].position.x, coins[coinCount].position.y, blockSize, blockSize};
                coins[coinCount].active = true; // Marca a moeda como ativa
                coins[coinCount].points = 10; // A moeda dá 10
                coinCount++;
            }
        }
    }

    return coinCount;
}

void CheckPressedKey(Player *player, float moveSpeed, float jumpForce) {
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

void CreateProjectile(Player *player, Projectile projectiles[MAX_PROJECTILES], float projectileWidth, float projectileHeight, float projectileSpeed, float dt) {
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


void MoveCamera(Camera2D *camera, Player *player) {
    camera->target = (Vector2){player->position.x + player->rect.width / 2, player->position.y + player->rect.height / 2};
}

void MovePlayer(Player *player, float moveSpeed, float jumpForce, float dt) {
    CheckPressedKey(player, moveSpeed, jumpForce);
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;
    player->rect.x = player->position.x;
    player->rect.y = player->position.y;
}

// Move os inimigos
void MoveEnemies(Enemy* enemies, int enemyCount, float dt) {
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

// Desativa projeteis quando batem em um bloco
void CheckProjectileBlockCollision(Projectile *projectile, Rectangle block) {
    if (CheckCollisionRecs(projectile->rect, block)) {
        projectile->active = false;
    }
}

// Move projeteis quanndo disparados
void MoveProjectiles(Projectile projectiles[MAX_PROJECTILES], float dt, int screenWidth, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Movimento do projetil
            projectiles[i].rect.x += projectiles[i].speed.x * dt;
            projectiles[i].rect.y += projectiles[i].speed.y * dt;

            // Desativa se vai pra fora da tela
            if (projectiles[i].rect.x < -600 || projectiles[i].rect.x > screenWidth) {
                projectiles[i].active = false;
            }

            // Verifica colisao com blocos
            for (int y = 0; y < rows; y++) {
                for (int x = 0; x < cols; x++) {
                    if (map[y][x] == 'B') {  // Verifica bloco a bloco ate achar um
                        Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};
                        CheckProjectileBlockCollision(&projectiles[i], block);
                    }
                }
            }
        }
    }
}


// Colisao entre jogador e moeda
void CheckPlayerCoinCollision(Player* player, Coin* coins, int* coinCount) {
    for (int i = 0; i < *coinCount; i++) {
        if (coins[i].active && CheckCollisionRecs(player->rect, coins[i].rect)) {
            player->points += coins[i].points;
            coins[i].active = false;
            printf("Points: %d\n", player->points);
        }
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
void CheckProjectileEnemyCollision(Projectile* projectiles, int* enemyCount, Enemy* enemies, Player* player) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            for (int j = 0; j < *enemyCount; j++) {
                if (enemies[j].active && CheckCollisionRecs(projectiles[i].rect, enemies[j].rect)) {
                    // Colisão detectada reduz a vida do inimigo
                    enemies[j].health -= 1;  // Diminui a vida
                    player->points += 10;
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

// Percorre o array de caracteres (mapa), cria um retangulo e usa CheckCollisionWithBlock() para determinar se o jogador está encostado. Caso sim e o bloco seja M, usa a diferença entre as duas posições, a variavel correction, é usada para manter o jogador na sua posição. Caso sim e o bloco seja O,
void HandlePlayerBlockCollisions(Player *player, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    player->isGrounded = false;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'B') {  //  Verifica bloco a bloco ate achar um
                Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};
                Vector2 correction = {0, 0};
                if (CheckCollisionWithBlock(player->rect, block, &correction)) {
                    if (correction.y != 0) { // Correcao vertical
                        player->velocity.y = 0;
                        player->position.y += correction.y;

                        if (correction.y < 0) {
                            player->isGrounded = true;
                        }
                    } else if (correction.x != 0) { // Correcao horizontal
                        player->velocity.x = 0;
                        player->position.x += correction.x;
                    }
                    player->rect.x = player->position.x;
                    player->rect.y = player->position.y;
                }
            }

            if (map[y][x] == 'O') {
                Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};
                Vector2 correction = {0, 0};
                if (CheckCollisionWithBlock(player->rect, block, &correction)) {
                    player->health -= 1;

                    if (player->health < 0) {
                        player->health = 0;
                    }

                    //TODO: SEPARAR "O" EM FUNÇÃO EXTERNA E CRIAR FUNÇÃO PARA RENDERIZAR OBSTÁCULOS
                    // if (correction.y != 0) { // Correcao vertical
                    //     player->velocity.y = 0;
                    //     player->position.y += correction.y;

                    //     if (correction.y < 0) {
                    //         player->isGrounded = true;
                    //     }
                    // } else if (correction.x != 0) { // Correcao horizontal
                    //     player->velocity.x = 0;
                    //     player->position.x += correction.x;
                    // }
                    // Possible useable code up

                    player->position.x -= correction.x;
                    player->position.y -= correction.y;
                    player->rect.x = player->position.x;
                    player->rect.y = player->position.y;
                    printf("Player health: %d\n", player->health);
                }
            }
        }
    }
}

// Chama todas as funções de colisão 1 vez só
void HandleCollisions(Player* player, Enemy* enemies, int enemyCount, Projectile projectiles[MAX_PROJECTILES], char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize, int *currentFrame, float dt, Coin* coins, int* coinCount) {
    HandlePlayerBlockCollisions(player, map, rows, cols, blockSize);
    HandlePlayerEnemyCollision(player, enemies, enemyCount, currentFrame, dt);
    CheckProjectileEnemyCollision(projectiles, &enemyCount, enemies, player);
    CheckPlayerCoinCollision(player, coins, coinCount);
}

// Atualiza textura que apresenta o jogador conforme movimento
void UpdatePlayerAnimation(Player *player, float *frameTimer, float frameSpeed, int *currentFrame) {
    if (*frameTimer >= frameSpeed) { // Condicional para nao entrar em loop infinito
        *frameTimer = 0.0f;

        if (!player->isGrounded) { // Jogador está no ar
            *currentFrame = player->isShooting ? 10 : 5; // Animação de pulo
        } else if (player->velocity.x != 0) { // Verifica se jogador está se mexendo horizontalmente (eixo x)
            if (player->isShooting) { // Verifica se jogador está atirando, se sim:
                *currentFrame = (*currentFrame < 6 || *currentFrame > 8) ? 7 : *currentFrame + 1; // Varia entre texturas 6, 7 e 9
            } else { // Se não está atirando:
                *currentFrame = (*currentFrame < 1 || *currentFrame > 3) ? 2 : *currentFrame + 1; // Varia entre texturas 1, 2 e 3
            }
        } else { // Jogador nao está fazendo nenhum movimento
            *currentFrame = player->isShooting ? 7 : 0; // Animação de estar parado ou parado e atirando
        }
    }
}

// Atualiza estado do jogador frame a frame e chama a função cada vez para trocar textura de acordo com estado
void UpdatePlayerAnimationState(Player *player, float *frameTimer, float frameSpeed, unsigned *currentFrame, Rectangle *frameRec, int frameWidth) {
    *frameTimer += GetFrameTime(); // Pega tempo desde o último frame
    UpdatePlayerAnimation(player, frameTimer, frameSpeed, currentFrame); // Aplica textura ao jogador
    frameRec->x = frameWidth * (*currentFrame);
    frameRec->width = player->facingRight ? -frameWidth : frameWidth;
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "INF-MAN");

    // Variaveis de controle de mundo
    float gravity = 500.0;
    float playerSpeed = 200.0;
    float jumpForce = -300.0;

    float enemySpeedX = 550.0;
    float enemySpeedY = 50.0;
    float enemyOffset = 200.0f;

    float projectileWidth = 20.0;
    float projectileHeight = 10.0;
    float projectileSpeed = 400.0;

    float framseSpeed = 0.15f;

    // Carregamento do mapa
    int rows, cols;
    char map[MAX_HEIGHT][MAX_WIDTH];
    LoadMap("map.txt", map, &rows, &cols);
    if (rows == 0 || cols == 0) {
            CloseWindow();
            return 1;
    }

    // Inicialização do jogador. Caso não haja spawnpoint no arquivo lido, encerra programa
    Player player = InitializePlayer();
    if (!FindPlayerSpawnPoint(map, rows, cols, &player)) {
        CloseWindow();
        return 1;
    }

    // Inicializações:
    Texture2D infmanTex;
    Rectangle frameRec;
    int frameWidth;
    InitializePlayerTextureAndAnimation(&infmanTex, &frameRec, &frameWidth);

    Camera2D camera = InitializeCamera(&player);
    float frameTimer = 0.0f;
    unsigned currentFrame = 0;

    Coin coins[MAX_WIDTH];
    int coinCount = InitializeCoins(map, rows, cols, coins, BLOCK_SIZE);

    Enemy enemies[MAX_WIDTH];
    int enemyCount = InitializeEnemies(map, rows, cols, enemies, BLOCK_SIZE, enemySpeedX, enemySpeedY, enemyOffset);

    Projectile projectiles[MAX_PROJECTILES];
    InitializeProjectiles(projectiles);

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        ApplyGravity(&player, gravity, dt);

        UpdatePlayerAnimationState(&player, &frameTimer, framseSpeed, &currentFrame, &frameRec, frameWidth);
        HandleRespawn(&player, SCREEN_HEIGHT);

        MovePlayer(&player, playerSpeed, jumpForce, dt);
        MoveCamera(&camera, &player);
        MoveEnemies(enemies, enemyCount, dt);
        MoveProjectiles(projectiles, dt, SCREEN_WIDTH, map, rows, cols, BLOCK_SIZE);

        CreateProjectile(&player, projectiles, projectileWidth, projectileHeight, projectileSpeed, dt);
        HandleCollisions(&player, enemies, enemyCount, projectiles, map, rows, cols, BLOCK_SIZE, &currentFrame, dt, coins, &coinCount);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        //Renderiza jogador
        DrawTexturePro(infmanTex, frameRec, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

        //Renderização
        RenderCoins(coins, coinCount);
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
