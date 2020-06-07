#include <stdio.h>
#include <conio.h>
#include "server.h"
using namespace _ghx;

/* 常量定义 */

// 配置文件名
static const KYString File_Config = "TestServer.ini";
static const KYString File_Output = "TestServer.txt";

// 配置文件的节名
static const KYString Sect_System = "system";

// 配置文件的键名
static const KYString Key_IP = "IP";
static const KYString Key_Port = "Port";
static const KYString Key_RecvThreads = "RecvThreads";

// 定时器
static TKYTimer* timer = NULL;

// 服务器事件
class TServerEvent
{
public:
    // 结束监听
    void dis_conn(TTCPServer* AServer);
    // 用户登录
    void do_login(const KYString AIp, DWord AId);
};


// 结束监听
void TServerEvent::dis_conn(TTCPServer* AServer)
{
    printf("over-----------------\n");
}

// 用户登录
void TServerEvent::do_login(const KYString AIp, DWord AId)
{
    printf("a user login sucess ip:%s ,id:%u\n\n", (char*)AIp, AId);
}

// 初始化服务器
void init_server(TServer* AServer)
{
    // 初始化
    void*    objConn;
    long     intValue, intKey;
    KYString strValue;
    TIniFile iniConfig(CurrAppFilePath + File_Config);
    
    // 设置 server 的属性
    strValue = iniConfig.ReadString(Sect_System, Key_IP, "127.0.0.1");
    AServer->set_addr(strValue);
    intValue = iniConfig.ReadInteger(Sect_System, Key_Port, 5000);
    AServer->set_port(intValue);
    intValue = iniConfig.ReadInteger(Sect_System, Key_RecvThreads, 10);
    AServer->set_recv_threads(intValue);

    AServer->set_keep_timeout(INFINITE);
    AServer->set_keep_interval(INFINITE);
    timer = new TKYTimer;
    AServer->set_timer(timer);
    AServer->set_disocnnect((TDoSrvEvent)&TServerEvent::dis_conn, &AServer);
    AServer->set_loign_sucess((TDoLoginSucess)&TServerEvent::do_login, &AServer);
}

// 打开菜单
void show_menu()
{
    printf("------------------------------------------------------\n");
    printf("--Press  Ctrl+C       exit.                        ---\n");
    printf("--Press  O            open server.                 ---\n");
    printf("--Press  C            close server.                ---\n");
    printf("--Press  E            delete a user.               ---\n");
    printf("--Press  Q            query  users.                ---\n");
    printf("--Press  T            menu view                    ---\n");
    printf("------------------------------------------------------\n");
}

// 打开成功 显示信息
void open_sucess(TServer* AServer)
{
    printf("\n-----server open sucess!----server is listening %ld----------\n", AServer->Port());
    show_menu();
}

// 测试删除
void test_delete(TServer* AServer)
{
    printf("\n输入要删除的用户id: ");
    int tmp_id;
    scanf("%d", &tmp_id);
    if (AServer->delete_user(tmp_id))
        printf("删除成功！\n");
    else
        printf("删除失败！\n");
}

// 测试查询
void test_query(TServer* AServer)
{
    DWord begin, end;
    printf("print begin index:");
    scanf("%u", &begin);
    printf("print end index:");
    scanf("%u", &end);
    TKYStringList res_list;
    if (AServer->query_users(begin, end, res_list))
    {
        printf("ip          id\n");
        int cnt = res_list.Count();
        for (int i = 0; i < cnt; i++)
            printf("%-12s%u\n", (char*)res_list.Item(i), (DWord)res_list.Data(i));
    }
    else
        printf("----faild-----\n");
}

// 测试打开
void test_open(TServer* AServer)
{
    if (AServer->open())
        open_sucess(AServer);
    else
        printf("----open faild----\n");
}

// 测试关闭
void test_close(TServer* AServer)
{
    AServer->close(true);
    printf("\n-----server close......--------------\n");
}

void test_server()
{
    int intKey;
    TServer* server = new TServer;
    init_server(server);

    if (server->open())
    {
        // 等待退出
        open_sucess(server);

        for (;;)
        {
            intKey = _getch();
            if (intKey == 0x03)  // Ctrl+C
                break;
            else
            {
                switch (intKey)
                {
                case 'E':  // 测试删除
                case 'e':
                    test_delete(server);
                    break;

                case 'Q':  // 测试查询
                case 'q':
                    test_query(server);
                    break;

                case 'T':  // 打开菜单
                case 't':
                    show_menu();
                    break;

                case 'O':  // 打开
                case 'o':
                    test_open(server);
                    break;

                case 'C':  // 关闭
                case 'c':
                    test_close(server);
                    break;
                }
            }
        }

    }

    delete server;
}

int main()
{
    // 初始化
    InitKYLib(NULL);
    TCPInitialize();
   
    // 测试服务程序
    test_server();

    FreeKYTimer(timer);
    // 释放接口
    TCPUninitialize();

    return 0;
}
