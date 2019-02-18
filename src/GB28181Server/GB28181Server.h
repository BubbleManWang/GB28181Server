#ifndef GB28181SERVER_H
#define GB28181SERVER_H

#include <stdio.h>
#include <string>
#include <list>

#include <eXosip2/eXosip.h>

///ͨ�����ݣ����������Ҫͨ��catalog���ȡ���ģ������Ͼ��������һ��ͨ��
struct ChannelItem
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

    std::list<ChannelItem> channelList; //һ��ͨ����Ӧһ·��Ƶ����һ����������ж��ͨ��

};


class GB28181Server
{
public:
    GB28181Server();

    void start();

    int SN = 1;
    struct eXosip_t *eCtx;

    std::list<DeviceNode> mDeviceList;

private:
    void run();

    void Register401Unauthorized(struct eXosip_t * peCtx,eXosip_event_t *je);
    void RegisterSuccess(struct eXosip_t * peCtx,eXosip_event_t *je);
    void RegisterFailed(struct eXosip_t * peCtx,eXosip_event_t *je);

    void Response(struct eXosip_t * peCtx,eXosip_event_t *je, int value);
    void Response403(struct eXosip_t * peCtx,eXosip_event_t *je);
    void Response200(struct eXosip_t * peCtx,eXosip_event_t *je); //��200OK

    int SendQueryCatalog(struct eXosip_t *peCtx , DeviceNode deviceNode);

};

#endif // GB28181SERVER_H
