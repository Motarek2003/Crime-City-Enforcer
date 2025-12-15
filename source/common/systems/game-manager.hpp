#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <json/json.hpp>
#include "../ecs/world.hpp"
#include "../components/character.hpp"
#include "./physics-system.hpp"
#include "./enemy-controller.hpp"
#include "../application.hpp"
#include <imgui.h>

namespace our {

    // Game phases
    enum class GamePhase {
        EXPLORATION,        // Player exploring, no combat
        WAVE_COMBAT,        // Fighting waves of enemies
        BOSS_WAITING,       // Boss is present but not yet triggered
        BOSS_FIGHT,         // Fighting the boss
        VICTORY,            // Player won
        DEFEAT              // Player lost
    };

    // Game Manager - handles quest progression, waves, and win/lose conditions
    class GameManager {
    private:
        Application* app = nullptr;
        GamePhase currentPhase = GamePhase::BOSS_WAITING;
        
        // Wave system
        int currentWave = 0;
        int totalWaves = 3;
        int enemiesRemainingInWave = 0;
        int enemiesPerWave = 3;
        bool waveSpawned = false;
        
        // Boss trigger
        bool bossTriggered = false;
        bool bossDefeated = false;
        
        // Victory/Defeat timing
        float endGameTimer = 0.0f;
        float endGameDelay = 3.0f;  // Show message for 3 seconds before state change
        bool endGameTriggered = false;
        
        // Spawn positions for wave enemies
        std::vector<glm::vec3> spawnPositions = {
            {-8, 2, -8},
            {8, 2, -8},
            {-8, 2, 8},
            {8, 2, 8},
            {0, 2, -12},
            {0, 2, 12}
        };

    public:
        EnemyControllerSystem* bossController = nullptr;
        
        void enter(Application* app, EnemyControllerSystem* bossCtrl = nullptr) {
            this->app = app;
            this->bossController = bossCtrl;
            reset();
        }
        
        void reset() {
            currentPhase = GamePhase::BOSS_WAITING;
            currentWave = 0;
            enemiesRemainingInWave = 0;
            waveSpawned = false;
            bossTriggered = false;
            bossDefeated = false;
            endGameTimer = 0.0f;
            endGameTriggered = false;
        }
        
        void update(World* world, float deltaTime, Application* appPtr) {
            // Update app reference if provided
            if (appPtr) this->app = appPtr;
            update(world, deltaTime);
        }
        
        void update(World* world, float deltaTime) {
            // Count alive enemies and check player status
            int aliveEnemies = 0;
            bool playerAlive = false;
            bool bossAlive = false;
            Entity* bossEntity = nullptr;
            Entity* playerEntity = nullptr;
            
            for (auto entity : world->getEntities()) {
                auto character = entity->getComponent<CharacterComponent>();
                if (!character) continue;
                
                if (entity->name == "taskmaster") {
                    bossEntity = entity;
                    bossAlive = character->getAlive();
                } else if (entity->name == "enemy" && character->getAlive()) {
                    aliveEnemies++;
                } else if (entity->name == "deadpool") {
                    playerEntity = entity;
                    playerAlive = character->getAlive();
                }
            }
            
            // Check for player defeat
            if (!playerAlive && currentPhase != GamePhase::DEFEAT) {
                currentPhase = GamePhase::DEFEAT;
                endGameTriggered = true;
                endGameTimer = 0.0f;
                std::cout << "=== PLAYER DEFEATED ===" << std::endl;
            }
            
            // Handle end game timer
            if (endGameTriggered) {
                endGameTimer += deltaTime;
                std::cout << "End game timer: " << endGameTimer << " / " << endGameDelay << std::endl;
                if (endGameTimer >= endGameDelay) {
                    if (currentPhase == GamePhase::VICTORY) {
                        std::cout << "=== CHANGING TO WIN STATE ===" << std::endl;
                        if (app) {
                            app->changeState("win");
                        } else {
                            std::cout << "ERROR: app pointer is null!" << std::endl;
                        }
                    } else if (currentPhase == GamePhase::DEFEAT) {
                        std::cout << "=== CHANGING TO DEAD STATE ===" << std::endl;
                        if (app) {
                            app->changeState("dead");
                        }
                    }
                }
                return;
            }
            
            // State machine
            switch (currentPhase) {
                case GamePhase::BOSS_WAITING:
                    // Boss waits in idle until triggered
                    // For now, trigger when player gets close
                    if (playerEntity && bossEntity) {
                        glm::vec3 playerPos = playerEntity->localTransform.position;
                        glm::vec3 bossPos = bossEntity->localTransform.position;
                        float distance = glm::length(playerPos - bossPos);
                        
                        // Trigger boss fight when player gets within 15 units
                        if (distance < 15.0f && !bossTriggered) {
                            bossTriggered = true;
                            currentPhase = GamePhase::BOSS_FIGHT;
                            // Activate the boss controller
                            if (bossController) {
                                bossController->activateBoss();
                            }
                            std::cout << "=== BOSS FIGHT TRIGGERED ===" << std::endl;
                        }
                    }
                    break;
                    
                case GamePhase::WAVE_COMBAT:
                // SCRAPPED
                    // Check if wave is complete
                    if (aliveEnemies == 0) {
                        currentWave++;
                        waveSpawned = false;
                        
                        if (currentWave > totalWaves) {
                            // All waves complete, boss fight begins
                            currentPhase = GamePhase::BOSS_FIGHT;
                            bossTriggered = true;
                            // Activate the boss controller
                            if (bossController) {
                                bossController->activateBoss();
                            }
                            std::cout << "=== ALL WAVES CLEARED! BOSS FIGHT! ===" << std::endl;
                        } else {
                            std::cout << "Wave " << currentWave << " complete! Preparing next wave..." << std::endl;
                        }
                    }
                    break;
                    
                case GamePhase::BOSS_FIGHT:
                    // Check if boss is defeated
                    if (!bossAlive && !bossDefeated) {
                        bossDefeated = true;
                        currentPhase = GamePhase::VICTORY;
                        endGameTriggered = true;
                        endGameTimer = 0.0f;
                        std::cout << "=== VICTORY! BOSS DEFEATED! ===" << std::endl;
                        std::cout << "End game timer started, waiting " << endGameDelay << " seconds..." << std::endl;
                    }
                    break;
                    
                case GamePhase::VICTORY:
                case GamePhase::DEFEAT:
                    // Handled above
                    break;
                    
                default:
                    break;
            }
        }
        
