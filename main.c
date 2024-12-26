#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_ENEMIES 1000
#define MAX_PROJECTILES 1000
#define BLOCK_SIZE 16
#define MAX_WIDTH 1000
#define MAX_HEIGHT 100
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600

#define MAX_HISTORY_SIZE 180

typedef struct Player {
    Vector2 position;   // Coordenadas (x, y)
    Vector2 velocity;   // Velocidade (x, y)
    Rectangle rect;     // Retangulo pra colisao
    bool isGrounded;    // Determina se está no chao
    bool facingRight;   // Determina direcao que está encarando
    bool isShooting;    // Determina se está atirando ou nao
    int health;         // Pontos de vida
    int points;         // Pontos para o placar
} Player;

typedef struct Enemy {
    Vector2 position;   // coordenadas (x, y)
    Vector2 velocity;   // velocidade (x, y)
    Rectangle rect;     // Retangulo pra colisao
    Vector2 minPosition; // posicao minima (x, y)
    Vector2 maxPosition; // posicao maxima (x, y)
    int health;         // pontos de vida
    bool active;        // determina se o inimigo está ativo
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

typedef struct {
    Vector2 positionHistory[MAX_HISTORY_SIZE];
    int currentIndex;
} PlayerHistory;

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

// Aplica calculo da gravidade
void ApplyGravity(Player *player, float gravity, float dt) {
    player->velocity.y += gravity * dt;
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
            DrawRectangleRec(projectiles[i].rect, YELLOW);
        }
    }
}

// Renderiza inimigos
void RenderEnemies(Enemy enemies[MAX_ENEMIES], int enemyCount, float blockSize, Texture2D enemyTexture) {
    for (int i = 0; i < enemyCount; i++) {
        if (enemies[i].active) {
            Rectangle destRect = {enemies[i].position.x, enemies[i].position.y, 21, 16};
            DrawTexturePro(enemyTexture,
                           (Rectangle){0, 0, enemyTexture.width, enemyTexture.height},
                           destRect,
                           (Vector2){0, 0},
                           0.0f,
                           WHITE);
            DrawText(TextFormat("HP: %d", enemies[i].health), enemies[i].position.x, enemies[i].position.y - 20, 10, DARKGRAY);
        }
    }
}


// Renderiza mapa
void RenderMap(char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize, Texture2D blockTexture, Texture2D obstacleTexture, Texture2D gateTexture) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (map[y][x] == 'B') {
                Rectangle destRect = {x * blockSize, y * blockSize, blockSize, blockSize};
                DrawTexturePro(blockTexture, (Rectangle){0, 0, blockTexture.width, blockTexture.height}, destRect, (Vector2){0, 0}, 0.0f, WHITE);
            }
            else if (map[y][x] == 'O') {
                Rectangle destRect = {x * blockSize, y * blockSize, blockSize, blockSize};
                DrawTexturePro(obstacleTexture, (Rectangle){0, 0, obstacleTexture.width, obstacleTexture.height}, destRect, (Vector2){0, 0}, 0.0f, WHITE);
            } else if (map[y][x] == 'G') {
                Rectangle destRect = {x * blockSize, y * blockSize - 16, blockSize * 2, blockSize * 2};
                DrawTexturePro(gateTexture, (Rectangle){0, 0, gateTexture.width, gateTexture.height}, destRect, (Vector2){0, 0}, 0.0f, WHITE);
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

void InitPlayerHistory(PlayerHistory *history) {
    history->currentIndex = 0;
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        history->positionHistory[i] = (Vector2){0, 0};
    }
}

