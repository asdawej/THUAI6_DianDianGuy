#include <vector>
#include <thread>
#include <queue>
#include <array>
#include <chrono>
#include "AI.h"
#include "constants.h"

#define PI 3.1415
// 为假则play()期间确保游戏状态不更新，为真则只保证游戏状态在调用相关方法时不更新
extern const bool asynchronous = false;

// 选手需要依次将player0到player4的职业在这里定义

extern const std::array<THUAI6::StudentType, 4> studentType = {
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent,
    THUAI6::StudentType::StraightAStudent };

extern const THUAI6::TrickerType trickerType = THUAI6::TrickerType::Assassin;
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
void playerBot(IStudentAPI&);
void moveStatus(IStudentAPI& api);
void retreatStatus(IStudentAPI& api);
void initialStatus(IStudentAPI& api);
void idleStatus(IStudentAPI& api);

// 可以在AI.cpp内部声明变量与函数

// 函数

// 循迹相关
double Distance(Point, Point);
std::queue<Point> bfs(Point, Point);
void Goto(IStudentAPI&, double, double, double); // randAngle = 1，则取波动范围为-0.5pi-0.5pi
void InitMapForMove(IAPI&);
//void initHwGroup();
void arrayClear();
int pathLen(Point, Point);

// 状态检查类
bool isSurround(IStudentAPI&, int, int);
bool stuckCheck(IStudentAPI&, int); // 注意，n必须在2-10之间
bool progressStuckCheck(int, int);
bool isTrigger(StudentAPI&, Point); // 学生和目标点是否在九宫格内

// debug相关
void printPathType(IStudentAPI&, std::queue<Point>);
void printQueue(std::queue<Point> q);
void printPosition(IStudentAPI&);
void printPointVector(std::vector<Point>);

// 决策相关

//void groupJuan(IStudentAPI& api);
void closestJuan(IStudentAPI& api);
void graduate(IStudentAPI& api);

// 发送信息相关
void receiveMessage(IStudentAPI& api);

// 爬窗相关
bool isWindowInPath(IStudentAPI& api, std::queue<Point> q);
bool isSurroundWindow(IStudentAPI& api);
bool isDelayedAfterWindow(IStudentAPI& api);

// 躲避tricker相关
Point findNearestPoint(IStudentAPI& api, int n);
double tricker_distance(IStudentAPI& api);
bool isTrickerInsight(IStudentAPI& api);
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
double derectionBeforeRetreat;

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
decision == 1 写作业
decision == 2 毕业
decision == 3 逃跑
...
*/

// groupJuan()相关变量
/*
int hwGroup1Index[5] = { 0,1,3,5,7 };
int hwGroup2Index[5] = { 2,4,6,8,9 };
std::vector<Point> hwGroup1;
std::vector<Point> hwGroup2;
*/
// graduate()相关变量
bool isGraduate = false;
static int framecount = 0; // 计数帧数
// rouse()相关变量
int rouseTarget = -1;
// 爬窗相关变量
static bool isCrossingWindow = 0;
static int CrossWindowCount = 0;

// 通信相关变量
static bool isSendMessage;
static bool hwIsFinished[10];
static int playerDecision[4];
static int playerState[4];
static Point playerPosition[4];
//static int player
void AI::play(IStudentAPI& api)
{
    // 公共操作
    myPlayerID = this->playerID;
    if (this->playerID == 0)
    {
        // 玩家0执行操作
        playerBot(api);
    }
    else if (this->playerID == 1)
    {
        // 玩家1执行操作
        playerBot(api);
    }
    else if (this->playerID == 2)
    {
        // 玩家2执行操作
        playerBot(api);
    }
    else if (this->playerID == 3)
    {
        // 玩家3执行操作
        playerBot(api);
    }
    // 当然可以写成if (this->playerID == 2||this->playerID == 3)之类的操作
    //  公共操作
}

