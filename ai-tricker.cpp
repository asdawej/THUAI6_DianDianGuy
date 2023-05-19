#include <vector>
#include <thread>
#include <queue>
#include <array>
#include <chrono>
#include "AI.h"
#include "constants.h"
#include "utils.hpp"

#define PI 3.1415
#define VERSION 230519.1

// 辅助函数
int (*G2C)(int) = IAPI::GridToCell;
int (*C2G)(int) = IAPI::CellToGrid;

// 为假则play()期间确保游戏状态不更新，为真则只保证游戏状态在调用相关方法时不更新
extern const bool asynchronous = false;

// 选手需要依次将player0到player4的职业在这里定义
extern const std::array<THUAI6::StudentType, 4> studentType = {
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent };

// Tricker职业，修改职业同时需要修改数值
extern const THUAI6::TrickerType trickerType = THUAI6::TrickerType::Assassin;
const int attack_dist_limit = 1480;
const int skill_dist_limit = 3700;

// 记录函数

// 定义非常多的状态（有限状态机）
enum class status
{
    initial,
    watch,
    sorround,
    idle,
    reset,
    index,
    retreat,
    avoid,
    move
};

static status BotStatus = status::idle;
static status LastStatus = status::reset;

// 存储路径的常量
const int MAXN = 50;
int a[MAXN][MAXN];      // 存储地图
int vis[MAXN][MAXN];    // 标记数组，记录每个点是否被访问过
int dis[MAXN][MAXN];    // 记录每个点到起点的最短距离
int pre[MAXN][MAXN][2]; // 记录每个点的前驱节点

// 定义坐标结构体
struct Point
{
    int x, y;
    Point() {}
    Point(int x, int y) : x(x), y(y) {}
};

// 定义队列元素结构体
struct Node
{
    Point p;
    int dist;
    Node(int d) { dist = d; }
    Node(Point p, int dist) : p(p), dist(dist) {}
    bool operator<(const Node& other) const
    { // 定义优先级，距离更小的优先级更高
        return dist > other.dist;
    }
};

// 状态机函数
void playerBot(ITrickerAPI&);
void moveStatus(ITrickerAPI& api);
void retreatStatus(ITrickerAPI& api);
void initialStatus(ITrickerAPI& api);
void idleStatus(ITrickerAPI& api);

// 函数

// 循迹相关
double Distance(Point, Point);
std::queue<Point> bfs(Point, Point);
void Goto(ITrickerAPI&, double, double, double); // randAngle = 1，则取波动范围为-0.5pi-0.5pi
void InitMapForMove(IAPI&);
//void initHwGroup();
void searchingArrayClear();
int pathLen(Point, Point);

// 状态检查类
bool isSurround(ITrickerAPI&, int, int);
bool stuckCheck(ITrickerAPI&, int); // 注意，n必须在2-10之间
bool progressStuckCheck(int, int);
int boolArrayCount(bool[], int);
void boolArrayReset(bool[], int);
bool isTrigger(ITrickerAPI&, Point);

// debug相关
void printPathType(ITrickerAPI&, std::queue<Point>);
void printQueue(std::queue<Point> q);
void printPosition(ITrickerAPI&);
void printPointVector(std::vector<Point>);

// 决策相关

void antiJuan(ITrickerAPI& api);


// 爬窗相关
bool isWindowInPath(ITrickerAPI& api, std::queue<Point> q);
bool isSurroundWindow(ITrickerAPI& api);
bool isDelayedAfterWindow(ITrickerAPI& api);

// 变量

// 循迹相关变量
static int64_t myPlayerID;
static THUAI6::PlaceType map[50][50];
static int steps;
static int hasinitmap;
static int hasBeenTo;
static bool hasInitMap = false;
static bool IHaveArrived = false;
static int lastX = 0, lastY = 0;
static int lastFrameCount = 0;
std::queue<Point> path;

// stuckCheck()相关变量
int32_t memoryX[10];
int32_t memoryY[10];
std::chrono::system_clock::time_point stuckCheckStartTime;

// progressStuckCheck()相关变量
int32_t memoryProgress[10];

