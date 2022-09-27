#include <iostream>
#include <fstream>
extern "C"
{
#include "allhead.h"
#include "font.h"
}

using namespace std;

class Bmp
{
    friend class Dlist;

public:
    int showanybmp(const char *bmppath, int x, int y, int w, int h);

private:
    char path[20];
    int x;
    int y;
    int w;
    int h;
};

/*
    x --》图片左上角的位置坐标
    y --》图片左上角的位置坐标
    w --》图片实际的宽
    h --》图片实际的高
*/
int Bmp::showanybmp(const char *bmppath, int x, int y, int w, int h)
{
    int bmpfd;
    int lcdfd;
    int i;

    int w_change;
    w_change = w;
    while (w_change % 4 != 0)
    {
        w_change++;
    }

    //定义指针保存lcd在内存中的首地址
    int *lcdpoint;
    //定义数组存放你读取到的bmp图片的颜色值
    char bmpbuf[w_change * h * 3];
    //定义数组存放转换得到的ARGB数据
    int lcdbuf[w_change * h]; // int类型数据占4个字节

    //打开你要显示的w*h的bmp图片
    bmpfd = open(bmppath, O_RDWR);
    if (bmpfd == -1)
    {
        perror("打开bmp图片失败了!\n");
        return -1;
    }

    //打开液晶屏的驱动
    lcdfd = open("/dev/fb0", O_RDWR);
    if (lcdfd == -1)
    {
        perror("打开lcd失败了!\n");
        return -1;
    }

    //映射得到lcd在内存中的首地址
    lcdpoint = (int *)mmap(NULL, 800 * 480 * 4, PROT_READ | PROT_WRITE, MAP_SHARED, lcdfd, 0);
    if (lcdpoint == NULL)
    {
        perror("获取lcd的首地址失败了!\n");
        return -1;
    }

    //跳过图片最前面的54字节的头信息
    lseek(bmpfd, 54, SEEK_SET);

    //从第55字节开始读取
    read(bmpfd, bmpbuf, w_change * h * 3);

    //把三个字节的BGR数据转换成四个字节ARGB
    for (i = 0; i < w_change * h; i++)
        lcdbuf[i] = 0x00 << 24 | bmpbuf[3 * i + 2] << 16 | bmpbuf[3 * i + 1] << 8 | bmpbuf[3 * i];

    //把图片每一行像素点拷贝到液晶屏对应的位置
    // 399x250  被填充成400*250     499x200 被填充成500x200
    for (i = 0; i < h; i++)
        memcpy(lcdpoint + (y + h - 1 - i) * 800 + x, &lcdbuf[i * w_change], w * 4);

    //关闭
    close(bmpfd);
    close(lcdfd);
    //解除映射
    munmap(lcdpoint, 800 * 480 * 4);
    return 0;
}

class Touchscreen
{
public:
    int open_touchscreen();
    int touch_screen(); //触发一次触摸屏的函数
    int close_touchscreen();
    int control();
    void set_stop()
    {
        stop = 1;
    }
    void set_continue()
    {
        stop = 0;
    }
    int get_stop()
    {
        return stop;
    }
    void init_speed()
    {
        speed = 1;
    }
    int get_speed()
    {
        return speed;
    }
    int move;
    int begin;
    int reset;
    int music;

private:
    int tsfd;
    int x;
    int y;
    int stop;
    int speed;
};

int Touchscreen::open_touchscreen()
{
    //打开触摸屏的驱动
    tsfd = open("/dev/input/event0", O_RDWR);
    if (tsfd == -1)
    {
        perror("打开触摸屏失败!\n");
        return -1;
    }
    return 0;
}

int Touchscreen::touch_screen() //触发一次触摸屏的函数
{
    int flag = 0;
    struct input_event myevent;

    while (1)
    {
        //读取触摸屏点击的坐标位置  阻塞
        read(tsfd, &myevent, sizeof(myevent));
        if (myevent.type == EV_ABS) //说明确实是触摸屏的驱动
        {
            if (myevent.code == ABS_X) //说明是x坐标
            {
                x = myevent.value;
                flag++;
            }
            if (myevent.code == ABS_Y) //说明是y坐标
            {
                y = myevent.value;
                flag++;
            }
            if (flag == 2)
            {
                flag = 0;
                cout << "(" << x << "," << y << ")" << endl;
                break;
            }
        }
    }
    return 0;
}