void AI::play(ITrickerAPI& api)
{
    auto self = api.GetSelfInfo();
    api.PrintSelfInfo();
    // api.MoveLeft(1000);
}
void arrayClear()
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
/*
void initHwGroup()
{
    for (int i = 0; i < 5; i++)
    {
        hwGroup1.emplace_back(hw[hwGroup1Index[i]]);
        hwGroup2.emplace_back(hw[hwGroup2Index[i]]);
    }
}
*/
// 搜索最短路径
std::queue<Point> bfs(Point start, Point end)
{
    arrayClear();
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
bool isSurround(IStudentAPI& api, int x, int y)
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
bool isTrigger(IStudentAPI& api, Point p)
{
    auto self = api.GetSelfInfo();
    auto sx = (self->x) / 1000;
    auto sy = (self->y) / 1000;
    if (abs(sx - p.x) <= 1.5 && abs(sy - p.y) <= 1)
        return true;
    return false;
}
void Goto(IStudentAPI& api, double destX, double destY, double randAngle = 0)
{
    // std::printf("goto %d,%d\n", destX, destY);
    auto self = api.GetSelfInfo();
    int sx = self->x;
    int sy = self->y;
    std::printf("-------------%d,%d------------\n", sx, sy);
    auto delta_x = (double)(destX * 1000 - sx);
    auto delta_y = (double)(destY * 1000 - sy);
    std::printf("-------------%lf,%lf------------\n", delta_x, delta_y);
    double ang = 0;
    // 直接走
    ang = atan2(delta_y, delta_x);
    api.Move(300, ang + (std::rand() % 10 - 5) * PI / 10 * randAngle);
}
// 判断实际速度是否为0（防止卡墙上）
bool stuckCheck(IStudentAPI& api, int n)
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
void printPathType(IStudentAPI& api, std::queue<Point> q)
{
    while (!q.empty())
    {
        std::cout << "(" << q.front().x << "," << q.front().y << ")->" << (int)api.GetPlaceType(q.front().x, q.front().y) << " ";
        q.pop();
    }
    std::cout << std::endl;
}
void printPosition(IStudentAPI& api)
{
    auto self = api.GetSelfInfo();
    auto sx = self->x;
    auto sy = self->y;
    std::cout << "position: (" << sx << "," << sy << ")" << std::endl;
}
/*
void groupJuan(IStudentAPI& api) //暂不使用
{
    std::vector <int> temp;
    std::vector <Point>hwGroup;
    if (myPlayerID % 2 == 0)
    {
        hwGroup = hwGroup1;
    }
    else
    {
        hwGroup = hwGroup2;
    }

    for (int i = 0; i < hwGroup.size(); i++)
    {
        temp.emplace_back(api.GetClassroomProgress(hwGroup[i].x, hwGroup[i].y));
    }
    for (int i = 0; i < hwGroup.size(); i++)
    {
        //std::cout<<"isTrigger:"<< isTrigger(api, hw[i])<<"progress:"<<temp[i]<<std::endl;
        if (isTrigger(api, hwGroup[i]) && temp[i] < 10000000)
        {
            std::cout << "doing hw:" << i << "progress:" << temp[i] << std::endl;
            api.StartLearning();
            if (progressStuckCheck(temp[i], 9))
            {
                api.Move(300, rand());
                BotStatus = status::initial;
                return;
            }

            return;
        }
    }
    for (int i = 0; i < hwGroup.size(); i++)
    {
        if (temp[i] < 10000000)
        {
            std::cout << "goto hw" << i << temp[i] << std::endl;
            targetP.x = hwGroup[i].x;
            targetP.y = hwGroup[i].y;
            BotStatus = status::initial;
            return;
        }
    }
    std::cout << "graduate!!" << std::endl;
    isGraduate = true;
}
*/
void closestJuan(IStudentAPI& api)
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
        // std::cout<<"isTrigger:"<< isTrigger(api, hw[i])<<"progress:"<<temp[i]<<std::endl;
        if (hwIsFinished[i] == 1)
        {
            continue;
        }
        if (isTrigger(api, hw[i]) && temp[i] < 10000000 && hwIsFinished[i] == 0)
        {
            std::cout << "doing hw:" << i << "progress:" << temp[i] << std::endl;
            api.StartLearning();
            if (progressStuckCheck(temp[i], 9))
            {
                api.Move(300, rand());
                BotStatus = status::initial;
                return;
            }
            return;
        }
        if (temp[i] == 10000000)
        {
            hwIsFinished[i] = 1;
            isSendMessage = 1;
            std::cout << "work finished!" << std::endl;
            std::cout << "now hwIsFinished:";
            for (int m = 0; m < 10; m++)
            {
                std::cout << hwIsFinished[m];
            }
        }
    }

    targetP = Point(0, 0);
    for (int i = 0; i < hw.size(); i++)
    {
        int tempDis = pathLen(hw[i], Point(cellX, cellY));
        if (temp[i] < 10000000 && tempDis < dis && hwIsFinished[i] == 0)
        {
            dis = tempDis;
            targetP.x = hw[i].x;
            targetP.y = hw[i].y;
        }
    }
    if (targetP.x == 0 && targetP.y == 0)
    {
        std::cout << "graduate!!" << std::endl;
        isGraduate = true;
    }
    else
    {
        BotStatus = status::initial;
        return;
    }
}
void graduate(IStudentAPI& api)
{
    auto self = api.GetSelfInfo();
    auto sx = self->x;
    auto sy = self->y;
    auto cellX = sx / 1000;
    auto cellY = sy / 1000;
    auto selfP = Point(cellX, cellY);
    if (isTrigger(api, gate[1]) && api.GetGateProgress(gate[1].x, gate[1].y) < 18000)
    {
        std::cout << "graduating!"
            << "progress:" << api.GetGateProgress(gate[1].x, gate[1].y) << std::endl;
        api.StartOpenGate();
        if (progressStuckCheck(api.GetGateProgress(gate[1].x, gate[1].y), 9))
        {
            api.Move(300, rand());
            BotStatus = status::initial;
            return;
        }
        return;
    }
    else if (isTrigger(api, gate[1]))
    {
        api.Graduate();
    }
    else
    {
        targetP.x = gate[1].x;
        targetP.y = gate[1].y;
        BotStatus = status::initial;
    }
}
void rouse(IStudentAPI& api)
{
    auto self = api.GetSelfInfo();
    auto sx = self->x;
    auto sy = self->y;
    auto cellX = sx / 1000;
    auto cellY = sy / 1000;
    auto selfP = Point(cellX, cellY);
    auto infoInsight = api.GetStudents();
    int addictProgress = 0;
    if (rouseTarget == -1) 
    {
        BotStatus = status::idle;
        return;
    }
    if (isTrigger(api, playerPosition[rouseTarget]) && playerState[rouseTarget] == 3)
    {
        api.StartRouseMate(rouseTarget);
        return;
    }
    else if(playerState[rouseTarget] == 3)
    {
        targetP.x = playerPosition[rouseTarget].x;
        targetP.y = playerPosition[rouseTarget].y;
        BotStatus = status::initial;
    }
    else 
    {
        rouseTarget = -1;
        BotStatus = status::idle;
    }
}
// 躲避tricker相关
Point findNearestPoint(IStudentAPI& api, int n)
{
    bool flag = false; // 记录是否找到
    int x = api.GetSelfInfo()->x / 1000;
    int y = api.GetSelfInfo()->y / 1000;
    int trickerx = 0;
    int trickery = 0;
    if (isTrickerInsight(api) == 1)
    {
        std::vector<std::shared_ptr<const THUAI6::Tricker>> tricker_vector = api.GetTrickers();
        trickerx = tricker_vector.front()->x / 1000;
        trickery = tricker_vector.front()->y / 1000;
    }
    std::cout << "trickerx:" << trickerx << "trickery:" << trickery << std::endl;
    Point nearestPoint;
    double mindis = 999999;
    for (int i = 0; i < 50; i++)
    {
        for (int j = 0; j < 50; j++)
        {
            if ((int)api.GetPlaceType(i, j) == n)
            {
                if (Distance(Point(i, j), Point(x, y)) < mindis)
                {
                    mindis = Distance(Point(i, j), Point(x, y));
                    nearestPoint = Point(i, j);
                }
            }
        }
    }
    return nearestPoint;
}

