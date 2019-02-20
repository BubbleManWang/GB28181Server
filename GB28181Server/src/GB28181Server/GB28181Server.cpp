#include "GB28181Server.h"

#include <stdio.h>
#include <stdlib.h>

//MD5
#include "MD5/HTTPDigest.h"
//xml
#include <mxml.h>

#if defined(WIN32)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#include <QDebug>

#if _MSC_VER
#define snprintf _snprintf   //snprintf �Ҳ�����ʶ��
#endif

//#define APP_LOG printf
#define APP_LOG qDebug

char *GB_NONCE = (char *)"6fe9ba44a76be22a";

#if defined(WIN32)
static DWORD WINAPI thread_Func(LPVOID pM)
#else
static void *thread_Func(void *pM)
#endif
{
    GB28181Server *pointer = (GB28181Server*)pM;

    pointer->run();

    return 0;
}

GB28181Server::GB28181Server()
{
    mIsStop = false;
    mIsThreadRunning = false;
}

void GB28181Server::setLocalIp(char *ip, int port)
{
    strcpy(LOCAL_IP, ip);
    LOCAL_PORT = port;
}

void GB28181Server::setGBServerInfo(char *sipId, char *passwd, char *realm)
{
    strcpy(SERVER_SIP_ID, sipId);
    strcpy(GB_PASSWORD, passwd);
    strcpy(GB_REALM, realm);
}

void GB28181Server::start()
{
    mIsStop = false;
#if defined(WIN32)
     HANDLE handle = CreateThread(NULL, 0, thread_Func, this, 0, NULL);
#else
    pthread_t thread1;
    pthread_create(&thread1,NULL,thread_Func,this);
#endif
}

void GB28181Server::stop()
{
    mIsStop = true;
    while(mIsThreadRunning)
    {
        Sleep(100);
    }
}

///�豸ע��ɹ�
void GB28181Server::deviceRegisted(DeviceNode node)
{

}

///�豸���£�catalog���󷵻ص��豸��Ϣ����
void GB28181Server::deviceUpdate(DeviceNode node)
{

}

///���յ���Ϣ
void GB28181Server::receiveMessage(const char *deviceID, MessageType type, char *msgBody)
{

}