// 目标坐标
Point targetP = Point(12, 3);

// 特殊点坐标
std::vector<Point> hw;
std::vector<Point> door;
std::vector<Point> window;
std::vector<Point> gate;
std::vector<Point> chest;

// 决策相关变量

// 总决策函数相关变量
int decision;
/*
    decision = 1 追击
    decision = 2 巡逻
...
*/

// 爬窗相关变量
static bool isCrossingWindow = 0;
static bool climbingCheck = 0;
static int climbingFrameCount = 0;

// 巡逻相关变量
bool hwIsFinished[10];
bool hwControl[10];// 判断是否已经到达过该点
int patrolMode;
/*
patrolMode = 1 在作业间巡逻
patrolMode = 2 在作业和校门间巡逻
patrolMode = 3 在校门间巡逻
*/

// 追击相关变量
int chasingFrameCount;
bool chasingProtect;

//========================================

void AI::play(IStudentAPI& api)
{
    // 公共操作
    myPlayerID = this->playerID;
    if (this->playerID == 0)
    {
        // 玩家0执行操作
    }
    else if (this->playerID == 1)
    {
        // 玩家1执行操作
    }
    else if (this->playerID == 2)
    {
        // 玩家2执行操作
    }
    else if (this->playerID == 3)
    {
        // 玩家3执行操作
    }
    // 当然可以写成if (this->playerID == 2||this->playerID == 3)之类的操作
    //  公共操作
}

void AI::play(ITrickerAPI& api)
{
    auto self = api.GetSelfInfo();
    api.PrintSelfInfo();
    playerBot(api);
}

void antiJuan(ITrickerAPI& api) {
    auto stus = api.GetStudents();
    if (stus.size() != 0) {
        Point stu_loc = { -100,-100 };
        Point now_loc_grid = { api.GetSelfInfo()->x, api.GetSelfInfo()->y };
        Point now_loc = { G2C(now_loc_grid.x), G2C(now_loc_grid.y) };
        for (int i = 0; i < stus.size(); i++) {
            Point temp_loc_grid = { stus[i]->x,stus[i]->y };
            Point temp_loc = { G2C(temp_loc_grid.x), G2C(temp_loc_grid.y) };
            if (Distance(now_loc, temp_loc) < Distance(now_loc, stu_loc) && stus[i]->playerState != THUAI6::PlayerState::Addicted)
                stu_loc = temp_loc;
            if (api.GetSelfInfo()->timeUntilSkillAvailable[0] == 0 && stus[i]->playerState != THUAI6::PlayerState::Addicted)
                api.UseSkill(0);    // 隐身，主要是得分；除此以外可以更加接近学生
            if (Distance(now_loc_grid, temp_loc_grid) <= skill_dist_limit && stus[i]->playerState != THUAI6::PlayerState::Addicted) {
                if (api.GetSelfInfo()->timeUntilSkillAvailable[1] == 0)
                    api.UseSkill(1);    // 启动飞刀，飞刀时间短，力求充分使用
                api.Attack(atan2(temp_loc_grid.y - now_loc_grid.y, temp_loc_grid.x - now_loc_grid.x));
                BotStatus = status::idle;
                return;
            }
            if (Distance(now_loc_grid, temp_loc_grid) <= attack_dist_limit && stus[i]->playerState != THUAI6::PlayerState::Addicted) {
                api.Attack(atan2(temp_loc_grid.y - now_loc_grid.y, temp_loc_grid.x - now_loc_grid.x));
                BotStatus = status::idle;
                return;
            }
        }
        if (stu_loc.x == -100 && stu_loc.y == -100)
        {
            decision = 2;
            BotStatus = status::idle;
        }
        else
        {
            targetP = stu_loc;
            BotStatus = status::initial;
        }

        return;
    }
    decision = 2;
    BotStatus = status::idle;
    return;
}

