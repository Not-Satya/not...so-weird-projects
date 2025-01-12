#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <algorithm>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

class FlappyBirb {
private:
    static const int SCREEN_HEIGHT = 20;
    static const int SCREEN_WIDTH = 40;
    
    // Refined physics parameters
    double birby;
    double velocity;
    const double GRAVITY = 0.3;      // Soft gravity
    const double JUMP_STRENGTH = 1.8; // Controlled jump
    const double MAX_FALL_SPEED = 3.0; // Manageable fall speed
    
    int birbx;
    std::vector<std::pair<int, int>> pipes;
    int pipeGap;
    int score;
    bool gameRunning;
    int frameCount;
    
    bool jumpRequested;
    
    int getNonBlockingInput() {
        #ifdef _WIN32
            return _kbhit() ? _getch() : -1;
        #else
            struct termios oldt, newt;
            int ch;
            int oldf;
            
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~(ICANON | ECHO);
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);
            oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
            fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
            
            ch = getchar();
            
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            fcntl(STDIN_FILENO, F_SETFL, oldf);
            
            return ch;
        #endif
    }

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    void drawScreen() {
        clearScreen();
        
        int birbPos = static_cast<int>(birby + 0.5);
        
        for (int i = 0; i < SCREEN_HEIGHT; i++) {
            for (int j = 0; j < SCREEN_WIDTH; j++) {
                if (i == birbPos && j == birbx) {
                    std::cout << "B"; 
                } else {
                    bool isPipe = false;
                    for (const auto& pipe : pipes) {
                        if (j == pipe.first && (i < pipe.second || i > (pipe.second + pipeGap))) {
                            std::cout << "|";
                            isPipe = true;
                            break;
                        }
                    }
                    if (!isPipe) std::cout << " ";
                }
            }
            std::cout << std::endl;
        }
        std::cout << "Score: " << score << std::endl;
    }

    void updatePhysics() {
        // Controlled gravity
        velocity += GRAVITY;
        
        // Speed limit
        velocity = std::min(velocity, MAX_FALL_SPEED);
        
        // Jump mechanics
        if (jumpRequested) {
            velocity = -JUMP_STRENGTH;
            jumpRequested = false;
        }
        
        // Update birb position
        birby += velocity;
        
        // Boundary control
        birby = std::max(0.0, std::min(birby, static_cast<double>(SCREEN_HEIGHT - 1)));
    }

    void updateGame() {
        frameCount++;
        
        // Faster pipe movement (every frame instead of every 3rd)
        // Move pipes
        for (auto& pipe : pipes) {
            pipe.first--;
            
            // Score when pipe fully passes
            if (pipe.first + 1 == birbx) {
                score++;
            }
        }

        // Add new pipes
        if (pipes.empty() || pipes.back().first < SCREEN_WIDTH - 15) {
            int gapY = rand() % (SCREEN_HEIGHT - pipeGap);
            pipes.push_back({SCREEN_WIDTH, gapY});
        }

        // Remove old pipes
        if (!pipes.empty() && pipes.front().first < 0) {
            pipes.erase(pipes.begin());
        }

        // Collision detection
        int birbPos = static_cast<int>(birby + 0.5);
        
        for (const auto& pipe : pipes) {
            // Check if birb is in pipe's x-coordinate
            if (birbx == pipe.first) {
                // Precise vertical collision check
                if (birbPos < pipe.second || birbPos > (pipe.second + pipeGap)) {
                    gameRunning = false;
                    return;
                }
            }
        }
        
        // Boundary checks
        if (birbPos >= SCREEN_HEIGHT || birbPos < 0) {
            gameRunning = false;
        }
    }

    void handleInput() {
        int ch = getNonBlockingInput();
        
        // Up arrow or space to jump
        if (ch == 72 || ch == ' ') {
            jumpRequested = true;
        }
        // Escape to quit
        else if (ch == 27) {
            gameRunning = false;
        }
    }

public:
    FlappyBirb() : 
        birby(SCREEN_HEIGHT/2.0), 
        velocity(0.0),
        birbx(5), 
        pipeGap(5), 
        score(0), 
        gameRunning(true),
        frameCount(0),
        jumpRequested(false)
    {
        std::srand(std::time(nullptr));
    }

    void run() {
        while (gameRunning) {
            handleInput();
            updatePhysics();
            updateGame();
            drawScreen();
            
            // Maintain consistent frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(70));
        }

        clearScreen();
        std::cout << "Game Over! Your Score: " << score << std::endl;
    }
};

// Static const member definitions
const int FlappyBirb::SCREEN_HEIGHT;
const int FlappyBirb::SCREEN_WIDTH;

int main() {
    FlappyBirb game;
    game.run();
    return 0;
}