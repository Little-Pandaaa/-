#define _CRT_SECURE_NO_WARNINGS

#include <graphics.h>//easy-x库
#include <windows.h>//Windows 系统 API 函数支持，如 Sleep()、线程函数 CreateThread()。用于系统调用。
#include <tchar.h>//支持 Unicode 编码的字符处理，允许 _T() 宏使用，兼容宽字符和多字节字符。
#include <time.h>//提供时间函数，如 time()、srand()、GetTickCount()，用于计时、随机化等。
#include <mmsystem.h>  // 播放音效相关的函数
#pragma comment(lib, "winmm.lib")  // 链接播放声音所需的库
IMAGE img1;
IMAGE img[3];
IMAGE marioFrames[2];
int emptyX, emptyY; // 空块位置
bool marioThreadRunning = true;

DWORD startTime;
int count;
IMAGE tiles[5][5];
int map[5][5];  // 当前拼图布局，存储 tile 编号，最后一块是空白（N*N - 1）
int TILE_SIZE;// 每块图像大小
struct Button {
    int x, y, width, height;
    const TCHAR* label;
};

Button buttons[] = {
    {380, 200, 200, 40, _T("1. 开始游戏")},
    {380, 300, 200, 40, _T("2. 游戏说明")},
    {380, 400, 200, 40, _T("3. 退出")}
};

Button rates[] = {
    {380, 200, 200, 40, _T("难度1. 3 X 3")},
    {380, 300, 200, 40, _T("难度2. 4 X 4")},
    {380, 400, 200, 40, _T("难度3. 5 X 5")}
};

Button pictures[] = {
    {100, 200, 180, 300, NULL},
    {380, 200, 180, 300, NULL},
    {660, 200, 180, 300, NULL}
};
void choosePicture(int N);
// ------------------ 工具函数 ------------------
// 交换两个拼图块
void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

// 判断是否点击了按钮
int getButtonClicked(int mx, int my,Button buttons[]) {
    for (int i = 0; i < 3; i++) {
        if (mx >= buttons[i].x && mx <= buttons[i].x + buttons[i].width &&
            my >= buttons[i].y && my <= buttons[i].y + buttons[i].height) {
            return i + 1;
        }
    }
    return 0;
}

//加载灯笼
void loadMarioImages() {
    loadimage(&marioFrames[0], _T("灯笼1.jpg"), 105, 160);
    loadimage(&marioFrames[1], _T("灯笼2.jpg"), 105, 160);
    //loadimage(&marioFrames[2], _T("变脸3.jpg"), 105, 160);
}


// ------------------ 主菜单 ------------------
void showMainMenu() {
    //cleardevice();
    //setbkcolor(WHITE);
    cleardevice();
    loadimage(&img1, _T("背景.jpg"));
    putimage(0, 0, &img1);
    settextcolor(DARKGRAY);
    setfillcolor(CYAN);
    setbkmode(TRANSPARENT);//设置文字背景透明，背景颜色完全填充
    settextstyle(35, 0, _T("SimSun"));
    outtextxy(280, 50, _T("=== 传统文化拼图游戏 ==="));

    settextstyle(25, 0, _T("SimSun"));

    for (int i = 0; i < 3; i++) {
        setfillcolor(CYAN);
        solidrectangle(buttons[i].x, buttons[i].y,
            buttons[i].x + buttons[i].width,
            buttons[i].y + buttons[i].height);
        outtextxy(buttons[i].x + 10, buttons[i].y + 8, buttons[i].label);
    }

}

// ------------------ 游戏说明 ------------------
void showInstructions() {
    cleardevice();
    loadimage(&img1, _T("背景.jpg"));
    putimage(0, 0, &img1);
    settextstyle(25, 0, _T("SimHei"));
    outtextxy(100, 100, _T("游戏说明："));
    outtextxy(100, 150, _T("点击图块，将空白块与相邻图块交换"));
    outtextxy(100, 180, _T("完成图案还原即胜利"));
    outtextxy(100, 220, _T("点击鼠标返回菜单..."));
    MOUSEMSG m;
    while (1) {
        m = GetMouseMsg();
        if (m.uMsg == WM_LBUTTONDOWN)
            break;
    }
}

// ------------------ 退出游戏 ------------------
void exitGame() {
    cleardevice();
    settextstyle(25, 0, _T("SimHei"));
    outtextxy(200, 200, _T("感谢游玩，游戏即将退出！"));
    Sleep(1500);
}

// ------------------ 判断是否胜利 ------------------
bool checkWin(int N) {
    int count = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i == N - 1 && j == N - 1) return true; // 空白处默认正确
            if (map[i][j] != count++) return false;
        }
    }
    return true;
}