// 躲避tricker相关
double tricker_distance(IStudentAPI& api)
{
    std::cout << "Getting tricker distance..." << std::endl;

    double distance = Constants::basicStudentAlertnessRadius;
    // dangerAlert
    double danger = api.GetSelfInfo()->dangerAlert;
    if (danger > 0)
        distance /= danger;
    // 可视
    std::vector<std::shared_ptr<const THUAI6::Tricker>> tricker_vector = api.GetTrickers();
    if (tricker_vector.size() > 0)
    {
        for (int i = 0; i < tricker_vector.size(); i++)
        {
            double temp_distance = sqrt(pow(tricker_vector[i]->x - api.GetSelfInfo()->x, 2) +
                pow(tricker_vector[i]->y - api.GetSelfInfo()->y, 2));
            distance = std::min(distance, temp_distance);
        }
    }
    return distance / 1000;
}
std::string arrayToString(bool a[], int size)
{
    std::string result;
    for (int i = 0; i < size; i++)
    {
        result += a[i];
    }
    return result;
}
void stringToArray(const std::string& str, bool a[], int size)
{
    for (int i = 0; i < size; i++)
    {
        a[i] = str[i];
    }
}
std::string arrayToString(int a[], int size)
{
    std::string result;
    for (int i = 0; i < size; i++)
    {
        result += a[i];
    }
    return result;
}
void stringToArray(const std::string& str, int a[], int size)
{
    for (int i = 0; i < size; i++)
    {
        a[i] = str[i];
    }
}
bool isWindowInPath(IStudentAPI& api, std::queue<Point> p)
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
bool isSurroundWindow(IStudentAPI& api)
{
    auto self = api.GetSelfInfo();
    int x = self->x / 1000;
    int y = self->y / 1000;
    if (x < 47 && x >2 && y < 47 && y >2)
    {
        for (int i = x - 2; i <= x + 2; i++)
        {
            for (int j = y - 2; j <= y + 2; j++)
            {
                if ((int)api.GetPlaceType(i, j) == 7)
                    return true;
            }
        }
    }
    return false;
}
void receiveMessage(IStudentAPI& api)
{
    while (api.HaveMessage())
    {
        std::cout << "receiving message!" << std::endl;
        auto receive = api.GetMessage();
        int buffer[27];
        int tempID;
        stringToArray(receive.second, buffer, 27);
        tempID = buffer[0];
        int* tempMessage = buffer + 1;

        for (int m = 0; m < 10; m++)
        {
            hwIsFinished[m] += tempMessage[m];
        }
        for (int m = 0; m < 4; m++)
        {
            if(m == tempID)
                playerDecision[m] = tempMessage[m+10];
        }
        for (int m = 0; m < 4; m++)
        {
            if(m == tempID)
			    playerState[m] = tempMessage[m+14];
		}
        for (int m = 0; m < 4; m++)
        {
            if(m == tempID)
                playerPosition[m].x = tempMessage[m+18];
        }
        for (int m = 0; m < 4; m++)
        {
			if(m == tempID)
				playerPosition[m].y = tempMessage[m+22];
		}
        std::cout << "message received!" << std::endl;
        for (int m = 0; m < 18; m++)
        {
            std::cout << tempMessage[m]<< "||";
        }
        std::cout << std::endl;
    }
}