// Inicializacao da camera
Camera2D InitializeCamera(Player *player) {
    Camera2D camera = {0};
    camera.target = (Vector2){player->position.x + player->rect.width / 2, player->position.y + player->rect.height / 2};
    camera.offset = (Vector2){SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    camera.zoom = 2.0f;
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

// Inicializa as moedas no mapa
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

// Recebe posição atual do jogador e atualiza ela, substituindo a antiga
void UpdatePlayerHistory(PlayerHistory *history, Vector2 currentPosition) {
    history->positionHistory[history->currentIndex] = currentPosition;
    history->currentIndex = (history->currentIndex + 1) % MAX_HISTORY_SIZE;
}

// Retorna posição de 3 segundos atrás em um formato vetor2
Vector2 GetPositionFrom3SecondsAgo(PlayerHistory *history) {
    int targetIndex = history->currentIndex - 180;  // 3 segundos = 180 frames
    if (targetIndex < 0) {
        targetIndex += MAX_HISTORY_SIZE;
    }
    return history->positionHistory[targetIndex];
}

void HandleRespawn(Player *player, float screenHeight, PlayerHistory *history) {
    if (player->position.y > screenHeight) {
        player->position = GetPositionFrom3SecondsAgo(history);
        player->velocity = (Vector2){0, 0};
        player->health -= 1;
    }
}

// Aplica movimento para o jogador conforme a tecla pressionada
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

// Cria projetil com coordenadas baseadas na posição atual do jogador e aplica estado do jogador estar atirando durante 0.5 segundos
void CreateProjectile(Player *player, Projectile projectiles[MAX_PROJECTILES], float projectileWidth, float projectileHeight, float projectileSpeed, float dt) {
    static float shootTimer = 0.0f;
    float animationDuration = 0.5;

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
        if (shootTimer >= animationDuration) {
            player->isShooting = false;
            shootTimer = 0.0f;
        }
    }
}

// Move camera de acordo com posição do jogador
void MoveCamera(Camera2D *camera, Player *player) {
    camera->target = (Vector2){player->position.x + player->rect.width / 2, SCREEN_HEIGHT / 2 - 110};
}

// Move jogador com base na velocidade multiplicada pelo frame atual
void MovePlayer(Player *player, float moveSpeed, float jumpForce, float dt) {
    CheckPressedKey(player, moveSpeed, jumpForce);
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;
    player->rect.x = player->position.x;
    player->rect.y = player->position.y;
}

// Move os inimigos com base na velocidade multiplicada pelo frame atual
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
void MoveProjectiles(Projectile projectiles[MAX_PROJECTILES], float dt, Player* player, int screenWidth, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize) {
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (projectiles[i].active) {
            // Movimento do projetil
            projectiles[i].rect.x += projectiles[i].speed.x * dt;
            projectiles[i].rect.y += projectiles[i].speed.y * dt;

            // Desativa se vai pra fora da tela
             if (projectiles[i].rect.x < player->position.x - screenWidth ||
                 projectiles[i].rect.x > player->position.x + screenWidth ||
                 projectiles[i].rect.y < player->position.y - screenWidth ||
                 projectiles[i].rect.y > player->position.y + screenWidth)
            {
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
void HandlePlayerEnemyCollision(Player* player, Enemy* enemies, int enemyCount, int* currentFrame, float dt, PlayerHistory* history) {
    for (int i = 0; i < enemyCount; i++) {
        if (CheckCollisionRecs(player->rect, enemies[i].rect) && enemies[i].active) {
            player->health -= 1;
            *currentFrame = 11;

            player->position = GetPositionFrom3SecondsAgo(history);
            player->velocity = (Vector2){0, 0};

            player->rect.x = player->position.x;
            player->rect.y = player->position.y;

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
                    player->points += 100;
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

// Caso haja colisão entre jogador e o bloco, e bloco seja M, usa a diferença entre as duas posições, a variavel correction, é usada para manter o jogador na sua posição.
void HandleBlockCollision(Player *player, Rectangle block) {
    Vector2 correction = {0, 0};
    if (CheckCollisionWithBlock(player->rect, block, &correction)) {
        if (correction.y != 0) { // Correção vertical
            player->velocity.y = 0;
            player->position.y += correction.y;

            if (correction.y < 0) {
                player->isGrounded = true;
            }
        } else if (correction.x != 0) { // Correção horizontal
            player->velocity.x = 0;
            player->position.x += correction.x;
        }

        player->rect.x = player->position.x;
        player->rect.y = player->position.y;
    }
}
// Caso haja colisão entre jogador e o bloco, e bloco seja O, empurra o jogador para trás de subtrai 1 de sua vida.
void HandleObstacleCollision(Player *player, Rectangle block, PlayerHistory *history) {
    Vector2 correction = {0, 0};
    if (CheckCollisionWithBlock(player->rect, block, &correction)) {
        player->health -= 1;
        if (player->health < 0) {
            player->health = 0;
        }

        // Reset player to position from 3 seconds ago
        player->position = GetPositionFrom3SecondsAgo(history);
        player->velocity = (Vector2){0, 0};

        // Update player's rect position
        player->rect.x = player->position.x;
        player->rect.y = player->position.y;

        printf("Player health: %d\n", player->health);
    }
}

// Caso haja colisão entre o jogador e o portão, fecha o jogo logando a pontuação total to jogador
void HandleGateCollision(Player *player, Rectangle block) {
    Vector2 correction = {0, 0};
    if (CheckCollisionWithBlock(player->rect, block, &correction)) {
        printf("Player's total points: %d\n", player->points);
        exit(1);
    }
}

// Percorre o array de caracteres (mapa), cria um retangulo e usa CheckCollisionWithBlock() para determinar se o jogador está colidindo com algum bloco.
void HandlePlayerBlockCollisions(Player *player, char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize, PlayerHistory *history) {
    player->isGrounded = false;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Rectangle block = {x * blockSize, y * blockSize, blockSize, blockSize};

            if (map[y][x] == 'B') {
                HandleBlockCollision(player, block);
            } else if (map[y][x] == 'O') {
                HandleObstacleCollision(player, block, history);
            } else if (map[y][x] == 'G') {
                HandleGateCollision(player, block);
            }
        }
    }
}

// Chama todas as funções de colisão 1 vez só
void HandleCollisions(Player* player, Enemy* enemies, int enemyCount, Projectile projectiles[MAX_PROJECTILES], char map[MAX_HEIGHT][MAX_WIDTH], int rows, int cols, float blockSize, int *currentFrame, float dt, Coin* coins, int* coinCount, PlayerHistory *history) {
    HandlePlayerBlockCollisions(player, map, rows, cols, blockSize, history);
    HandlePlayerEnemyCollision(player, enemies, enemyCount, currentFrame, dt, history);
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

// Verifica vida do jogador e, se 0 ou abaixo, encerra o jogo
void CheckPlayerHealth(Player *player) {
    if (player->health <= 0) {
        exit(1);
    }
}

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "INF-MAN");

    // Variaveis de controle de mundo
    float gravity = 500.0;
    float playerSpeed = 200.0;
    float jumpForce = -300.0;

    float enemySpeedX = 150.0;
    float enemySpeedY = 50.0;
    float enemyOffset = 200.0f;

    float projectileWidth = 20.0;
    float projectileHeight = 10.0;
    float projectileSpeed = 400.0;

    float frameSpeed = 0.15f;

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
    Texture2D background = LoadTexture("background.png");
    Texture2D blockTexture = LoadTexture("tile1.png");
    Texture2D obstacleTexture = LoadTexture("spike.png");
    Texture2D gateTexture = LoadTexture("gate.png");
    Texture2D enemiesTexture = LoadTexture("enemies.png");
    Texture2D heartTexture = LoadTexture("heart.png");

    Texture2D infmanTex;
    Rectangle frameRec;
    int frameWidth;
    InitializePlayerTextureAndAnimation(&infmanTex, &frameRec, &frameWidth);

    Camera2D camera = InitializeCamera(&player);
    float frameTimer = 0.0f;
    unsigned currentFrame = 0;

    PlayerHistory history;
    InitPlayerHistory(&history);

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

        CheckPlayerHealth(&player);
        UpdatePlayerHistory(&history, player.position);
        UpdatePlayerAnimationState(&player, &frameTimer, frameSpeed, &currentFrame, &frameRec, frameWidth);
        HandleRespawn(&player, SCREEN_HEIGHT, &history);

        MovePlayer(&player, playerSpeed, jumpForce, dt);
        MoveCamera(&camera, &player);
        MoveEnemies(enemies, enemyCount, dt);
        MoveProjectiles(projectiles, dt, &player, SCREEN_WIDTH, map, rows, cols, BLOCK_SIZE);

        CreateProjectile(&player, projectiles, projectileWidth, projectileHeight, projectileSpeed, dt);
        HandleCollisions(&player, enemies, enemyCount, projectiles, map, rows, cols, BLOCK_SIZE, &currentFrame, dt, coins, &coinCount, &history);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        for (int i = 0; i < cols; i++) {
            for (int j = 0; j < rows; j++) {
                DrawTexture(background, i * background.width, 0, WHITE);
            }
        }

        //Renderiza jogador
        DrawTexturePro(infmanTex, frameRec, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

        //Renderização
        RenderCoins(coins, coinCount);
        RenderMap(map, rows, cols, BLOCK_SIZE, blockTexture, obstacleTexture, gateTexture);
        RenderProjectiles(projectiles);
        RenderEnemies(enemies, enemyCount, BLOCK_SIZE, enemiesTexture);

        EndMode2D();

        int heartX = 85;
        int heartY = 37;
        float heartWidth = 30.0f;
        float heartHeight = 30.0f;
        for (int i = 0; i < player.health; i++) {
            Rectangle destRect = { heartX + i * (heartWidth + 5), heartY, heartWidth, heartHeight };
            DrawTexturePro(heartTexture, (Rectangle){0, 0, heartTexture.width, heartTexture.height}, destRect, (Vector2){0, 0}, 0.0f, WHITE);
        }

        int textX = 10;
        int textY = 40;
        int textHeight = 20;
        int textWidth = MeasureText(TextFormat("Health:"), textHeight);
        DrawRectangle(textX - 5, textY - 5, textWidth + 10, textHeight + 10, BLACK);
        DrawText(TextFormat("Health:"), textX, textY, 20, WHITE);

        int pointsX = 10;
        int pointsY = 70;
        int pointsHeight = 20;
        int pointsWidth = MeasureText(TextFormat("Points: %d", player.points), textHeight);
        DrawRectangle(pointsX - 5, pointsY - 5, pointsWidth + 10, pointsHeight + 10, BLACK);
        DrawText(TextFormat("Points: %d", player.points), 10, 70, textHeight, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