// ------------------ 初始化拼图 ------------------
void initPuzzle(int N, int picture_num) {

    emptyX = N - 1, emptyY = N - 1; // 空块位置
    TILE_SIZE = 420 / N;
    SetWorkingImage(&img[picture_num]);

    // 截图成 tiles[N][N]
    int count = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            getimage(&tiles[i][j], j * TILE_SIZE, i * TILE_SIZE, TILE_SIZE, TILE_SIZE);
            map[i][j] = count++;
        }
    }

    // 最后一块设为空
    map[N - 1][N - 1] = -1;
    emptyX = N - 1;
    emptyY = N - 1;

    SetWorkingImage(NULL);

    // 打乱拼图（简单随机）
    for (int i = 0; i < 100; ++i) {
        int dir = rand() % 4;
        int dx[] = { 0, 1, 0, -1 };
        int dy[] = { -1, 0, 1, 0 };
        int nx = emptyX + dx[dir];
        int ny = emptyY + dy[dir];
        if (nx >= 0 && nx < N && ny >= 0 && ny < N) {
            swap(&map[emptyY][emptyX], &map[ny][nx]);
            emptyX = nx;
            emptyY = ny;
        }
    }
}

// ------------------ 绘制拼图 ------------------
void drawPuzzle(int N,int picture_num) {
    // 创建一个缓冲区图像
    IMAGE buffer;                      // 声明缓冲区对象
    buffer.Resize(960, 720);           // 调整大小与窗口一致
    // 使用 SetWorkingImage 设置为内存中的缓冲区图像
    SetWorkingImage(&buffer);
    //setbkcolor(WHITE);
    settextcolor(RED);
    setbkmode(TRANSPARENT);
    // 只清空背景，不清除所有图像
    cleardevice();
    loadimage(&img1, _T("背景.jpg"));
    putimage(0, 0, &img1);
    // 绘制拼图
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (map[i][j] != -1) {  // 如果不是空块，绘制拼图块
                int id = map[i][j];
                int y = id / N;
                int x = id % N; 
                putimage(j * TILE_SIZE + 50, i * TILE_SIZE + 50, &tiles[y][x]);
            }
            //setlinecolor(BLACK);
            rectangle(j * TILE_SIZE + 50, i * TILE_SIZE + 50,
                j * TILE_SIZE + 50 + TILE_SIZE,
                i * TILE_SIZE + 50 + TILE_SIZE);
        }
    }
    putimage(500, 50, &img[picture_num]);
    DWORD now = GetTickCount();
    TCHAR buf[100];
    _stprintf(buf, _T("用时：%ld 秒"), (now - startTime) / 1000);  // 转换为秒
    outtextxy(50, 500, buf);
    TCHAR clickText[50];
    _stprintf(clickText, _T("点击次数: %d 次"), count);
    outtextxy(50, 560, clickText);
    // 使用 SetWorkingImage 恢复默认工作图像，并将缓冲区图像显示在屏幕上
    SetWorkingImage(NULL);
    putimage(0, 0, &buffer);  // 将缓冲区内容绘制到屏幕
}





void showWinDialog(int count, DWORD timeUsed)
{
    int winWidth = 400;   // 弹窗宽度
    int winHeight = 250;  // 弹窗高度
    int winX = (getwidth() - winWidth) / 2;  // 居中X坐标
    int winY = (getheight() - winHeight) / 2; // 居中Y坐标

    // 弹窗背景（白色圆角矩形，带阴影效果）
    setfillcolor(RGB(230, 230, 230));  // 浅灰色阴影
    fillroundrect(winX + 5, winY + 5, winX + winWidth + 5, winY + winHeight + 5, 15, 15);

    setfillcolor(WHITE);  // 白色弹窗
    fillroundrect(winX, winY, winX + winWidth, winY + winHeight, 15, 15);

    // 设置文字样式
    settextcolor(BLACK);
    settextstyle(28, 0, _T("宋体"));
    outtextxy(winX + 120, winY + 30, _T("🎉 拼图完成！"));

    settextstyle(20, 0, _T("宋体"));
    TCHAR infoText[256];
    _stprintf(infoText, _T("操作次数: %d"), count);
    outtextxy(winX + 100, winY + 80, infoText);

    _stprintf(infoText, _T("用时: %d 秒"), timeUsed);
    outtextxy(winX + 100, winY + 120, infoText);

    // 绘制"确定"按钮
    setfillcolor(RGB(70, 130, 180));  // 深蓝色按钮
    fillroundrect(winX + 150, winY + 170, winX + 250, winY + 210, 10, 10);
    settextcolor(WHITE);
    outtextxy(winX + 185, winY + 175, _T("确定"));

    // 等待用户点击"确定"按钮
    while (true) {
        MOUSEMSG m = GetMouseMsg();
        if (m.uMsg == WM_LBUTTONDOWN) {
            if (m.x >= winX + 150 && m.x <= winX + 250 &&
                m.y >= winY + 170 && m.y <= winY + 210) {
                marioThreadRunning = true;
                break;  // 点击了"确定"按钮
            }
        }
    }
}

