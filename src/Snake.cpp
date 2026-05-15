#include<iostream>
#include<cctype>
#include<vector>
#include<thread>
#include<chrono>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include<random>
using namespace std;

class Screen {
private:
    struct Point2D {
        int x = 0;
        int y = 0;
    };

    Point2D projectPoint(Point2D p) { // Its called 'Normalized Coordinates'
        Point2D result;

        int numX = 1;
        int numY = 1;
        string xLen = to_string(p.x);
        string yLen = to_string(p.y);

        for (char c : xLen) {
            if (isalnum(c)) 
                numX *= 10;
        }

        for (char c : yLen) {
            if (isalnum(c))
                numY *= 10;
        }

        float nx = float(p.x) / numX;
        float ny = float(p.y) / numY;

        result.x = max(0, min(WIDTH-1, int((nx + 1) * (WIDTH - 1) / 2.0f)));
        result.y = max(0, min(HEIGHT-1, int((1 - ny) * (HEIGHT - 1) / 2.0f)));
        return result;
    }

    int HEIGHT;
    int WIDTH;
    vector<vector<char>> screen;

    void initScreen() {
        screen.resize(HEIGHT);
        for (int i = 0; i < HEIGHT; i++) {
            screen[i].resize(WIDTH, ' ');
        }
    }

public:
    Screen() : HEIGHT(20), WIDTH(40) { initScreen(); }

    void printScreen() {
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                if (y < 1 || y == HEIGHT - 1) cout << "-"; else if (x < 1 || x == WIDTH - 1) cout << "|"; else cout << screen[y][x];
            }
            cout << "\n";
        }
    }

    void plotPoint(int x, int y, bool pickup) {
        Point2D p = projectPoint(Point2D{x, y});
        if (p.x >= 0 && p.x < WIDTH && p.y >= 0 && p.y < HEIGHT) {
            if (pickup) screen[p.y][p.x] = '*'; else screen[p.y][p.x] = '#';
        }
    }

    void removePoint(const int& x, const int& y) {
        Point2D p = projectPoint(Point2D{x, y});
        screen[p.y][p.x] = ' ';
    }

    void convertPickupPos(int x, int y) {
        Point2D p = projectPoint(Point2D{x, y});
    }
};

class Objects {
private:
    int randomInt(int min, int max) {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> dist(min, max);
        return dist(gen);
    }
public:
    vector<pair<int, int>> pickupPos; // Save pickup positions

    vector<pair<int, int>> generatePickups(Screen& s, const int bodyX, const int bodyY, const int count, int& max) {
        int x = randomInt(-10, 10);
        int y = randomInt(-10, 10);

        vector<pair<int, int>> result;

        int extraPickupChance = randomInt(1, max);
        if (extraPickupChance == 1) {
            int x1 = randomInt(-10, 10);
            int y1 = randomInt(-10, 10);

            result.push_back({x1, y1});
        }
        result.push_back({x, y});
        if (count == 10) { max--; } else if (count == 20) { max--; } else if (count == 30) { max--; } else if (count == 40) { max--; }

        return result;
    }
};

class Snake {
private:
    struct snakeBody {
        int x, y;
        snakeBody* next;

        snakeBody(const int& hor, const int& vert) : x(hor), y(vert), next(nullptr) {}
    };

    snakeBody* head;

public:
    Snake() : head(nullptr) {}

    void addBody(int x, int y) {
        snakeBody* newPiece = new snakeBody(x, y);
        
        if (!head) {
            head = newPiece;
            return;
        }

        snakeBody* curr = head;
        while (curr->next) {
            curr = curr->next;
        }
        curr->next = newPiece;
    }

    snakeBody getHeadPos() {
        if (!head) throw runtime_error("Snake is empty!");

        snakeBody* curr = head;
        if (curr->next) {
            curr = curr->next;
        }

        return *curr;
    }

    void updatePos(Screen& s, int prevX, int prevY) {
        if (!head) throw runtime_error("Snake is empty!");

        int nx;
        int ny;

        snakeBody* curr = head->next;
        if (curr) {
            while (curr != nullptr) {
                nx = curr->x; // save positions
                ny = curr->y;

                curr->x = prevX; // set new positions
                curr->y = prevY;

                prevX = nx; // update previous positions
                prevY = ny;

                curr = curr->next;
            }
        }

        s.removePoint(prevX, prevY);
    }

    void plotFullBody(Screen& s) {
        if (!head) throw runtime_error("Snake is empty!");

        snakeBody* curr = head->next;
        if (curr) {
            while (curr != nullptr) {
                s.plotPoint(curr->x, curr->y, false);
                curr = curr->next;
            }
        }
    }