void GB28181Server::run()
{
    int iReturnCode = 0;

    //��ʼ��������Ϣ
    TRACE_INITIALIZE(6, NULL);

    //��ʼ��eXosip��osipջ
    eCtx = eXosip_malloc();
    iReturnCode = eXosip_init(eCtx);
    if (iReturnCode != OSIP_SUCCESS)
    {
        APP_LOG("Can't initialize eXosip!");
        exit(1);
    }
    else
    {
        APP_LOG("eXosip_init successfully!\n");
    }

    //��һ��UDP socket �����ź�
    iReturnCode = eXosip_listen_addr(eCtx, IPPROTO_UDP, NULL, LOCAL_PORT, AF_INET, 0);
    if (iReturnCode != OSIP_SUCCESS)
    {
        APP_LOG("eXosip_listen_addr error!");
        exit(1);
    }

    mIsThreadRunning = true;

    //�����������Ϣ���������߳�
    //�������ظ��������Ϣ
    while (!mIsStop)
    {
        eXosip_event_t *je = NULL;
        //�����¼�
        je = eXosip_event_wait(eCtx, 0, 4);
        if (je == NULL)
        {
            osip_usleep(100000);
            continue;
        }

        switch (je->type)
        {
            case EXOSIP_MESSAGE_NEW:				//����Ϣ����
            {
                APP_LOG("new msg method:%s\n", je->request->sip_method);

                //����ע����Ϣ
                if ( MSG_IS_REGISTER(je->request) )
                {
                    //��ȡ�������ֶ�ֵ,���� MD5 ����
                    osip_authorization_t * Sdest = NULL;
                    osip_message_get_authorization(je->request,0,&Sdest);

                    if ( Sdest == NULL )
                    {   ///ע����Ϣ�в�����Ȩ��Ϣ���򷵻�401 Unauthorized����Ȩ�ޣ���Ӧ
                        Register401Unauthorized(eCtx,je);
                    }
                    else
                    {
                        char *pcMethod = je->request->sip_method;
                        char *pAlgorithm = NULL;

                        if (Sdest->algorithm != NULL)
                        {
                            pAlgorithm = osip_strdup_without_quote(Sdest->algorithm);
                        }
                        char *pUsername = NULL;
                        if ( Sdest->username != NULL )
                        {
                            pUsername = osip_strdup_without_quote(Sdest->username);
                        }
                        char *pRealm = NULL;
                        if ( Sdest->realm != NULL )
                        {
                            pRealm = osip_strdup_without_quote(Sdest->realm);
                        }
                        char *pNonce = NULL;
                        if ( Sdest->nonce != NULL )
                        {
                            pNonce = osip_strdup_without_quote(Sdest->nonce);
                        }
                        char *pNonce_count = NULL;
                        if ( Sdest->nonce_count != NULL)
                        {
                            pNonce_count = osip_strdup_without_quote(Sdest->nonce_count);
                        }

                        char *pUri = NULL;
                        if ( Sdest->uri != NULL )
                        {
                            pUri = osip_strdup_without_quote(Sdest->uri);
                        }

                        ///�ͻ��������response����
                        char *request_response = NULL;
                        if ( Sdest->response != NULL )
                        {
                            request_response = osip_strdup_without_quote(Sdest->response);
                        }

                        receiveMessage(pUsername, MessageType_Register, NULL);

                        if (pAlgorithm == NULL
                                || pUsername == NULL
                                || pRealm == NULL
                                || pNonce == NULL)
                        {
                            RegisterFailed(eCtx,je); //��֤ʧ��
                        }
                        else
                        {
                            ///1.�����response
                            ///2.��ͻ������������response���Ƚϣ����һ������ͨ��ע�ᣬ�ظ�200.����ظ�401ʧ��
                            //��Ҫȥ�����˶��������
                            HASHHEX HA1={0};
                            DigestCalcHA1(pAlgorithm,pUsername,pRealm,GB_PASSWORD,pNonce,
                                          pNonce_count, HA1);

                            HASHHEX Response={0};
                            HASHHEX HA2={0};
                            //����������������棬�Ѿ������� H(A2),���Բ���Ҫ�Լ����� H(A2)
                            DigestCalcResponse(HA1,pNonce,pNonce_count,Sdest->cnonce,Sdest->message_qop,0,
                                               pcMethod,pUri,HA2,Response);

                            if ( memcmp(request_response, Response, HASHHEXLEN) == 0 )
                            {
                                //����ע��ɹ�����Ϣ
                                RegisterSuccess(eCtx, je);

                                osip_via_t * s_via = NULL;
                                osip_message_get_via(je->request,0,&s_via);
                                char * ip = osip_via_get_host(s_via);
                                char* port = osip_via_get_port(s_via);

                                DeviceNode node;
                                node.DeviceID = pUsername;
                                node.IPAddress = ip;
                                node.Port = atoi(port);
                                mDeviceList.push_back(node);

                                deviceRegisted(node);

                                APP_LOG("Ip = %s %s \n", ip, port);
                                APP_LOG("Camera Register Success!! userName=%s pUri=%s pcMethod=%s Response=%s\n",
                                        pUsername, pUri, pcMethod, Response);

                                SendQueryCatalog(eCtx, node);
                            }
                            else  //��֤ʧ��
                            {
                                RegisterFailed(eCtx,je);

                                APP_LOG("Camera Register Failed!! userName=%s pUri=%s \n",
                                        pUsername, pUri);

                            }

                            if (pAlgorithm != NULL)
                                osip_free(pAlgorithm);
                            if (pUsername != NULL)
                                osip_free(pUsername);
                            if (pRealm != NULL)
                                osip_free(pRealm);
                            if (pNonce != NULL)
                                osip_free(pNonce);
                            if (pNonce_count != NULL)
                                osip_free(pNonce_count);
                            if (pUri != NULL)
                                osip_free(pUri);
                        }

                    }
                }
                else if (MSG_IS_MESSAGE(je->request))
                {
                    osip_body_t *body = NULL;
                    osip_message_get_body(je->request, 0, &body);
                    if (body != NULL)
                    {
                        APP_LOG("msg body is:%s\n", body->body);

                        //��body->body�м���xml
                        mxml_node_t *xml = mxmlLoadString(NULL,body->body,MXML_NO_CALLBACK);

                        //��xml��ʼ���²��� name=CmdType attrr=""
                        mxml_node_t * CmdTypeNode = mxmlFindElement(xml, xml,"CmdType",NULL,NULL,MXML_DESCEND);

                        if(CmdTypeNode != NULL)
                        {
                            osip_via_t * s_via = NULL;
                            osip_message_get_via(je->request,0,&s_via);
                            char* ip = osip_via_get_host(s_via);
                            char* port = osip_via_get_port(s_via);

                            const char *CmdType = mxmlGetText(CmdTypeNode, NULL);

                            APP_LOG("CmdType=%s \n", CmdType);

                            ///������
                            if ( 0 == strcmp(CmdType, "Keepalive"))
                            {

                                bool isDeviceExist = false;

                                mxml_node_t * DeviceIDNode = mxmlFindElement(xml,xml,"DeviceID",NULL,NULL,MXML_DESCEND);

                                if(DeviceIDNode != NULL)
                                {
                                    const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);

                                    APP_LOG("Recv Keepalive DeviceId is:%s\n", DeviceID);

                                    std::list<DeviceNode>::iterator iter;

                                    for(iter = mDeviceList.begin(); iter != mDeviceList.end() ;iter++)
                                    {
                                        DeviceNode node = *iter;
                                        if (node.DeviceID.compare(DeviceID) == 0 && node.IPAddress.compare(ip) == 0)
                                        {
                                            isDeviceExist = true;
                                            break;
                                        }
                                    }

                                    receiveMessage(DeviceID, MessageType_KeepAlive, body->body);
                                }

                                if (isDeviceExist)
                                {
                                    Response200(eCtx, je);
                                }
                                else
                                {
                                    Response403(eCtx, je); //�豸û�м�¼�����403�����豸���·���ע��
                                }
                            }
                            else if ( 0 == strcmp(CmdType, "Catalog"))
                            {
                                Response200(eCtx, je);

                                mxml_node_t * DeviceIDNode = mxmlFindElement(xml,xml,"DeviceID",NULL,NULL,MXML_DESCEND);

                                if(DeviceIDNode != NULL)
                                {
                                    const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);

                                    receiveMessage(DeviceID, MessageType_Catalog, body->body);
                                }

                                mxml_node_t * DeviceListNode = mxmlFindElement(xml,xml,"DeviceList",NULL,NULL,MXML_DESCEND);

                                if(DeviceListNode != NULL)
                                {
                                    int count = mxmlGetRefCount(DeviceListNode);

                                    for (int i=0; i<count; i++)
                                    {
                                        mxml_node_t * OneItem = mxmlGetNextSibling(DeviceListNode);

                                        if (OneItem == NULL) break;

                                        mxml_node_t * ItemNode = mxmlFindElement(DeviceListNode, DeviceListNode,"Item",NULL,NULL,MXML_DESCEND);

                                        if(ItemNode != NULL)
                                        {
                                            mxml_node_t * DeviceIDNode = mxmlFindElement(ItemNode, ItemNode,"DeviceID",NULL,NULL,MXML_DESCEND);
                                            mxml_node_t * ParentIDNode = mxmlFindElement(ItemNode, ItemNode,"ParentID",NULL,NULL,MXML_DESCEND);
                                            mxml_node_t * IPAddressNode = mxmlFindElement(ItemNode, ItemNode,"IPAddress",NULL,NULL,MXML_DESCEND);
                                            mxml_node_t * PortNode = mxmlFindElement(ItemNode, ItemNode,"Port",NULL,NULL,MXML_DESCEND);
                                            mxml_node_t * StatusNode = mxmlFindElement(ItemNode, ItemNode,"Status",NULL,NULL,MXML_DESCEND);

                                            const char *DeviceID = mxmlGetText(DeviceIDNode, NULL);
                                            const char *ParentID = mxmlGetText(ParentIDNode, NULL);
                                            const char *IPAddress = mxmlGetText(IPAddressNode, NULL);
                                            const char *Port = mxmlGetText(PortNode, NULL);
                                            const char *Status = mxmlGetText(StatusNode, NULL);

                                            APP_LOG("Recv Catalog DeviceInfo is:%s %s %s %s\n", DeviceID, IPAddress, Port, Status);

                                            std::list<DeviceNode> deviceListTmp;

                                            std::list<DeviceNode>::iterator iter;
                                            for(iter = mDeviceList.begin(); iter != mDeviceList.end() ;iter++)
                                            {
                                                DeviceNode deviceNode = *iter;
                                                if (deviceNode.DeviceID.compare(ParentID) == 0 && deviceNode.IPAddress.compare(ip) == 0)
                                                {
                                                    bool isExist = false;

                                                    std::list<ChannelNode>::iterator iter;
                                                    for(iter = deviceNode.channelList.begin(); iter != deviceNode.channelList.end() ;iter++)
                                                    {
                                                        ChannelNode cameraNode = *iter;
                                                        if (cameraNode.DeviceID.compare(DeviceID) == 0 && cameraNode.IPAddress.compare(ip) == 0)
                                                        {
                                                            isExist = true;
                                                            break;
                                                        }
                                                    }

                                                    if (!isExist)
                                                    {
                                                        ChannelNode item;
                                                        item.DeviceID = DeviceID;
                                                        item.IPAddress = IPAddress;
                                                        item.Port = atoi(Port);
                                                        item.Status = Status;

                                                        deviceNode.channelList.push_back(item);
                                                    }

                                                    deviceUpdate(deviceNode);
//                                                    break; //��Ҫ�˳�����ʣ�µ��豸Ҳ�����µ��б�
                                                }

                                                deviceListTmp.push_back(deviceNode);
                                            }

                                            mDeviceList = deviceListTmp;

                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        // APP_ERR("get body failed");
                    }
                }
                else if (strncmp(je->request->sip_method, "BYE", 4) != 0)
                {
                    Response200(eCtx, je);
                    APP_LOG("msg BYE\n");
                }

                break;
            }
            case EXOSIP_MESSAGE_ANSWERED:			//��ѯ
            {
                APP_LOG("msg EXOSIP_MESSAGE_ANSWERED\n");

                osip_body_t *body = NULL;
                osip_message_get_body(je->request, 0, &body);
                if (body != NULL)
                {
                    APP_LOG("msg body is:%s\n", body->body);
                }

                Response200(eCtx, je);

                break;
            }
            default:
                APP_LOG("##test,%s:%d, unsupport type:%d\n", __FILE__, __LINE__, je->type);
                Response200(eCtx, je);
                break;
        }
        eXosip_event_free(je);
    }

    eXosip_quit(eCtx);
    osip_free(eCtx);
    eCtx = NULL;

    APP_LOG("Finished!\n");

    mIsThreadRunning = false;
}

void GB28181Server::Register401Unauthorized(struct eXosip_t * peCtx,eXosip_event_t *je)
{
    ///����401 Unauthorized����Ȩ�ޣ���Ӧ,����Ҫ���UAC�����û���֤������ͨ��WWW-Authenticate�ֶ�Я��UAS֧�ֵ���֤��ʽ������������֤��nonce
    int iReturnCode = 0;
    osip_message_t * pSRegister = NULL;

    osip_www_authenticate_t * header = NULL;
    osip_www_authenticate_init(&header);

    if (header == NULL) return;

    osip_www_authenticate_set_auth_type (header,osip_strdup("Digest"));
    osip_www_authenticate_set_realm(header,osip_enquote(GB_REALM));
    osip_www_authenticate_set_nonce(header,osip_enquote(GB_NONCE));

    char *pDest = NULL;
    osip_www_authenticate_to_str(header,&pDest);

    if (pDest == NULL) return;

    iReturnCode = eXosip_message_build_answer (peCtx,je->tid,401,&pSRegister);
    if ( iReturnCode == 0 && pSRegister != NULL )
    {
        osip_message_set_www_authenticate(pSRegister,pDest);
        osip_message_set_content_type(pSRegister,"Application/MANSCDP+xml");
        eXosip_lock(peCtx);
        eXosip_message_send_answer (peCtx,je->tid,401,pSRegister);
        eXosip_unlock(peCtx);
    }

    osip_www_authenticate_free(header);
    osip_free(pDest);
}

void GB28181Server::RegisterSuccess(struct eXosip_t * peCtx,eXosip_event_t *je)
{
    int iReturnCode = 0 ;
    osip_message_t * pSRegister = NULL;
    iReturnCode = eXosip_message_build_answer (peCtx,je->tid,200,&pSRegister);

    char timeCh[32] = {0};

#if defined(WIN32)
    SYSTEMTIME sys;
    GetLocalTime( &sys );

    sprintf(timeCh,"%d-%02d-%02dT%02d:%02d:%02d.%03d",
            sys.wYear, sys.wMonth, sys.wDay, sys.wHour, sys.wMinute, sys.wSecond, sys.wMilliseconds);

#else
    struct timeval    tv;
    struct timezone tz;

    struct tm         *p;

    gettimeofday(&tv, &tz);
    p = localtime(&tv.tv_sec);


    sprintf(timeCh,"%d-%02d-%02dT%02d:%02d:%02d.%03d",
            1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec/1000);

#endif

//    osip_message_set_topheader (pSRegister, "date", "2018-04-25T21:42:40.075");
    osip_message_set_topheader (pSRegister, "date", timeCh);

    if ( iReturnCode == 0 && pSRegister != NULL )
    {
        eXosip_lock(peCtx);
        eXosip_message_send_answer (peCtx,je->tid,200,pSRegister);
        eXosip_unlock(peCtx);
    }
}

void GB28181Server::RegisterFailed(struct eXosip_t * peCtx,eXosip_event_t *je)
{
    int iReturnCode = 0 ;
    osip_message_t * pSRegister = NULL;
    iReturnCode = eXosip_message_build_answer (peCtx,je->tid, 401, &pSRegister);
    if ( iReturnCode == 0 && pSRegister != NULL )
    {
        eXosip_lock(peCtx);
        eXosip_message_send_answer (peCtx,je->tid,401, pSRegister);
        eXosip_unlock(peCtx);
    }
}

void GB28181Server::Response(struct eXosip_t * peCtx,eXosip_event_t *je, int value)
{
    int iReturnCode = 0 ;
    osip_message_t * pSRegister = NULL;
    iReturnCode = eXosip_message_build_answer (peCtx,je->tid, value, &pSRegister);

    if ( iReturnCode == 0 && pSRegister != NULL )
    {
        eXosip_lock(peCtx);
        eXosip_message_send_answer (peCtx,je->tid,value,pSRegister);
        eXosip_unlock(peCtx);
    }
}

//��403
void GB28181Server::Response403(struct eXosip_t * peCtx,eXosip_event_t *je)
{
    Response(peCtx, je, 403);
}

void GB28181Server::Response200(struct eXosip_t * peCtx,eXosip_event_t *je)
{
    Response(peCtx, je, 200);
}

static const char *whitespace_cb(mxml_node_t *node, int where)
{
    return NULL;
}

void GB28181Server::doSendCatalog(DeviceNode node)
{
    SendQueryCatalog(eCtx, node);
}

//��������catalog��Ϣ
int GB28181Server::SendQueryCatalog(struct eXosip_t *peCtx, DeviceNode deviceNode)
{
    fprintf(stderr,"sendQueryCatalog\n");

    char sn[32] = {0};
    int ret;
    mxml_node_t *tree, *query, *node;

    const char *deviceId = deviceNode.DeviceID.c_str();
    const char *platformSipId = SERVER_SIP_ID;

    const char *platformIpAddr= deviceNode.IPAddress.c_str();
    int platformSipPort = deviceNode.Port;

    const char *localSipId = SERVER_SIP_ID;
    const char *localIpAddr= LOCAL_IP;

    tree = mxmlNewXML("1.0");
    if (tree != NULL)
    {
        query = mxmlNewElement(tree, "Query");
        if (query != NULL)
        {
            char buf[256] = { 0 };
            char dest_call[256], source_call[256];
            node = mxmlNewElement(query, "CmdType");
            mxmlNewText(node, 0, "Catalog");
            node = mxmlNewElement(query, "SN");
            snprintf(sn, 32, "%d", SN++);
            mxmlNewText(node, 0, sn);
            node = mxmlNewElement(query, "DeviceID");
            mxmlNewText(node, 0, deviceId);
            mxmlSaveString(tree, buf, 256, whitespace_cb);

            osip_message_t *message = NULL;
            snprintf(dest_call, 256, "sip:%s@%s:%d", platformSipId, platformIpAddr, platformSipPort);
            snprintf(source_call, 256, "sip:%s@%s:%d", localSipId, localIpAddr, LOCAL_PORT);
            ret = eXosip_message_build_request(peCtx, &message, "MESSAGE", dest_call, source_call, NULL);
            if (ret == 0 && message != NULL)
            {
                osip_message_set_body(message, buf, strlen(buf));
                osip_message_set_content_type(message, "Application/MANSCDP");
                eXosip_lock(peCtx);
                eXosip_message_send_request(peCtx, message);
                eXosip_unlock(peCtx);
                APP_LOG("xml:%s, dest_call:%s, source_call:%s, ok", buf, dest_call, source_call);
            }
            else
            {
                APP_LOG("eXosip_message_build_request failed!\n");
            }
        }
        else
        {
            APP_LOG("mxmlNewElement Query failed!\n");
        }
        mxmlDelete(tree);
    }
    else
    {
        APP_LOG("mxmlNewXML failed!\n");
    }

    return 0;
}