bool isTrickerInsight(IStudentAPI& api)
{
    std::vector<std::shared_ptr<const THUAI6::Tricker>> tricker_vector = api.GetTrickers();
    if (tricker_vector.size() > 0)
    {
        return true;
    }
    else
        return false;
}
//爬完窗后不会重复爬
bool isDelayedAfterWindow(IStudentAPI& api)
{
    CrossWindowCount++;
    if (CrossWindowCount >= 10)
    {
        CrossWindowCount = 0;
        return true;
    }
    else
        return false;

}
void botInit(IStudentAPI& api)      //状态机的初始化
{
    std::ios::sync_with_stdio(false);
    auto self = api.GetSelfInfo();
    framecount++;
    if (framecount >10)
    {
        receiveMessage(api);
        isSendMessage = 1;
        framecount = 0;
    }
    if (!hasInitMap)
    {
        InitMapForMove(api);
        hasInitMap = true;
        //initHwGroup();
    }
    std::cout << isTrickerInsight(api) << std::endl;

    playerDecision[self->playerID] = decision;
    playerState[self->playerID] = (int)self->playerState;
    playerPosition[self->playerID].x = self->x/1000;
    playerPosition[self->playerID].y = self->y/1000;

    //print info

    std::cout << "isHwFinished: ";
    for (int i = 0; i < 10; i++)
    {
		std::cout << hwIsFinished[i] << "||";
	}
    std::cout << std::endl;
    std::cout << "playerDecision: ";
    for (int i = 0; i < 4; i++)
    {
        std::cout << playerDecision[i] << "||";
    }
    std::cout << std::endl;
    std::cout << "playerState: ";
    for (int i = 0; i < 4; i++)
    {
		std::cout << playerState[i] << "||";
	}
    std::cout << std::endl;
    std::cout << "playerPosition: ";
    for (int i = 0; i < 4; i++)
    {
        //print playerPosition
        std::cout <<"("<< playerPosition[i].x << "," << playerPosition[i].y << ")" << "||";
    }
    std::cout << std::endl;
    

    if (isSendMessage == 1)
    {
        std::string tempstring;
        tempstring += api.GetSelfInfo()->playerID;
        tempstring += arrayToString(hwIsFinished, 10);
        tempstring += arrayToString(playerDecision, 4);
        tempstring += arrayToString(playerState, 4);
        for (int i = 0; i < 4; i++)
        {
            tempstring += playerPosition[i].x;
        }
        for (int i = 0; i < 4; i++)
        {
			tempstring += playerPosition[i].y;
		}
        for (int k = 0; k < 4; k++)
        {
            if (k != api.GetSelfInfo()->playerID)
            {
                api.SendTextMessage(k, tempstring);
                std::cout << "sending message!" << std::endl;
            }
        }
        isSendMessage = 0;
    }


    if (isTrickerInsight(api) == 1 && decision != 3)
    {
        decision = 3;
        std::cout << "distance:" << tricker_distance(api) << std::endl;
        if (tricker_distance(api) <= 2)
        {
            targetP = findNearestPoint(api, 7);
            std::cout << "goto window" << std::endl;
        }
        else
        {
            targetP = findNearestPoint(api, 3);
            std::cout << "goto grass" << std::endl;
        }
        BotStatus = status::initial;
        return;
    }
    else if (isTrickerInsight(api) != 1)
    {   

        for (int i = 0; i < 4; i++)
        {

            if (playerState[i] == 3)
            {
                decision = 4;
                rouseTarget = i;
                if(rouseTarget == -1)
                    BotStatus = status::idle;
                return;
            }
        }
        if (rouseTarget != -1)
        {
            rouseTarget = -1;
            BotStatus = status::idle;
		}
        if (!isGraduate)
        {
            decision = 1;
        }
        else if (isGraduate)
        {
            decision = 2;
        }
    }
}
// ——————————————状态机函数—————————————————————————————
void playerBot(IStudentAPI& api)
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