    bool checkBodyCollision(Objects& object, Snake& snake, snakeBody body, Screen& screen, const int prevX, const int prevY, int& count) {
        if (!head) throw runtime_error("Snake is empty!");

        snakeBody* curr = head->next;
        if (curr) {
            while (curr) {
                if (body.x == curr->x && body.y == curr->y || body.x == -10 || body.x == 10 || body.y == -10 || body.y == 9) { // Screen border collision
                    return true;
                } else curr = curr->next;
            }
        }

        for (auto it = object.pickupPos.begin(); it != object.pickupPos.end(); ++it) {
            if (body.x == it->first && body.y == it->second) { count++; snake.addBody(prevX, prevY); object.pickupPos.erase(it); return false; }
        }
        return false;
    }

    bool checkFullBodyCollision(vector<pair<int, int>> pos) {
        snakeBody* curr = head->next;
        if (curr) {
            while (curr) {
                for (const auto& p : pos) {
                    if (p.first == curr->x && p.second == curr->y) return true;
                }
                curr = curr->next;
            }
        }
        return false;
    }
};

class input { // IDK HOW TS WORKS YET BUT TERMINAL INPUT (dont worry about ts you'll never use it again)
private:
    void setNonBlocking(bool enable) {
        struct termios ttystate;
        tcgetattr(STDIN_FILENO, &ttystate);

        if (enable) {
            ttystate.c_lflag &= ~ICANON; // disable line buffering
            ttystate.c_lflag &= ~ECHO;   // disable echo
            ttystate.c_cc[VMIN] = 0;     // min chars to read
            ttystate.c_cc[VTIME] = 0;    // no timeout
        } else {
            ttystate.c_lflag |= ICANON;  // restore canonical mode
            ttystate.c_lflag |= ECHO;    // restore echo
        }

        tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
    }
public:
    input() { setNonBlocking(true); }

    bool kbhit() {
        fd_set set;
        struct timeval tv = {0, 0};
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
    }

    void setBlockingFalse() {
        setNonBlocking(false);
    }
};

int main() {
    Screen screen;
    Snake snake;
    input i;
    Objects object;

    // Setup clock
    int time = 5000;
    using clock = chrono::steady_clock;
    auto lastPickupTime = clock::now();
    auto pickupInterval = chrono::milliseconds(time);

    // Initialize body cells and get position
    snake.addBody(0, 0);
    snake.addBody(0, -1);
    snake.addBody(0, -2);
    snake.addBody(0, -3);
    auto body = snake.getHeadPos();

    // Track pickups
    int max = 5;
    int pickupCount = 1;

    char dir = 'w';

    while (true) {
        int prevX = body.x;
        int prevY = body.y;

        if (i.kbhit()) {
            char c;
            read(STDIN_FILENO, &c, 1);
            // Inputs
            switch(c) {
                case 'w': dir = 'w'; break;
                case 's': dir = 's'; break;
                case 'd': dir = 'd'; break;
                case 'a': dir = 'a'; break;
                default: break;
            }
        }
        // Movement
        if (dir == 'w') body.y += 1; else if (dir == 's') body.y -= 1; else if (dir == 'd') body.x += 1; else if (dir == 'a') body.x -= 1;

        // Plot and setup full snake body per frame
        screen.plotPoint(body.x, body.y, false);
        snake.updatePos(screen, prevX, prevY);
        snake.plotFullBody(screen);
        std::cout << "\033c"; // Reset terminal

        // Collision
        if (snake.checkBodyCollision(object, snake, body, screen, prevX, prevY, pickupCount)) { cout << "Game Over" << endl; break; }

        // Handle pickups
        auto now = clock::now();
        if (now - lastPickupTime >= pickupInterval) {
            vector<pair<int, int>> generatedPos;
            do { generatedPos = object.generatePickups(screen, body.x, body.y, pickupCount, max); } while (snake.checkFullBodyCollision(generatedPos));
            for (auto& p : generatedPos) {
                screen.plotPoint(p.first, p.second, true);
                object.pickupPos.push_back({p});
            }
            lastPickupTime = now;
        }

        for (auto it = object.pickupPos.begin(); it != object.pickupPos.end(); ++it) {
            screen.convertPickupPos(it->first, it->second);
        }

        time -= 200;
        cout << body.x << ", " << body.y << endl;
        cout << pickupCount << endl;

        screen.printScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    i.setBlockingFalse();
};

// Overly-complicated snake to demonstrate use of simple data structures and algorithms