int Touchscreen::control()
{
    if (x > 110 && x < 165 && y > 370 && y < 420 && move != 2)
    {
        move = 1;
        cout << "点击了方向键：右" << endl;
    }
    else if (x > 0 && x < 55 && y > 370 && y < 420 && move != 1)
    {
        move = 2;
        cout << "点击了方向键：左" << endl;
    }
    else if (x > 55 && x < 105 && y > 310 && y < 365 && move != 4)
    {
        move = 3;
        cout << "点击了方向键：上" << endl;
    }
    else if (x > 55 && x < 105 && y > 420 && y < 479 && move != 3)
    {
        move = 4;
        cout << "点击了方向键：下" << endl;
    }
    else if (x > 82 && x < 160 && y > 82 && y < 150 && stop == 0)
    {
        stop = 1;
        cout << "点击了暂停键" << endl;
    }
    else if (x > 0 && x < 80 && y > 82 && y < 150 && stop == 1)
    {
        stop = 0;
        cout << "点击了继续键" << endl;
    }
    else if (x > 425 && x < 605 && y > 145 && y < 195 && stop == 1)
    {
        Bmp show;
        cout << "点击了退出游戏键" << endl;
        UnInit_Font();
        close_touchscreen();
        usleep(300000);
        show.showanybmp("final.bmp", 0, 0, 800, 480);
        exit(0);
    }
    else if (x > 425 && x < 605 && y > 215 && y < 265 && stop == 1)
    {
        speed = 1;
        cout << "点击了恢复原速键" << endl;
    }
    else if (x > 425 && x < 605 && y > 285 && y < 335 && stop == 1)
    {
        reset = 1;
        cout << "点击了重新开始键" << endl;
    }
    else if (x > 425 && x < 515 && y > 355 && y < 405 && stop == 1)
    {
        if (music == 0)
        {
            system("madplay 1.mp3 &");
            music = 2;
        }
        if (music == 1)
        {
            system("killall -CONT madplay");
            music = 2;
        }
        cout << "点击了放歌键" << endl;
    }
    else if (x > 515 && x < 605 && y > 355 && y < 405 && stop == 1)
    {
        if (music == 2)
        {
            system("killall -STOP madplay");
            music = 1;
        }
        cout << "点击了关歌键" << endl;
    }
    else if (x > 82 && x < 160 && y > 151 && y < 215 && speed > 1)
    {
        speed = speed / 2;
        cout << "点击了速度除以2" << endl;
    }
    else if (x > 0 && x < 80 && y > 151 && y < 215 && speed < 8)
    {
        speed = speed * 2;
        cout << "点击了速度乘以2" << endl;
    }
    else if (x > 0 && x < 160 && y > 0 && y < 165)
    {
        begin = 1;
        cout << "点击了开始游戏" << endl;
    }
    else
    {
    }
}

int Touchscreen::close_touchscreen()
{
    //关闭触摸屏
    close(tsfd);
    return 0;
}

class Node
{
    friend class Dlist;

private:
    char path[20];
    int x;
    int y;
    Node *next;
    Node *prev;
};

class Dlist
{
public:
    Dlist()
    {
        head = new Node;
        head->x = 0;
        head->y = 0;
        strcpy(head->path, "clear.bmp");
        head->next = head;
        head->prev = head;
    }
    void insert(const char *_path, int _x, int _y);
    void showSnake();
    void delFirstNode();
    void delAllNode();
    void changeColor();

    void moveRight();
    void moveLeft();
    void moveUp();
    void moveDown();

    int getSnakeHeadx();
    int getSnakeHeady();

    int eatSelfBody();

    int color;

private:
    Node *head;
};

void Dlist::delAllNode()
{
    Node *p = head;
    Node *p1 = p->next;
    while (p->next != head)
    {
        p = p->next;
        p1 = p1->next;
        head->next = p1;
        p1->prev = head;
        p->next = NULL;
        p->prev = NULL;
        delete p;
        p = p1->prev;
    }
    delete p;
}