void game(int N, int picture_num)
{
    count = 0;
    initPuzzle(N, picture_num);
    startTime = GetTickCount();
    bool won = false;

    while (!won) {
        drawPuzzle(N, picture_num);

        MOUSEMSG m = GetMouseMsg();
        if (m.uMsg == WM_LBUTTONDOWN) {
            int x = (m.x - 50) / TILE_SIZE;
            int y = (m.y - 50) / TILE_SIZE;

            if (x >= 0 && x < N && y >= 0 && y < N) {
                if ((abs(x - emptyX) == 1 && y == emptyY) ||
                    (abs(y - emptyY) == 1 && x == emptyX)) {
                    count++;
                    swap(&map[y][x], &map[emptyY][emptyX]);
                    emptyX = x;
                    emptyY = y;
                }
            }
            won = checkWin(N);
        }
    }

    // 计算用时（秒）
    DWORD timeUsed = (GetTickCount() - startTime) / 1000;

    // 显示自定义弹窗（不覆盖背景）
    showWinDialog(count, timeUsed);

    cleardevice();  // 返回菜单前清屏
}
// ------------------ 游戏主逻辑 ------------------
void startGame() {
    cleardevice();//
    loadimage(&img1, _T("背景.jpg"));
    putimage(0, 0, &img1);
    settextstyle(35, 0, _T("SimSun"));
    outtextxy(280, 50, _T("=== 请选择游戏难度 ==="));
    setbkmode(TRANSPARENT);
    settextstyle(25, 0, _T("SimSun"));

    for (int i = 0; i < 3; i++) {
        setfillcolor(CYAN);
        solidrectangle(rates[i].x, rates[i].y,
            rates[i].x + rates[i].width,
            rates[i].y + rates[i].height);
        outtextxy(rates[i].x + 10, rates[i].y + 8, rates[i].label);
    }
    MOUSEMSG n;
    while (1) {
        n = GetMouseMsg();
        if (n.uMsg == WM_LBUTTONDOWN) {
            int choice = getButtonClicked(n.x, n.y,rates);
            if (choice == 1) {
                choosePicture(3);
                break;
            }
            else if (choice == 2) {
                choosePicture(4);
                break;
            }
            else if (choice == 3) {
                choosePicture(5);
                break;
            }
        }
    }

}
//选择图片
void choosePicture(int N) {
    cleardevice();
    loadimage(&img1, _T("背景.jpg"));
    putimage(0, 0, &img1);
    settextstyle(35, 0, _T("SimSun"));
    outtextxy(280, 50, _T("=== 请选择图片 ==="));
    setbkmode(TRANSPARENT);
    TCHAR filename[20];
    IMAGE resized[3];
    for (int i = 0; i < 3; i++) {
        _stprintf(filename, _T("%d.jpg"), i+1); // 构造文件名
        loadimage(&img[i], filename, 420, 420);
        loadimage(&resized[i], filename, 180, 300);
    }

    for (int i = 0; i < 3; i++) {
        putimage(pictures[i].x, pictures[i].y, &resized[i]);
    }
    MOUSEMSG n;
    while (1) {
        n = GetMouseMsg();
        if (n.uMsg == WM_LBUTTONDOWN) {
            int choice = getButtonClicked(n.x, n.y,pictures);
            if (choice == 1) {
                marioThreadRunning = false;
                game(N,0);
                break;
            }
            else if (choice == 2) {
                marioThreadRunning = false;
                game(N,1);
                break;
            }
            else if (choice == 3) {
                marioThreadRunning = false;
                game(N,2);
                break;
            }
        }
    }

}
//显示马里奥
DWORD WINAPI showMario(LPVOID lpParam) {
    int currentFrame = 0;
    while (marioThreadRunning){
        // 防止与主线程冲突（可选）
        BeginBatchDraw();

        // 清除马里奥区域（右下角）
        setfillcolor(TRANSPARENT);
        solidrectangle(855, 560, 960, 720);  // 64x64 区域

        // 显示当前帧
        putimage(855, 560, &marioFrames[currentFrame]);

        EndBatchDraw();

        // 切换下一帧
        currentFrame = (currentFrame + 1) % 2;

        Sleep(500); // 帧率控制
    }

    return 0;
}


// ------------------ 主函数 ------------------
int main() {

    initgraph(960, 720);
    srand((unsigned int)time(NULL));
    PlaySound(_T("为你写的歌-方大同#aGt3.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
    loadMarioImages();
    
    while (1) {
        showMainMenu();
        CreateThread(NULL, 0, showMario, NULL, 0, NULL);
        MOUSEMSG m;
        while (1) {
            m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN) {
                int choice = getButtonClicked(m.x, m.y,buttons);
                if (choice == 1) {
                    startGame();
                    break;
                }
                else if (choice == 2) {
                    showInstructions();
                    break;
                }
                else if (choice == 3) {
                    PlaySound(NULL, NULL, 0);
                    exitGame();
                    closegraph();
                    return 0;
                }

            }
        }
    }

    return 0;
}