void patrolMode1(ITrickerAPI& api)
{
    double dis = 999999;
    std::vector<int> temp;
    auto self = api.GetSelfInfo();
    auto sx = self->x;
    auto sy = self->y;
    auto cellX = sx / 1000;
    auto cellY = sy / 1000;
    for (int i = 0; i < hw.size(); i++)
    {
        temp.emplace_back(api.GetClassroomProgress(hw[i].x, hw[i].y));
        std::cout << "temp" << i << ":" << temp[i] << std::endl;
    }
    for (int i = 0; i < hw.size(); i++)
    {
        if (hwIsFinished[i] == 1)
        {
            continue;
        }
        if (isTrigger(api, hw[i]) && temp[i] < 10000000 && hwIsFinished[i] == 0)
        {
            hwControl[i] = 1;
        }
        if (temp[i] == 10000000)
        {
            hwIsFinished[i] = 1;
        }
    }

    targetP = Point(0, 0);
    for (int i = 0; i < hw.size(); i++)
    {
        int tempDis = pathLen(hw[i], Point(cellX, cellY));
        if (tempDis < dis && hwIsFinished[i] == 0 && hwControl[i] == 0)
        {
            dis = tempDis;
            targetP.x = hw[i].x;
            targetP.y = hw[i].y;
        }
    }
    BotStatus = status::initial;
    return;
}
void patrolMode2(ITrickerAPI& api)
{
}
void patrolMode3(ITrickerAPI& api)
{
}

void walkingAround(ITrickerAPI& api)
{

    patrolMode = 1;
    if (boolArrayCount(hwControl, 10) >= 10 - boolArrayCount(hwIsFinished, 10))
    {
        std::cout << "hwReset!!!" << std::endl;
        boolArrayReset(hwControl, 10);
    }
    switch (patrolMode)
    {
    case 1:
        patrolMode1(api);
        break;
    case 2:
        patrolMode2(api);
        break;
    case 3:
        patrolMode3(api);
        break;
    default:
        break;
    }

}

int boolArrayCount(bool arr[], int n)
{
    int count = 0;
    for (int i = 0; i < n; i++)
    {
        if (arr[i] == 1)
        {
            count++;
        }
    }
    return count;
}

void boolArrayReset(bool arr[], int n)
{
    for (int i = 0; i < n; i++)
    {
        arr[i] = 0;
    }
}

void searchingArrayClear()
{
    int i, j;
    for (i = 0; i < 50; i++)
    {
        for (j = 0; j < 50; j++)
        {
            vis[i][j] = 0;
            dis[i][j] = 0;
            pre[i][j][0] = 0;
            pre[i][j][1] = 0;
        }
    }
}

void InitMapForMove(IAPI& api)
{
    int i, j;
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 休眠 2 秒
    a[26][42] = 1;
    a[13][48] = 1;
    for (i = 0; i < 50; i++)
    {
        for (j = 0; j < 50; j++)
        {
            a[i][j] = (int)api.GetPlaceType(i, j) - 1;
            if (a[i][j] >= 10)
                a[i][j] = 9;
            if (a[i][j] == 2)
                a[i][j] = 0;
            std::cout << a[i][j];
            if ((int)api.GetPlaceType(i, j) == 4)
            {
                hw.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 8)
            {
                door.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 9)
            {
                door.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 10)
            {
                door.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 7)
            {
                window.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 5)
            {
                gate.push_back(Point(i, j));
            }
            else if ((int)api.GetPlaceType(i, j) == 11)
            {
                chest.push_back(Point(i, j));
            }
        }
        std::cout << std::endl;
    }

    for (int i = 0; i < door.size(); i++)
    {
        a[door[i].x][door[i].y] = 1;
    }
    for (int i = 0; i < window.size(); i++)
    {
        a[window[i].x][window[i].y] = 0;
    }
    api.Wait();
}

