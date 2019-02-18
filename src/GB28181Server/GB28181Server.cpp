#include "GB28181Server.h"

#include <stdio.h>
#include <stdlib.h>

//MD5
#include "MD5/HTTPDigest.h"

#if defined(WIN32)
    #include <winsock2.h>
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#define LOCAL_PORT 5060
#define APP_LOG printf

char *GB_PASSWORD = (char *)"123456";
char *GB_REALM = (char *)"34020000";
char *GB_NONCE = (char *)"6fe9ba44a76be22a";

GB28181Server::GB28181Server()
{

}

void GB28181Server::start()
{
    run();
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


    //�����������Ϣ���������߳�
    //�������ظ��������Ϣ
    while (1)
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

                        if (pAlgorithm == NULL
                                || pUsername == NULL
                                || pRealm  == NULL
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

                                APP_LOG("Camera Register Success!! userName=%s pUri=%s pcMethod=%s Response=%s\n",
                                        pUsername, pUri, pcMethod, Response);

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
                    RegisterSuccess(eCtx, je);
                }
                else if (strncmp(je->request->sip_method, "BYE", 4) != 0)
                {
                    APP_LOG("msg BYE\n");
                }

                break;
            }
            default:
                APP_LOG("##test,%s:%d, unsupport type:%d\n", __FILE__, __LINE__, je->type);
                RegisterSuccess(eCtx, je);
                break;
        }
        eXosip_event_free(je);
    }

    eXosip_quit(eCtx);
    osip_free(eCtx);
    eCtx = NULL;
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
