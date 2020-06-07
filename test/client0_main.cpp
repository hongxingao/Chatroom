#include <stdio.h>
#include <conio.h>
#include "client0.h"
using namespace _ghx;

// �����ļ���
static const KYString File_Config = "TestClient.ini";

// �����ļ��Ľ���
static const KYString Sect_System = "system";
// �����ļ��ļ���
static const KYString Id = "ID";
static const KYString Key_IP = "IP";
static const KYString Key_Port = "Port";
static const KYString Key_BindPort = "BindPort";
static const KYString Key_Timeout = "Timeout";
static const KYString Key_KeepTimeout = "KeepTimeout";

// �¼���
class TEvent
{
public:
    void rec_private_msg(Longword ASourId, const KYString& AData);
    void rec_broadcast_msg(Longword ASourId, const KYString& AData);
    void dis_conn();
};

// �յ�˽��
void TEvent::rec_private_msg(Longword ASourId, const KYString& AData)
{
    printf("rec private msg from:%ld,size:%ld\n", ASourId, AData.Length());
}

// �յ��㲥
void TEvent::rec_broadcast_msg(Longword ASourId, const KYString& AData)
{
    printf("rec broadcast msg from:%ld,size:%ld\n", ASourId, AData.Length());
}

// �Ͽ�����
void TEvent::dis_conn()
{
    printf("\n---------disconnect------------------\n");
}

// ��ʼ���ͻ���
void init_client(TClient* AClient)
{
    long     intValue, intKey;
    KYString strValue;
    TIniFile iniConfig(CurrAppFilePath + File_Config);

    // ��������
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

// ��ʾ�˵�
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


// �򿪳ɹ� ��ӡ���
void open_sucess(TClient* AClient)
{
    printf("\n---------open sucess-------your id:%u\n", AClient->Id());
    show_menu();
}

// ����˽�Ź���
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

// ���Թ㲥
void test_broadcast(TClient* AClient)
{
    char data[128];
    DWord id;
    printf("print data:");
    scanf("%s", data);
    DWord count = AClient->broadcast(data);
    printf("broadcast %u users\n", count);
}

// ���Բ�ѯ
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

// ���Դ�
void test_open(TClient* AClient)
{
    if (AClient->open() == TU_trSuccess)
        open_sucess(AClient);
    else
        printf("---open---------failed\n");
}

// ���Կͻ���
void test_client()
{
    TClient* client = new TClient;
    
    init_client(client);
    if (client->open() == TU_trSuccess)
    {
        long intKey;
        // �ȴ��˳�
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
                case 'S':   // ����˽��
                case 's':
                    test_pivate_msg(client);
                    break;

                case 'B':   // ���Թ㲥
                case 'b':
                    test_broadcast(client);
                    break;

                case 'Q':  // ���Բ�ѯ
                case 'q':
                    test_query(client);
                    break;

                case 'T':
                case 't':  // �򿪲˵�
                    show_menu();
                    break;

                case 'O':  // ��
                case 'o':
                    test_open(client);
                    break;

                case 'C': // �ر�
                case 'c':
                    client->close(true);
                    break;
                }
            }
        }
        
    }

    // �ͷ�
    delete client;

}

int main()
{
    // ��ʼ��
    InitKYLib(NULL);
    TCPInitialize();

    // ���Կͻ���
    test_client();

    // �ͷŽӿ�
    TCPUninitialize();

    return 0;
}