int Dlist::eatSelfBody()
{
    Node *p = head;
    while (p->next != head)
    {
        p = p->next;
    }
    Node *body = head;
    while (body->next != p)
    {
        body = body->next;
        if (p->x == body->x && p->y == body->y)
        {
            return -1;
        }
    }
    return 1;
}

int Dlist::getSnakeHeadx()
{
    Node *p = head;
    while (p->next != head)
    {
        p = p->next;
    }
    return p->x;
}

int Dlist::getSnakeHeady()
{
    Node *p = head;
    while (p->next != head)
    {
        p = p->next;
    }
    return p->y;
}

void Dlist::insert(const char *_path, int _x, int _y)
{
    Node *newnode = new Node;
    newnode->x = _x;
    newnode->y = _y;
    strcpy(newnode->path, _path);

    Node *p = head;
    while (p->next != head)
    {
        p = p->next;
    }
    p->next = newnode;
    newnode->prev = p;
    newnode->next = head;
}

void Dlist::showSnake()
{
    Bmp show;
    Node *p = head;
    while (p->next != head)
    {
        p = p->next;
        show.showanybmp(p->path, p->x, p->y, 20, 20);
    }
}

void Dlist::delFirstNode()
{
    Bmp show;
    Node *p = head->next;
    show.showanybmp("clear.bmp", p->x, p->y, 20, 20);
    Node *p1 = p->next;
    head->next = p1;
    p1->prev = head;
    p->next = NULL;
    p->prev = NULL;
    delete p;
}

void Dlist::moveRight()
{
    delFirstNode();
    Node *p = head;
    int temp;
    while (p->next != head)
    {
        p = p->next;
    }
    temp = p->x + 20;
    if (temp > 799)
    {
        temp = 180;
    }
    if (color == 0)
        insert("redsnake.bmp", temp, p->y);
    else if (color == 1)
        insert("bluesnake.bmp", temp, p->y);
    else if (color == 2)
        insert("greensnake.bmp", temp, p->y);
}

void Dlist::moveLeft()
{
    delFirstNode();
    Node *p = head;
    int temp;
    while (p->next != head)
    {
        p = p->next;
    }
    temp = p->x - 20;
    if (temp < 180)
    {
        temp = 780;
    }
    if (color == 0)
        insert("redsnake.bmp", temp, p->y);
    else if (color == 1)
        insert("bluesnake.bmp", temp, p->y);
    else if (color == 2)
        insert("greensnake.bmp", temp, p->y);
}

void Dlist::moveUp()
{
    delFirstNode();
    Node *p = head;
    int temp;
    while (p->next != head)
    {
        p = p->next;
    }
    temp = p->y - 20;
    if (temp < 0)
    {
        temp = 460;
    }
    if (color == 0)
        insert("redsnake.bmp", p->x, temp);
    else if (color == 1)
        insert("bluesnake.bmp", p->x, temp);
    else if (color == 2)
        insert("greensnake.bmp", p->x, temp);
}

void Dlist::moveDown()
{
    delFirstNode();
    Node *p = head;
    int temp;
    while (p->next != head)
    {
        p = p->next;
    }
    temp = p->y + 20;
    if (temp > 479)
    {
        temp = 0;
    }
    if (color == 0)
        insert("redsnake.bmp", p->x, temp);
    else if (color == 1)
        insert("bluesnake.bmp", p->x, temp);
    else if (color == 2)
        insert("greensnake.bmp", p->x, temp);
}

void Dlist::changeColor()
{
    Node *p = head;
    if (color == 0)
    {
        while (p->next != head)
        {
            p = p->next;
            strcpy(p->path, "redsnake.bmp");
        }
    }
    else if (color == 1)
    {
        while (p->next != head)
        {
            p = p->next;
            strcpy(p->path, "bluesnake.bmp");
        }
    }
    else if (color == 2)
    {
        while (p->next != head)
        {
            p = p->next;
            strcpy(p->path, "greensnake.bmp");
        }
    }
}

class Food
{
    friend class Dlist;
    friend class Node;

public:
    void showFood();
    void eatFood();
    int getFoodx();
    int getFoody();
    int color;

private:
    int food_x;
    int food_y;
};

int Food::getFoodx()
{
    return food_x;
}

int Food::getFoody()
{
    return food_y;
}

