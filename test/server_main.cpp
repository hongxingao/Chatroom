#include <stdio.h>
#include <conio.h>
#include "server.h"
using namespace _ghx;

/* �������� */

// �����ļ���
static const KYString File_Config = "TestServer.ini";
static const KYString File_Output = "TestServer.txt";

// �����ļ��Ľ���
static const KYString Sect_System = "system";

// �����ļ��ļ���
static const KYString Key_IP = "IP";
static const KYString Key_Port = "Port";
static const KYString Key_RecvThreads = "RecvThreads";

// ��ʱ��
static TKYTimer* timer = NULL;

// �������¼�
class TServerEvent
{
public:
    // ��������
    void dis_conn(TTCPServer* AServer);
    // �û���¼
    void do_login(const KYString AIp, DWord AId);
};


// ��������
void TServerEvent::dis_conn(TTCPServer* AServer)
{
    printf("over-----------------\n");
}

// �û���¼
void TServerEvent::do_login(const KYString AIp, DWord AId)
{
    printf("a user login sucess ip:%s ,id:%u\n\n", (char*)AIp, AId);
}

// ��ʼ��������
void init_server(TServer* AServer)
{
    // ��ʼ��
    void*    objConn;
    long     intValue, intKey;
    KYString strValue;
    TIniFile iniConfig(CurrAppFilePath + File_Config);
    
    // ���� server ������
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

// �򿪲˵�
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

// �򿪳ɹ� ��ʾ��Ϣ
void open_sucess(TServer* AServer)
{
    printf("\n-----server open sucess!----server is listening %ld----------\n", AServer->Port());
    show_menu();
}

// ����ɾ��
void test_delete(TServer* AServer)
{
    printf("\n����Ҫɾ�����û�id: ");
    int tmp_id;
    scanf("%d", &tmp_id);
    if (AServer->delete_user(tmp_id))
        printf("ɾ���ɹ���\n");
    else
        printf("ɾ��ʧ�ܣ�\n");
}

// ���Բ�ѯ
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

// ���Դ�
void test_open(TServer* AServer)
{
    if (AServer->open())
        open_sucess(AServer);
    else
        printf("----open faild----\n");
}

// ���Թر�
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
        // �ȴ��˳�
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
                case 'E':  // ����ɾ��
                case 'e':
                    test_delete(server);
                    break;

                case 'Q':  // ���Բ�ѯ
                case 'q':
                    test_query(server);
                    break;

                case 'T':  // �򿪲˵�
                case 't':
                    show_menu();
                    break;

                case 'O':  // ��
                case 'o':
                    test_open(server);
                    break;

                case 'C':  // �ر�
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
    // ��ʼ��
    InitKYLib(NULL);
    TCPInitialize();
   
    // ���Է������
    test_server();

    FreeKYTimer(timer);
    // �ͷŽӿ�
    TCPUninitialize();

    return 0;
}
