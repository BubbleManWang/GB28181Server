#ifndef GB28181SERVER_H
#define GB28181SERVER_H

#include <stdio.h>
#include <string>
#include <list>

#include <eXosip2/eXosip.h>

///ͨ�����ݣ����������Ҫͨ��catalog���ȡ���ģ������Ͼ��������һ��ͨ��
struct ChannelNode
{
    std::string DeviceID;
    std::string IPAddress;
    int Port;
    std::string Status;
};

///�豸���ݣ�ͨ��ע�������ȡ������Ϣ
struct DeviceNode
{
    std::string DeviceID;
    std::string IPAddress;
    int Port;

    std::list<ChannelNode> channelList; //һ��ͨ����Ӧһ·��Ƶ����һ����������ж��ͨ��

    bool operator == (DeviceNode node)//��������������ľ���ʵ��
    {
        bool isSame = false;

        if ((node.DeviceID.compare(this->DeviceID) == 0) && (node.IPAddress.compare(this->IPAddress) == 0))
        {
            isSame = true;
        }

        return isSame;
    }

};

enum MessageType
{
    MessageType_Register = 0,
    MessageType_KeepAlive,
    MessageType_Catalog,
    MessageType_CallAnswer,
    MessageType_CallFailed
};

class GB28181Server
{
public:
    GB28181Server();

    void setLocalIp(char *ip, int port);
    void setGBServerInfo(char *sipId, char *passwd, char *realm);

    void start();
    void stop();

    void doSendCatalog(DeviceNode node);
    void doSendInvitePlay(ChannelNode node);

    std::list<DeviceNode> getDeviceList(){return mDeviceList;}

    void run(); //�߳�ִ�к���

protected:
    virtual void deviceRegisted(DeviceNode node); //�豸ע��ɹ�
    virtual void deviceUpdate(DeviceNode node);   //�豸���£�catalog���󷵻ص��豸��Ϣ����
    virtual void receiveMessage(const char *deviceID, MessageType type, char *msgBody);  //���յ���Ϣ

private:
    char SERVER_SIP_ID[256];
    char GB_PASSWORD[36];
    char GB_REALM[36];

    char LOCAL_IP[36];
    int LOCAL_PORT;

    int SN = 1;
    struct eXosip_t *eCtx;

    bool mIsStop;          //�Ƿ��յ�ֹͣ����
    bool mIsThreadRunning; //�����Ƿ�������

    std::list<DeviceNode> mDeviceList;

    void Register401Unauthorized(struct eXosip_t * peCtx,eXosip_event_t *je);
    void RegisterSuccess(struct eXosip_t * peCtx,eXosip_event_t *je);
    void RegisterFailed(struct eXosip_t * peCtx,eXosip_event_t *je);

    void Response(struct eXosip_t * peCtx,eXosip_event_t *je, int value); //�����������
    void Response403(struct eXosip_t * peCtx,eXosip_event_t *je); //��403
    void Response200(struct eXosip_t * peCtx,eXosip_event_t *je); //��200OK

    void ResponseCallAck(struct eXosip_t * peCtx, eXosip_event_t *je);

    int SendQueryCatalog(struct eXosip_t *peCtx , DeviceNode deviceNode); //�����豸Ŀ¼

    //������Ƶ��Ϣ��SDP��Ϣ
    int SendInvitePlay(struct eXosip_t *peCtx, ChannelNode cameraNode);

};

#endif // GB28181SERVER_H