void Food::showFood()
{
    Bmp show;
    food_x = 180 + rand() % 799;
    if (food_x > 799)
    {
        food_x = food_x - 800 + 180;
    }
    food_y = rand() % 479;
    if (color == 0)
        show.showanybmp("blue.bmp", food_x, food_y, 20, 20);
    else if (color == 1)
        show.showanybmp("green.bmp", food_x, food_y, 20, 20);
    else if (color == 2)
        show.showanybmp("red.bmp", food_x, food_y, 20, 20);
}

void Food::eatFood()
{
    Bmp show;
    show.showanybmp("clear.bmp", food_x, food_y, 20, 20);
}

void *touchTask(void *arg)
{
    Touchscreen *p = (Touchscreen *)arg;
    while (1)
    {
        p->touch_screen();
        p->control();
    }
}

int main()
{
    ifstream fin;

    Init_Font();
    fin.open("record.txt", ios_base::in);
    char record[50] = {0};
    fin.getline(record, sizeof(record));
    int record_int = atoi(record);

    int score = 0;
    char score_char[50] = {0};
    sprintf(score_char, "%d", score);
    unsigned char *score_uchar = (unsigned char *)score_char;

    Touchscreen touch;
    Bmp show;
    show.showanybmp("background.bmp", 0, 0, 800, 480);

    unsigned char *urecord = (unsigned char *)record;
    Display_characterX(85, 230, urecord, 0xFF0000, 2);
    Display_characterX(85, 280, score_uchar, 0xFF0000, 2);

    fin.close();

    touch.open_touchscreen();
    touch.begin = 0;
    touch.reset = 0;
    touch.music = 0;
    pthread_t id;
    //创建子线程--》帮助我并发执行清除垃圾的任务
    pthread_create(&id, NULL, touchTask, &touch);

    while (1)
    {
        Dlist snake;
        Food food;
        food.color = 0;
        snake.color = 0;
        fin.open("record.txt", ios_base::in);
        fin.getline(record, sizeof(record));
        fin.close();
        Clean_Area(85, 230, 70, 30, 0xFFFFFF);
        Clean_Area(85, 280, 70, 30, 0xFFFFFF);
        Display_characterX(85, 230, urecord, 0xFF0000, 2);
        Display_characterX(85, 280, score_uchar, 0xFF0000, 2);
        while (1)
        {
            if (touch.begin == 1)
                break;
        }
        snake.insert("redsnake.bmp", 340, 240);
        snake.insert("redsnake.bmp", 360, 240);
        snake.insert("redsnake.bmp", 380, 240);
        snake.insert("redsnake.bmp", 400, 240);
        snake.showSnake();
        food.showFood();
        touch.set_continue();
        touch.init_speed();
        touch.move = 1; //最开始向右移动
        while (1)
        {
            snake.changeColor();
            if (touch.get_stop() == 1)
            {
                while (1)
                {
                    show.showanybmp("menu.bmp", 330, 50, 360, 400);
                    if (touch.get_stop() == 0)
                    {
                        show.showanybmp("background.bmp", 0, 0, 800, 480);
                        Display_characterX(85, 230, urecord, 0xFF0000, 2);
                        Display_characterX(85, 280, score_uchar, 0xFF0000, 2);
                        break;
                    }
                    if (touch.reset == 1)
                        break;
                }
                if (touch.reset == 1)
                {
                    touch.reset = 0;
                    snake.delAllNode();
                    if (score > record_int)
                    {
                        ofstream fout("record.txt");
                        fout.write(score_char, strlen(score_char));
                        fout.close();
                        record_int = score;
                    }
                    memset(score_char, 0, sizeof(score_char));
                    strcpy(score_char, "0");
                    score_uchar = (unsigned char *)score_char;
                    show.showanybmp("background.bmp", 0, 0, 800, 480);
                    score = 0;
                    break;
                }
                // show.showanybmp("background.bmp", 0, 0, 800, 480);
                show.showanybmp("blue.bmp", food.getFoodx(), food.getFoody(), 20, 20);
            }
            if (touch.move == 1)
            {
                snake.moveRight();
            }
            else if (touch.move == 2)
            {
                snake.moveLeft();
            }
            else if (touch.move == 3)
            {
                snake.moveUp();
            }
            else if (touch.move == 4)
            {
                snake.moveDown();
            }
            snake.showSnake();
            if (snake.eatSelfBody() == -1)
            {
                show.showanybmp("lose.bmp", 280, 100, 500, 120);
                snake.delAllNode();
                sleep(2);
                touch.begin = 0;
                if (score > record_int)
                {
                    ofstream fout("record.txt");
                    fout.write(score_char, strlen(score_char));
                    fout.close();
                    record_int = score;
                }
                memset(score_char, 0, sizeof(score_char));
                strcpy(score_char, "0");
                score_uchar = (unsigned char *)score_char;
                show.showanybmp("background.bmp", 0, 0, 800, 480);
                score = 0;
                break;
            }
            if (touch.get_speed() == 1)
                usleep(600000);
            else if (touch.get_speed() == 2)
                usleep(300000);
            else if (touch.get_speed() == 4)
                usleep(150000);
            else if (touch.get_speed() == 8)
                usleep(75000);
            if (snake.getSnakeHeadx() <= food.getFoodx() + 19 && snake.getSnakeHeadx() >= food.getFoodx() - 19 && snake.getSnakeHeady() <= food.getFoody() + 19 && snake.getSnakeHeady() >= food.getFoody() - 19)
            {
                food.color++;
                snake.color++;
                if (food.color == 3)
                    food.color = 0;
                if (snake.color == 3)
                    snake.color = 0;
                show.showanybmp("clear.bmp", food.getFoodx(), food.getFoody(), 20, 20);
                food.showFood();
                if (touch.move == 1)
                {
                    if (snake.getSnakeHeadx() + 20 < 799)
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx() + 20, snake.getSnakeHeady());
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx() + 20, snake.getSnakeHeady());
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx() + 20, snake.getSnakeHeady());
                    }
                    else
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx() + 20 - 800, snake.getSnakeHeady());
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx() + 20 - 800, snake.getSnakeHeady());
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx() + 20 - 800, snake.getSnakeHeady());
                    }
                }
                else if (touch.move == 2)
                {
                    if (snake.getSnakeHeadx() - 20 >= 180)
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx() - 20, snake.getSnakeHeady());
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx() - 20, snake.getSnakeHeady());
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx() - 20, snake.getSnakeHeady());
                    }
                    else
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx() - 20 + 620, snake.getSnakeHeady());
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx() - 20 + 620, snake.getSnakeHeady());
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx() - 20 + 620, snake.getSnakeHeady());
                    }
                }
                else if (touch.move == 3)
                {
                    if (snake.getSnakeHeady() - 20 >= 0)
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20);
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20);
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20);
                    }
                    else
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20 + 480);
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20 + 480);
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() - 20 + 480);
                    }
                }
                else if (touch.move == 4)
                {
                    if (snake.getSnakeHeady() + 20 < 479)
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20);
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20);
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20);
                    }
                    else
                    {
                        if (snake.color == 0)
                            snake.insert("redsnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20 - 480);
                        else if (snake.color == 1)
                            snake.insert("bluesnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20 - 480);
                        else if (snake.color == 2)
                            snake.insert("greensnake.bmp", snake.getSnakeHeadx(), snake.getSnakeHeady() + 20 - 480);
                    }
                }

                if (touch.get_speed() == 1)
                    score = score + 1;
                else if (touch.get_speed() == 2)
                    score = score + 2;
                else if (touch.get_speed() == 4)
                    score = score + 4;
                else if (touch.get_speed() == 8)
                    score = score + 8;

                memset(score_char, 0, sizeof(score_char));
                sprintf(score_char, "%d", score);
                score_uchar = (unsigned char *)score_char;
                Clean_Area(85, 280, 70, 30, 0xFFFFFF);
                Display_characterX(85, 280, score_uchar, 0xFF0000, 2);

                if (score >= 9999)
                {
                    ofstream fout("record.txt");
                    fout.write(score_char, strlen(score_char));
                    fout.close();
                    memset(score_char, 0, sizeof(score_char));
                    strcpy(score_char, "0");
                    score_uchar = (unsigned char *)score_char;
                    score = 0;
                }
            }
        }
    }

    touch.close_touchscreen();
    return 0;
}