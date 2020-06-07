#include <stdio.h>
#include <conio.h>
#include "client0.h"
using namespace _ghx;

// 配置文件名
static const KYString File_Config = "TestClient.ini";

// 配置文件的节名
static const KYString Sect_System = "system";
// 配置文件的键名
static const KYString Id = "ID";
static const KYString Key_IP = "IP";
static const KYString Key_Port = "Port";
static const KYString Key_BindPort = "BindPort";
static const KYString Key_Timeout = "Timeout";
static const KYString Key_KeepTimeout = "KeepTimeout";

// 事件类
class TEvent
{
public:
    void rec_private_msg(Longword ASourId, const KYString& AData);
    void rec_broadcast_msg(Longword ASourId, const KYString& AData);
    void dis_conn();
};

// 收到私信
void TEvent::rec_private_msg(Longword ASourId, const KYString& AData)
{
    printf("rec private msg from:%ld,size:%ld\n", ASourId, AData.Length());
}

// 收到广播
void TEvent::rec_broadcast_msg(Longword ASourId, const KYString& AData)
{
    printf("rec broadcast msg from:%ld,size:%ld\n", ASourId, AData.Length());
}

// 断开连接
void TEvent::dis_conn()
{
    printf("\n---------disconnect------------------\n");
}

// 初始化客户端
void init_client(TClient* AClient)
{
    long     intValue, intKey;
    KYString strValue;
    TIniFile iniConfig(CurrAppFilePath + File_Config);

    // 设置属性
    strValue = iniConfig.ReadString(Sect_System, Key_IP, "127.0.0.1");
    AClient->set_peer_addr(strValue);
    intValue = iniConfig.ReadInteger(Sect_System, Key_Port, 5000);
    AClient->set_peer_port(intValue);
    intValue = iniConfig.ReadInteger(Sect_System, Key_KeepTimeout, 300000);
    AClient->set_keep_timeout(intValue);
    intValue = iniConfig.ReadInteger(Sect_System, Id, 1111);
    AClient->set_id(intValue);
    intValue = iniConfig.ReadInteger(Sect_System, Key_BindPort, 8888);
    AClient->set_curr_port(intValue);

    AClient->set_on_recv_private((TDORecvMsg)&TEvent::rec_private_msg, NULL);
    AClient->set_on_recv_broadcast((TDORecvMsg)&TEvent::rec_broadcast_msg, NULL);
    AClient->on_dis_conn.Method = (TDODisConn)&TEvent::dis_conn;
}

// 显示菜单
void show_menu()
{
    printf("------------------------------------------------------\n");
    printf("--Press  Ctrl+C       exit.                        ---\n");
    printf("--Press  O            open client.                 ---\n");
    printf("--Press  C            close client.                ---\n");
    printf("--Press  S            send private msg to a client.---\n");
    printf("--Press  Q            query users.                 ---\n");
    printf("--Press  B            broadcast msg.               ---\n");
    printf("--Press  T            menu view                    ---\n");
    printf("------------------------------------------------------\n");
}


// 打开成功 打印结果
void open_sucess(TClient* AClient)
{
    printf("\n---------open sucess-------your id:%u\n", AClient->Id());
    show_menu();
}

// 测试私信功能
void test_pivate_msg(TClient* AClient)
{
    char data[128];
    DWord id;
    printf("print id:");
    scanf("%d", &id);
    printf("print data:");
    scanf("%s", data);

    if (id == AClient->Id())
        printf("you talk to yourself ???????\n");
    else if (AClient->private_chat(id, data) == TU_trSuccess)
        printf("send sucess\n");
    else
        printf("failed\n");
}

// 测试广播
void test_broadcast(TClient* AClient)
{
    char data[128];
    DWord id;
    printf("print data:");
    scanf("%s", data);
    DWord count = AClient->broadcast(data);
    printf("broadcast %u users\n", count);
}

// 测试查询
void test_query(TClient* AClient)
{
    DWord begin, end;
    printf("print begin index:");
    scanf("%u", &begin);
    printf("print end index:");
    scanf("%u", &end);
    TKYList res_list;

    if (AClient->query(begin, end, res_list) && res_list.Count() > 0)
    {
        printf("%ld users!\n", res_list.Count());
        int cnt = res_list.Count();
        for (int i = 0; i < cnt; i++)
            printf("id :%u\n", (DWord)res_list.Item(i));
    }
    else
        printf("failed\n");
}

// 测试打开
void test_open(TClient* AClient)
{
    if (AClient->open() == TU_trSuccess)
        open_sucess(AClient);
    else
        printf("---open---------failed\n");
}

// 测试客户端
void test_client()
{
    TClient* client = new TClient;
    
    init_client(client);
    if (client->open() == TU_trSuccess)
    {
        long intKey;
        // 等待退出
        open_sucess(client);
        for (;;)
        {
            intKey = _getch();
            if (intKey == 0x03)  // Ctrl+C
                break;
            else
            {
                switch (intKey)
                {
                case 'S':   // 测试私信
                case 's':
                    test_pivate_msg(client);
                    break;

                case 'B':   // 测试广播
                case 'b':
                    test_broadcast(client);
                    break;

                case 'Q':  // 测试查询
                case 'q':
                    test_query(client);
                    break;

                case 'T':
                case 't':  // 打开菜单
                    show_menu();
                    break;

                case 'O':  // 打开
                case 'o':
                    test_open(client);
                    break;

                case 'C': // 关闭
                case 'c':
                    client->close(true);
                    break;
                }
            }
        }
        
    }

    // 释放
    delete client;

}

int main()
{
    // 初始化
    InitKYLib(NULL);
    TCPInitialize();

    // 测试客户端
    test_client();

    // 释放接口
    TCPUninitialize();

    return 0;
}