void moveStatus(IStudentAPI& api)
{
    auto self = api.GetSelfInfo();
    std::cout << "move!" << std::endl;
    std::cout << "isSurroundWindow:" << isSurroundWindow(api) << std::endl;
    if (isCrossingWindow == 1)
    {
        if (isSurroundWindow(api) == 1 && isDelayedAfterWindow(api) == 1)
        {
            std::cout << "my skipping window!" << std::endl;
            api.SkipWindow();
            api.SkipWindow();

        }
    }
    if ((int)api.GetPlaceType(targetP.x, targetP.y) == 4)
    {
        if (!path.empty() && !isTrigger(api, targetP))
        {
            // std::cout << path.front().x << path.front().y << std::endl;
            Goto(api, path.front().x + 0.5, path.front().y + 0.5);
            if (isSurround(api, path.front().x + 0.5, path.front().y + 0.5))
                path.pop();
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
            if (isSurround(api, path.front().x + 0.5, path.front().y + 0.5))
                path.pop();
        }
        else
        {
            BotStatus = status::idle;
        }
    }

    if (stuckCheck(api, 3))
    {
        BotStatus = status::retreat;
        stuckCheckStartTime = std::chrono::system_clock::now();
        derectionBeforeRetreat = self->facingDirection;
    }
    /*
    if (isTrickerInsight(api) == 1)
    {
        BotStatus = status::avoid_initial;
        return;
    }
    */
}
void retreatStatus(IStudentAPI& api)
{
    std::cout << "retreat" << std::endl;
    if (isCrossingWindow == 1)
    {
        if (isSurroundWindow(api) == 1 && isDelayedAfterWindow(api) == 1)
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


void initialStatus(IStudentAPI& api)
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
void idleStatus(IStudentAPI& api)
{
    std::cout << "idling!" << std::endl;

    switch (decision)
    {
    case 1:
    {
        closestJuan(api);
        break;
    }
    case 2:
    {
        std::cout << "graduate!!" << std::endl;
        graduate(api);
        break;
    }
    case 3:
    {
        decision = 0;
        break;
    }
    case 4:
    {
        rouse(api);
        break;
    }
    default:
    {
        break;
    }
    }
}