        // Getters for UI
        GamePhase getPhase() const { return currentPhase; }
        int getCurrentWave() const { return currentWave; }
        int getTotalWaves() const { return totalWaves; }
        bool isBossTriggered() const { return bossTriggered; }
        bool isBossDefeated() const { return bossDefeated; }
        bool isVictory() const { return currentPhase == GamePhase::VICTORY; }
        bool isDefeat() const { return currentPhase == GamePhase::DEFEAT; }
        float getEndGameTimer() const { return endGameTimer; }
        
        // Get status message for UI
        std::string getStatusMessage() const {
            switch (currentPhase) {
                case GamePhase::EXPLORATION:
                    return "Explore the area";
                case GamePhase::WAVE_COMBAT:
                    return "Wave " + std::to_string(currentWave) + "/" + std::to_string(totalWaves);
                case GamePhase::BOSS_WAITING:
                    return "Find Taskmaster";
                case GamePhase::BOSS_FIGHT:
                    return "BOSS FIGHT: Taskmaster";
                case GamePhase::VICTORY:
                    return "MISSION COMPLETE!";
                case GamePhase::DEFEAT:
                    return "MISSION FAILED";
                default:
                    return "";
            }
        }
        
        void exit() {}
        
        // Draw the game status UI
        void drawUI() {
            // Position at top center
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 10), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_Always);
            
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                                      ImGuiWindowFlags_NoResize | 
                                      ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_AlwaysAutoResize;
            
            ImGui::Begin("GameStatus", nullptr, flags);
            
            // Status message with color based on phase
            ImVec4 color;
            switch (currentPhase) {
                case GamePhase::VICTORY:
                    color = ImVec4(1.0f, 0.85f, 0.0f, 1.0f); // Gold
                    break;
                case GamePhase::DEFEAT:
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
                    break;
                case GamePhase::BOSS_FIGHT:
                    color = ImVec4(1.0f, 0.4f, 0.0f, 1.0f); // Orange
                    break;
                default:
                    color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
                    break;
            }
            
            std::string statusMsg = getStatusMessage();
            float textWidth = ImGui::CalcTextSize(statusMsg.c_str()).x;
            ImGui::SetCursorPosX((300 - textWidth) * 0.5f);
            ImGui::TextColored(color, "%s", statusMsg.c_str());
            
            // Show countdown for end game
            if (endGameTriggered) {
                float remaining = endGameDelay - endGameTimer;
                if (remaining > 0) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f", remaining);
                    float timerWidth = ImGui::CalcTextSize(buf).x;
                    ImGui::SetCursorPosX((300 - timerWidth) * 0.5f);
                    ImGui::Text("%s", buf);
                }
            }
            
            ImGui::End();
        }
    };
}