// 搜索最短路径
std::queue<Point> bfs(Point start, Point end)
{
    searchingArrayClear();
    std::queue<Point> path;
    std::priority_queue<Node> q;
    q.push(Node(start, 0));
    vis[start.x][start.y] = 1;
    std::cout << " check1 " << std::endl;
    while (!q.empty())
    {
        Node cur = q.top();
        q.pop();
        Point p = cur.p;
        int dist = cur.dist;
        if (p.x == end.x && p.y == end.y)
        { // 找到终点
            path.push(p);
            while (p.x != start.x || p.y != start.y)
            { // 回溯路径
                Point pre_p = Point(pre[p.x][p.y][0], pre[p.x][p.y][1]);
                path.push(pre_p);
                p = pre_p;
            }
            break;
        }
        if (p.x - 1 >= 0 && !vis[p.x - 1][p.y] && a[p.x - 1][p.y] == 0)
        { // 向上搜索
            vis[p.x - 1][p.y] = 1;
            dis[p.x - 1][p.y] = dist + 1;
            pre[p.x - 1][p.y][0] = p.x;
            pre[p.x - 1][p.y][1] = p.y;
            q.push(Node(Point(p.x - 1, p.y), dist + 1));
        }
        if (p.x + 1 < MAXN && !vis[p.x + 1][p.y] && a[p.x + 1][p.y] == 0)
        { // 向下搜索
            vis[p.x + 1][p.y] = 1;
            dis[p.x + 1][p.y] = dist + 1;
            pre[p.x + 1][p.y][0] = p.x;
            pre[p.x + 1][p.y][1] = p.y;
            q.push(Node(Point(p.x + 1, p.y), dist + 1));
        }
        if (p.y - 1 >= 0 && !vis[p.x][p.y - 1] && a[p.x][p.y - 1] == 0)
        { // 向左搜索
            vis[p.x][p.y - 1] = 1;
            dis[p.x][p.y - 1] = dist + 1;
            pre[p.x][p.y - 1][0] = p.x;
            pre[p.x][p.y - 1][1] = p.y;
            q.push(Node(Point(p.x, p.y - 1), dist + 1));
        }
        if (p.y + 1 < MAXN && !vis[p.x][p.y + 1] && a[p.x][p.y + 1] == 0)
        { // 向右搜索
            vis[p.x][p.y + 1] = 1;
            dis[p.x][p.y + 1] = dist + 1;
            pre[p.x][p.y + 1][0] = p.x;
            pre[p.x][p.y + 1][1] = p.y;
            q.push(Node(Point(p.x, p.y + 1), dist + 1));
        }
    }
    if (path.empty())
    {
        std::cout << "empty!!" << std::endl;
    }
    printQueue(path);
    return path;
}

void printQueue(std::queue<Point> q)
{
    // printing content of queue
    while (!q.empty())
    {
        std::cout << "(" << q.front().x << "," << q.front().y << ")->";
        q.pop();
    }
    std::cout << std::endl;
}

void printPointVector(std::vector<Point> v)
{
    for (int i = 0; i < v.size(); i++)
    {
        std::cout << "(" << v[i].x << "," << v[i].y << ")->";
    }
    std::cout << std::endl;
}

bool isSurround(ITrickerAPI& api, int x, int y)
{
    double distance;
    auto self = api.GetSelfInfo();
    auto sx = (self->x) / 1000;
    auto sy = (self->y) / 1000;
    distance = sqrt((sx - x) * (sx - x) + (sy - y) * (sy - y));
    if (distance <= 0.3)
        return true;
    return false;
}

bool isTrigger(ITrickerAPI& api, Point p)
{
    auto self = api.GetSelfInfo();
    auto sx = (self->x) / 1000;
    auto sy = (self->y) / 1000;
    if (abs(sx - p.x) <= 1.5 && abs(sy - p.y) <= 1)
        return true;
    return false;
}

void Goto(ITrickerAPI& api, double destX, double destY, double randAngle = 0)
{
    // std::printf("goto %d,%d\n", destX, destY);
    auto self = api.GetSelfInfo();
    int sx = self->x;
    int sy = self->y;

    auto delta_x = (double)(destX * 1000 - sx);
    auto delta_y = (double)(destY * 1000 - sy);
    std::cout << "dx" << delta_x << "dy" << delta_y << std::endl;
    double ang = 0;
    // 直接走
    ang = atan2(delta_y, delta_x);
    std::cout << "angle:" << ang << std::endl;
    if (delta_x != 0 && delta_y != 0)
        api.Move(300, ang + (std::rand() % 10 - 5) * PI / 10 * randAngle);
}

// 判断实际速度是否为0（防止卡墙上）
bool stuckCheck(ITrickerAPI& api, int n)
{
    if (n >= 2 && n <= 10)
    {
        auto self = api.GetSelfInfo();
        auto sx = self->x;
        auto sy = self->y;
        for (int i = 0; i <= n - 2; i++)
        {
            memoryX[i] = memoryX[i + 1];
            memoryY[i] = memoryY[i + 1];
        }
        memoryX[n - 1] = sx;
        memoryY[n - 1] = sy;
        if (abs(memoryX[0] - sx) < 100 && abs(memoryY[0] - sy) < 100)
        {
            std::cout << "stuck!" << std::endl;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool progressStuckCheck(int progress, int n) // 需要更新！

{
    /*
    if (n >= 2 && n <= 10)
    {
        for (int i = 0; i <= n - 2; i++)
        {
            memoryProgress[i] = memoryProgress[i + 1];
        }
        memoryProgress[n - 1] = progress;
        if (memoryProgress[0] == progress)
        {
            std::cout << "progressStuck!" << std::endl;
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    */
    return false;
}

double Distance(Point a, Point b)
{
    return (sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)));
}

int pathLen(Point a, Point b)
{
    auto tempPath = bfs(a, b);
    return tempPath.size();
}

void printPathType(ITrickerAPI& api, std::queue<Point> q)
{
    while (!q.empty())
    {
        std::cout << "(" << q.front().x << "," << q.front().y << ")->" << (int)api.GetPlaceType(q.front().x, q.front().y) << " ";
        q.pop();
    }
    std::cout << std::endl;
}

void printPosition(ITrickerAPI& api)
{
    auto self = api.GetSelfInfo();
    auto sx = self->x;
    auto sy = self->y;
    std::cout << "position: (" << sx << "," << sy << ")" << std::endl;
}

bool isWindowInPath(ITrickerAPI& api, std::queue<Point> p)
{
    std::queue<Point> q = p;
    while (!q.empty())
    {
        if ((int)api.GetPlaceType(q.front().x, q.front().y) == 7)
        {
            return true;
        }
        q.pop();
        std::cout << "popping" << std::endl;
    }
    return false;
}

bool isSurroundWindow(ITrickerAPI& api)
{
    auto self = api.GetSelfInfo();
    int x = self->x / 1000;
    int y = self->y / 1000;
    for (int i = x - 2; i <= x + 2; i++)
    {
        for (int j = y - 2; j <= y + 2; j++)
        {
            if (i >= 1 && i <= 49 && j >= 1 && j <= 49)
            {
                if ((int)api.GetPlaceType(i, j) == 7)
                    return true;
            }
        }
    }
    return false;
}

//爬完窗后不会重复爬
bool isDelayedAfterWindow(ITrickerAPI& api)
{
    if ((int)api.GetSelfInfo()->playerState == 15)
    {
        climbingCheck = true;
        climbingFrameCount = 0;
    }
    climbingFrameCount++;
    if (climbingFrameCount > 15)
    {
        climbingCheck = false;
    }
    return !climbingCheck;
}

void botInit(ITrickerAPI& api)      //状态机的初始化
{
    std::ios::sync_with_stdio(false);
    auto self = api.GetSelfInfo();
    if (!hasInitMap)
    {
        InitMapForMove(api);
        hasInitMap = true;
    }

    auto stus = api.GetStudents();
    int stuNum = 0;
    for (int i = 0; i < stus.size(); i++)
    {
        if ((int)stus[i]->playerState != 3)
        {
            stuNum++;
        }
    }
    std::cout << "decision:" << decision << "stu:" << stuNum << std::endl;
    if (decision == 1)
    {
        chasingFrameCount++;
    }
    if ((stuNum != 0 && decision != 1) || (decision == 1 && chasingFrameCount > 10 && stuNum != 0))
    {
        chasingFrameCount = 0;
        decision = 1;
        BotStatus = status::idle;
    }
    else if (stuNum == 0 && (BotStatus != status::move && BotStatus != status::retreat))
    {
        decision = 2;
    }
}

void playerBot(ITrickerAPI& api)
{
    botInit(api);
    std::cout << "isSurroundWindow:    " << isSurroundWindow(api) << std::endl;
    switch (BotStatus)
    {
        // 有限状态机的core
    case status::initial:
    {
        initialStatus(api);
        break;
    }
    case status::move:
    {
        moveStatus(api);
        // printPosition(api);
        // printQueue(path);
        // printPathType(api, path);
        break;
    }
    case status::retreat:
    {
        retreatStatus(api);
        break;
    }
    case status::idle:
    {
        idleStatus(api);
        break;
    }
    }
    api.Wait();
}

//=============================================

// ——————————————状态机函数—————————————————————————————

void moveStatus(ITrickerAPI& api)
{
    auto self = api.GetSelfInfo();
    std::cout << "move!" << std::endl;
    std::cout << "isSurroundWindow:" << isSurroundWindow(api) << std::endl;
    if (isCrossingWindow)
    {
        if (isSurroundWindow(api) && isDelayedAfterWindow(api))
        {
            std::cout << "my skipping window!" << std::endl;
            api.SkipWindow();
            api.SkipWindow();

        }
    }
    if (isSurround(api, path.front().x + 0.5, path.front().y + 0.5))
        path.pop();

    if (api.GetPlaceType(targetP.x, targetP.y) == THUAI6::PlaceType::ClassRoom)
    {
        if (!path.empty() && !isTrigger(api, targetP))
        {
            // std::cout << path.front().x << path.front().y << std::endl;
            Goto(api, path.front().x + 0.5, path.front().y + 0.5);

        }
        else
        {
            BotStatus = status::idle;
        }
    }
    else
    {
        if (!path.empty())
        {
            // std::cout << path.front().x << path.front().y << std::endl;
            Goto(api, path.front().x + 0.5, path.front().y + 0.5);
        }
        else
        {
            BotStatus = status::idle;
        }
    }

    if (stuckCheck(api, 8))
    {

        BotStatus = status::retreat;
        stuckCheckStartTime = std::chrono::system_clock::now();


    }
    /*
    if (isTrickerInsight(api) == 1)
    {
        BotStatus = status::avoid_initial;
        return;
    }
    */
}

void retreatStatus(ITrickerAPI& api)
{
    std::cout << "retreat" << std::endl;
    if (isCrossingWindow)
    {
        if (isSurroundWindow(api) && isDelayedAfterWindow(api))
        {
            std::cout << "my skipping window!" << std::endl;
            api.SkipWindow();
        }
    }
    double randAngle = 1;
    auto currentTime = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - stuckCheckStartTime);
    if (diff.count() > 500)
    {
        std::cout << "deep rand!!" << std::endl;
        randAngle = 2;
        BotStatus = status::initial;
    }
    if (!path.empty() && !isTrigger(api, targetP))
    {
        std::cout << path.front().x << path.front().y << std::endl;
        Goto(api, path.front().x, path.front().y, randAngle);
        if (isSurround(api, path.front().x, path.front().y))
            path.pop();
    }
    else
    {
        BotStatus = status::idle;
    }
    if (!stuckCheck(api, 3))
    {
        BotStatus = status::move;
    }
}

void initialStatus(ITrickerAPI& api)
{
    std::cout << "initial" << std::endl;

    int x = (api.GetSelfInfo()->x) / 1000;
    int y = (api.GetSelfInfo()->y) / 1000;
    path = bfs(Point(targetP.x, targetP.y), Point(x, y));

    if (isWindowInPath(api, path))
    {
        isCrossingWindow = 1;
    }
    else
        isCrossingWindow = 0;
    std::cout << "isCrossingWindow" << isCrossingWindow << std::endl;
    IHaveArrived = false;
    BotStatus = status::move;
    printQueue(path);
}

void idleStatus(ITrickerAPI& api)
{
    std::cout << "idling!" << std::endl;

    switch (decision)
    {
    case 1:
    {
        antiJuan(api);
        break;
    }
    case 2:
    {
        walkingAround(api);
        break;
    }
    case 3:
    {
        break;
    }
    case 4:
    {
        break;
    }
    default:
    {
        break;
    }
    }
}