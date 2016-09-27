
#define _XOPEN_SOURCE
#define _BSD_SOURCE
#include <stdlib.h>
#include <poll.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/socket.h>
#include <bitlbee.h>
#include <ssl_client.h>

#define ANDROIDSMS_DEFAULT_SERVER "192.168.2.162"
#define ANDROIDSMS_DEFAULT_PORT "8888"
#define ANDROIDSMS_DEFAULT_SCRIPT "/home/impulse/src/scripts/inc/android_sms_message"


struct sms_data {
    int socket_desc;
    int udp_socket;
    int lock_buddy;
    gint connectionevent;
    gint acceptevent;
};

static void setup_udp(struct sms_data*, account_t *acc );
static gboolean udp_receive(gpointer data, gint fd, b_input_condition cond);
static gboolean connection_accept(gpointer data, gint fd, b_input_condition cond);
static gboolean connection_receive(gpointer data, gint fd, b_input_condition cond);

void handle_sentnotification(struct im_connection *ic, char* messageline);

const char MSG_REGISTER = 'R';
const char MSG_SENT = 'S';
const char MSG_DELIVERED = 'D';
const char MSG_CONTACTS = 'X';

static void androidsms_login(account_t *acc)
{
    struct im_connection *ic = imcb_new(acc);
    struct sms_data *sd = g_new0(struct sms_data, 1);
    imcb_log(ic,"LOGGING IN....DOING NOTHERING FUCK");

#ifndef NEVER
    ic->proto_data = sd;

    imcb_log(ic, "Connecting");

    printf("SMSLISTENER\n");
    //imcb_log(ic,"SMSListener starting..");

    int client_sock , c , read_size;
    struct sockaddr_in server , client;

    sd->socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    if (sd->socket_desc == -1)
    {
        printf("Could not create socket");
        //imcb_log(ic,"Could not create socket");
    }
    //puts("Socket created");

    //imcb_log(ic,"Preparing socket...");
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( set_getint(&acc->set, "port") );

    //Bind
    //imcb_log(ic,"Binding...");
    printf("bind TCP\n");
    if( bind(sd->socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        imcb_log(ic,"Binding failed");
        close(sd->socket_desc);
        return;
    }
    //puts("bind done");

    //Listen
    listen(sd->socket_desc , 3);
    //imcb_log(ic,"Listening...");

    //Accept and incoming connection
    sd->acceptevent = b_input_add(sd->socket_desc,B_EV_IO_READ,(b_event_handler)connection_accept,ic);
    imcb_connected(ic);
    imcb_log(ic,"Connected");
    ic->flags &= ~(OPT_PONGS);
#ifdef never
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *phoneserver;

    //portno=8888;
    portno=set_getint(&acc->set, "port");
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    phoneserver=gethostbyname( set_getstr(&acc->set, "server") );
    bzero((char *) &serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    bcopy((char *)phoneserver->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            phoneserver->h_length);
    serv_addr.sin_port=htons(portno);
    printf("sending contact request  (init) to %s\n", set_getstr(&acc->set, "server"));

    /*
       struct timeval timeout;
       timeout.tv_sec=2;
       timeout.tv_usec=0;
       if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO, (char*)&timeout,sizeof(timeout))<0)
       error("socket option set failed\n");
       */
    int retval;

    connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

    printf("connected %i\n",retval);
    char* contactquery= "sendcontacts\n";
    write(sockfd,contactquery,strlen(contactquery));
    close(sockfd);
    imcb_log(ic, "sent contact request (init)");
#endif
#endif
    imcb_log(ic, "UDP setup");
    setup_udp(sd,acc);

    return;
}

static int androidsms_buddy_msg(struct im_connection *ic, char *who, char *message,
        int flags)
{
    char *ptr, *nick;
    printf("\nsending message...\n");
    account_t *acc;
    acc=ic->acc;
    int st;

    //construct message
    char* androidmessage;
    androidmessage=g_malloc(sizeof(char)*(strlen(who)+strlen(message)+3));
    memcpy(androidmessage,who,strlen(who));
    memcpy(androidmessage+strlen(who),"::",2);
    memcpy(androidmessage+strlen(who)+2,message,strlen(message));
    androidmessage[strlen(who)+strlen(message)+2]='\0';

    //imcb_buddy_msg(ic, "skypeconsole", androidmessage, 0, 0);

    //imcb_log(ic, androidmessage);

    //send message
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    //portno=8888;
    portno=set_getint(&acc->set, "port");
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    //server=gethostbyname("192.168.1.115");
    server=gethostbyname( set_getstr(&acc->set, "server") );
    bzero((char *) &serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port=htons(portno);

           struct timeval timeout;
           timeout.tv_sec=2;
           timeout.tv_usec=0;
           if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");
           if (setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");

    int retval;
    retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    printf("Connect retval: %d\n",retval);
    printf("Errno: %d\n",errno);
    printf("who: %s\n",who);
    if (retval>=0) 
        write(sockfd,androidmessage,strlen(androidmessage));
    else
    {
        int bufsize;
        char *errmessage_fmt="/me [%c%d%s%c]: %s";
        char *errmessage;
        char *reason="Unable to connect";
        int color=4;
        bufsize=strlen(reason)+strlen(errmessage_fmt)+strlen(message);
        errmessage=g_malloc(bufsize);//ZZ
        g_snprintf(errmessage,bufsize,errmessage_fmt,0x03,color,reason,0x03,message);

        printf("MSMSMS:  %s\n",errmessage);
        imcb_buddy_msg(ic,who,errmessage,0,0);
    }

    printf("gfree2\n");
    g_free(androidmessage);
    printf("gfree2o\n");
    close(sockfd);


    /* Unused parameter */
    flags = flags;

    nick = g_strdup(who);
    ptr = strchr(nick, '@');
    if (ptr)
        *ptr = '\0';

    //if (!strncmp(who, "skypeconsole", 12))
    //st = skype_printf(ic, "%s\n", message);
    //else
    //st = skype_printf(ic, "MESSAGE %s %s\n", nick, message);
    printf("gfree3\n");
    g_free(nick);
    printf("gfree3o\n");
    printf("message sent(?)\n");

    return st;
}

void format_phonenumber(char* number)
{
    //printf("format number\n");
    int off=0;
    int len;
    len=strlen(number);
    int i=0;
    while(!(isdigit(number[i]) && number[i]!='1') && i<len)
        i++;

    for (i=i;i<len;i++)
        if (isdigit(number[i]))
            number[off++]=number[i];
    number[off]='\0';
    //printf("number[%s]\n",number);
}

void format_phonenumber_old(char* number)
{
    int off=0;
    int len;
    len=strlen(number);
    int i=0;
    while(!(isdigit(number[i]) && number[i]!='1') && i<len)
        i++;

    for (i=i;i<len;i++)
        if (isdigit(number[i]))
            number[off++]=number[i];
    number[off]='\0';
}

//Handle getting contact list from phone
void handle_contact(struct im_connection *ic, char* contactline)
{
    struct sms_data *sd=ic->proto_data;
    char *sub;
    char *number;
    char *namea;
    int len;
    if ((sub=strstr(contactline,"::"))!=NULL)
    {
        number=(char*)g_malloc(sizeof(char)*(sub-contactline)+100); 
        if ((strstr(contactline,";"))!=NULL){
            len=strstr(contactline,";")-sub;
            //printf("   LEN: %d\n",len);
            namea=g_malloc(sizeof(char)*(len+1));
            //printf("MALLOC6: %x size: %d\n",namea,len+1);
            memcpy(number,contactline+1,sub-contactline-1);
            //printf("   copy: %d\n",sub-contactline-1);
            //printf("   number: %s\n",number);
            memcpy(namea, sub+2, len-2);//qq added +2 nonsense (x2)
            namea[len-2]='\0';
            //printf("   namea: %s\n",namea);

            //printf("   sub-contactline: %d\n",sub-contactline);
            number[sub-contactline-1]='\0';
            format_phonenumber(number);
            //printf("LAGAGLAG\n");
            //imcb_buddy_msg(ic, "skypeconsole", number, 0, 0);
            //imcb_buddy_msg(ic, "skypeconsole", namea, 0, 0);
            //imcb_buddy_msg(ic, "skypeconsole", "   ", 0, 0);
            len=strlen(number);
            if (len>10)
                //printf("**WARNING**:  abnormal length!  %d\n",len);
                while(sd->lock_buddy>0)
                {

                }
            sd->lock_buddy=1;

            printf("   start add buddy: [%s] [%s] \n", number,namea);
            imcb_add_buddy(ic, number, NULL);
            //printf("ADDED\n");
            imcb_buddy_nick_hint(ic,number,namea);//qq used to be namea instead of asdf
            imcb_buddy_status(ic,number,BEE_USER_ONLINE,NULL,NULL);
            //printf(" done add buddy: %s\n", number);
            sd->lock_buddy=0;
            //printf("FREE\n");
    printf("gfree4\n");
            g_free(namea);
    printf("gfree4o\n");
        }
    printf("gfree5\n");
        g_free(number);
    printf("gfree5o\n");
    }
}

static void androidsms_add_buddy(struct im_connection *ic, char *who, char *group)
{
    struct sms_data *sd = ic->proto_data;
    char *nick, *ptr;

    nick = g_strdup(who);
    ptr = strchr(nick, '@');
    if (ptr)
        *ptr = '\0';

    //ZZ
    if (!group) {
        //skype_printf(ic, "SET USER %s BUDDYSTATUS 2 Please authorize me\n",
        //		nick);
    printf("gfree6\n");
        g_free(nick);
    printf("gfree6o\n");
    } else {
        //struct skype_group *sg = skype_group_by_name(ic, group);

        //if (!sg) {
        //skype_printf(ic, "CREATE GROUP %s", group);
        //sd->pending_user = g_strdup(nick);
        //} else {
        //skype_printf(ic, "ALTER GROUP %d ADDUSER %s", sg->id, nick);
        //}
    }

}

void handle_message(struct im_connection *ic, char* messageline)
{
    char *sub;
    //imcb_chat_log();
    char *number;
    char *namea;
    int len;
    printf("substr stuff\n");
    if ((sub=strstr(messageline,"::"))!=NULL) {
        //imcb_buddy_msg(ic, "skypeconsole","else",0,0);
    printf("getnumber\n");
        number=(char*)g_malloc(sizeof(char)*(sub-messageline)+1); 
        if ((strstr(messageline,";"))!=NULL){
            len=strstr(messageline,";")-sub;
            printf("mallocshit\n");
            namea=g_malloc(sizeof(char)*(len));
            printf("mallocedshit\n");
            memcpy(number,messageline,sub-messageline);
            memcpy(namea, sub+2, len-2);
            namea[len-2]='\0';
            number[sub-messageline]='\0';
            format_phonenumber(number);
            //imcb_buddy_msg(ic, "skypeconsole",number,0,0);
            imcb_buddy_msg(ic, number,namea,0,0);
            //char *testmsg="/me DIDN'T RECEIVE (NO SERVICE): ";
            //imcb_buddy_msg(ic,number,testmsg,0,0);

            char* script;//="/home/impulse/src/scripts/inc/android_sms_message";
            script=set_getstr(&(ic->acc->set), "script");//QQ
            char* fullcommand;

            bee_user_t *user;
            user=  bee_user_by_handle(ic->bee,ic,number);
            char* nick;
            nick=user->nick;

            printf("cmd stuff\n");
            int cmdlen;
            cmdlen=sizeof(char) * (strlen(nick)+strlen(script)+strlen(namea) +7 );
            fullcommand=g_malloc( cmdlen );
            int retval;
            retval=g_snprintf(fullcommand,cmdlen,"%s \"%s\" \"%s\"",script,nick,namea);
            printf("%d/%d\n",retval,cmdlen);
            system(fullcommand);
            printf("cmd stuff done\n");
            //system("/home/impulse/src/scripts/inc/android_sms_message test");
        }
    }


}

void handle_sentnotification(struct im_connection *ic, char* messageline)
{
    char *sub;
    char *number;
    char *namea;
    char *subline;
    int len;

    int start,end;
    int id;
    char *strid;
    int result;
    char *strresult;
    char *message;

    printf("HANDLE SENT NOTIFICATION\n");

    start=index(messageline,'X')-messageline;
    end=index(messageline,':')-messageline;
    //return;

    strid=g_malloc(sizeof(char)*strlen(messageline));
    strresult=g_malloc(sizeof(char)*strlen(messageline));
    memcpy(strid,messageline, strlen(messageline));
    memcpy(strresult,messageline, strlen(messageline));
    //sprintf(
    id=strchr(messageline,'/')+1-messageline;
    result=strchr(messageline+id,'/')+1-messageline;
    message=index(messageline,':')+2;

    strid=strid+id;
    strid[result-3]='\0';
    printf("strid %s\n",strid);
    printf("id %d\n",id);

    strresult=strresult + result;
    strresult[index(strresult,'X')-strresult]='\0';
   
    number=g_malloc(sizeof(char)*(end-start+3));
    memcpy(number,messageline+start+1,end-start-1);
    number[end-start]='\0';
    printf("NUM: %s\n",number);
    format_phonenumber(number);
    printf("NUm: %s\n",number);

    printf("result %d\n",result);
    printf("strresult %s\n",strresult);
    id=atoi(strid);
    result=atoi(strresult);
    printf("converted id %d\n",id);

    char *errmessage;
    char *reason;

    const char *success="sent";
    const char *fail_general="failed";
    const char *fail_pdu="no PDU";
    const char *fail_service="no service";
    const char *fail_4="no service";
    int color;
    char *errmessage_fmt="/me [%c%d%s%c]: %s";
    switch(result)
    {
    case -1:
        imcb_log(ic, "OK");
        reason=success;
        color=2;
        break;
    case 1:
        //imcb_log(ic, "FAILED");
        //imcb_buddy_msg(ic, number, "    FAILED", 0, 0);
        reason=fail_general;
        color=4;
        break;
    case 2:
        //imcb_log(ic, "PDU");
        //imcb_buddy_msg(ic, number, "    FAILED", 0, 0);
        reason=fail_pdu;
        color=4;
        break;
    case 3:
        //imcb_log(ic, "NO SERVICE");
        //imcb_buddy_msg(ic, number, "    FAILED", 0, 0);
        reason=fail_service;
        color=4;
        break;
    case 4:
        //imcb_log(ic, "NO SERVICE");
        //imcb_buddy_msg(ic, number, "    FAILED", 0, 0);
        reason=fail_4;
        color=4;
        break;
    }
    if(result>-3){
        int bufsize;
        bufsize=strlen(reason)+strlen(errmessage_fmt)+strlen(message);
        errmessage=g_malloc(bufsize);//ZZ
        g_snprintf(errmessage,bufsize,errmessage_fmt,0x03,color,reason,0x03,message);
    }
    printf("MSMSMS:  %s\n",errmessage);
    imcb_buddy_msg(ic,number,errmessage,0,0);





    printf("full:%s\n",messageline);
    printf("%d - %d\n",start,end);

}

static void androidsms_init(account_t *acc)
{
    struct im_connection *ic = acc->ic;
    //imcb_log(NULL, "Init androidsms");
    printf(ic, "Init androidsms");
    struct sms_data *sd = g_new0(struct sms_data, 1);
    printf("SEGFAULT00\n");
    sd->lock_buddy=0;

    set_t *s;
    printf("SEGFAULT00\n");

    s = set_add(&acc->set, "server", ANDROIDSMS_DEFAULT_SERVER, NULL,
            acc);

    printf("SEGFAULT00\n");

    //s->flags |= ACC_SET_OFFLINE_ONLY;

    s = set_add(&acc->set, "port", ANDROIDSMS_DEFAULT_PORT, set_eval_int, acc);
    s->flags |= ACC_SET_OFFLINE_ONLY;
    printf("SEGFAULT00\n");

    s = set_add(&acc->set, "script", ANDROIDSMS_DEFAULT_SCRIPT, set_eval_int, acc);
    s->flags |= ACC_SET_OFFLINE_ONLY;

    //if (set_getbool(&acc->set, "skypeconsole"))
    //	imcb_add_buddy(ic, "skypeconsole", NULL);
}

static void send_contact_request(account_t *acc)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *phoneserver;

    //portno=8888;
    phoneserver=gethostbyname( set_getstr(&acc->set, "server") );
    if (phoneserver){
        portno=set_getint(&acc->set, "port");
        sockfd=socket(AF_INET, SOCK_STREAM, 0);
        printf("phoneserver: %d\n",phoneserver);
        bzero((char *) &serv_addr,sizeof(serv_addr));
        serv_addr.sin_family=AF_INET;
        bcopy((char *)phoneserver->h_addr,
                (char *)&serv_addr.sin_addr.s_addr,
                phoneserver->h_length);
        serv_addr.sin_port=htons(portno);
        //printf("sending contact request  (function) to |%s:%s\n", set_getstr(&acc->set, "server"), set_getstr(&acc->set, "port"));

        ///*
           struct timeval timeout;
           timeout.tv_sec=2;
           timeout.tv_usec=0;
           if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");
           if (setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");
          // */
        int retval;

        retval=-2;
            printf("sending contact request  (function) to |%s:%s\n", set_getstr(&acc->set, "server"), set_getstr(&acc->set, "port"));

            char* phone_str=g_malloc(200);
            retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

        while(retval)
        {
            printf("sending contact request  (function) to |%s:%s\n", set_getstr(&acc->set, "server"), set_getstr(&acc->set, "port"));

            char* phone_str=g_malloc(200);
            retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
            printf("connected %i\n",retval);
            if (retval) {
                printf("  errno %i\n",errno);
                perror("requesting contacts");
            }
            printf("  portno %d\n",portno);
            retval=0;
        }
        char* contactquery= "sendcontacts\n";
        write(sockfd,contactquery,strlen(contactquery));
        printf("wrote\n");
        close(sockfd);
    }
}

static void register_withserver(struct im_connection *ic)
{

    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char *message;

    account_t *acc;
    acc=ic->acc;

    printf("Register\n\n");
    message=g_malloc(sizeof(char)*3);//Just send a single 'R' for now ZZ

    message[0]=MSG_REGISTER;
    message[1]='\n';
    message[2]='\0';

    portno=set_getint(&acc->set, "port");
    sockfd=socket(AF_INET, SOCK_STREAM, 0);
    server=gethostbyname( set_getstr(&acc->set, "server") );
    printf("  with: %s:%d\n",set_getstr(&acc->set, "server"), set_getint(&acc->set, "port"));
    bzero((char *) &serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    bcopy((char *)server->h_addr,
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port=htons(portno);

           struct timeval timeout;
           timeout.tv_sec=2;
           timeout.tv_usec=0;
           if (setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");
           if (setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO, (char*)&timeout,sizeof(timeout))<0)
               error("socket option set failed\n");

    int retval;
    retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
    if (retval>=0) {
        write(sockfd,message,strlen(message));
        printf("Sent registration message\n");
    }else{
        printf("Register Connect retval: %d\n",retval);
        printf("register Errno: %d\n",errno);
        perror("Register ");
    }

    close(sockfd);

}

static void setup_udp(struct sms_data *sd, account_t *acc)
{
    //struct sms_data *sd=acc->sd;
    struct im_connection *ic = acc->ic;
    printf("udp setup\n");
    imcb_log(ic, "UDP setup");

    struct sockaddr_in sendaddr;
    struct sockaddr_in recvaddr;
    int numbytes;
    int addr_len;
    int broadcast=1;
    if((sd->udp_socket = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }
    if((setsockopt(sd->udp_socket,SOL_SOCKET,SO_BROADCAST,
                    &broadcast,sizeof broadcast)) == -1)
    {
        perror("setsockopt - SO_SOCKET ");
        exit(1);
    }
    printf("Socket created\n");
    sendaddr.sin_family = AF_INET;

    int portno;
    portno=set_getint(&acc->set, "port");

    //sendaddr not necessary?
    sendaddr.sin_port = htons(portno+1);
    sendaddr.sin_addr.s_addr = INADDR_ANY;
    memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);

    recvaddr.sin_family = AF_INET;
    recvaddr.sin_port = htons(portno+1);
    recvaddr.sin_addr.s_addr = INADDR_ANY;
    memset(recvaddr.sin_zero,'\0',sizeof recvaddr.sin_zero);
    printf("bind udp\n");
    if(bind(sd->udp_socket, (struct sockaddr*) &recvaddr, sizeof recvaddr) == -1)
    {
        perror("bind");
        exit(1);
    }
    send_contact_request(acc);

    //udp_receive
    b_input_add(sd->udp_socket,B_EV_IO_READ,(b_event_handler)udp_receive,acc);
    imcb_log(ic,"receiver added");

}

static void androidsms_chat_msg(struct groupchat *gc, char *message, int flags)
{
    return;
}

static gboolean connection_accept(gpointer data, gint fd, b_input_condition cond)
{
    struct im_connection *ic=data;
    struct sms_data *sd=ic->proto_data;
    int client_sock;
    struct sockaddr_in server, client;
    int c;
    //printf("accept connection\n");

    c = sizeof(struct sockaddr_in);
    client_sock = accept(sd->socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

    b_input_add(client_sock,B_EV_IO_READ,(b_event_handler)connection_receive,ic);
    //imcb_log(ic,"Connection accepted");
    if (client_sock < 0) {
        perror("accept failed");
    }
    //puts("Connection accepted");

    //Receive a message from client
}

static gboolean connection_receive(gpointer data, gint fd, b_input_condition cond)
{
    struct im_connection *ic=data;
    char client_message[2000];
    int read_size;
    printf("connection_receive\n");

    //printf("receive data\n");

    read_size = recv(fd, client_message , 2000 , 0);
    if (read_size>0)
    {
        //printf("READ_SIZE: %d\n",read_size);
        //printf("MESSAGE:\n %s\n",client_message);
        //imcb_buddy_msg(ic, "skypeconsole", "  CONNECTION ACCEPT ", 0, 0);
        char *next;
        char *line;
        int len;
        int offset=0;
        client_message[read_size]='\0';
    printf("while\n");
        while((next=strchr(client_message+offset,'\n'))>0 && offset<read_size)
        {
            //printf("line\n");
            len=next-(client_message+offset);
            line=g_malloc(sizeof(char)*(len+1));
            //printf("MALLOC4: %x\n",line);
    printf("memcpy\n");
            memcpy(line,client_message+offset,len);
    printf("memcpyd\n");
            //printf("%s\n",line);
            line[len]='\0';
            //printf("LNL%d\n",len);
            offset=next-client_message+1;
            if (line[0]==MSG_CONTACTS)
            {
                //printf("handle_contact\n");
                handle_contact(ic,line);
            }
            else if (line[0]=='C') 
                imcb_log(ic,line);
            else if (line[0]==MSG_DELIVERED)
                imcb_log(ic,line);
            else if (line[0]==MSG_SENT)
            {
                imcb_log(ic,line);
                handle_sentnotification(ic,line);
            }
            else
            {
                printf("handle_message\n");
                handle_message(ic,line);
            }
            printf("gfree1\n");
            g_free(line);
            printf("gfree1o\n");
        }
        printf("onreceive TRUE\n");
        //close(fd);
        return TRUE;
    } else {
        printf("onreceive FALSE\n");
        close(fd);
        return FALSE;
        //printf("CALLBACK FALSE\n");
    }

    if(read_size == 0) {
        //puts("Client disconnected");
        //fflush(stdout);
    } else if(read_size == -1) {
        perror("recv failed");
    }

}

static gboolean udp_receive(gpointer data, gint fd, b_input_condition cond)
{
    account_t *acc=data;
    struct im_connection *ic = acc->ic;
    imcb_log(ic, "UDP receive");

    char buf[256];
    int addr_len;
    int numbytes;
    int portno;

    struct sockaddr_in sendaddr;
    sendaddr.sin_family = AF_INET;
    portno=set_getint(&acc->set, "port");
    sendaddr.sin_port = htons(portno+1);
    sendaddr.sin_addr.s_addr = INADDR_ANY;
    memset(sendaddr.sin_zero,'\0', sizeof sendaddr.sin_zero);

    addr_len = sizeof sendaddr;
    if ((numbytes = recvfrom(fd, buf, sizeof buf, 0,
                    (struct sockaddr *)&sendaddr, (socklen_t *)&addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    char *serveraddress;
    const IPBUFLEN = 30;
    serveraddress=g_malloc(IPBUFLEN);
    inet_ntop(AF_INET,&(sendaddr.sin_addr),serveraddress,IPBUFLEN);
    imcb_log(ic, "server address: %s",serveraddress);
    imcb_log(ic, "  retval: %d",set_setstr(&acc->set,"server",serveraddress));
    imcb_log(ic, "  set to: %s",set_getstr(&acc->set,"server"));
    
    //printf("numbytes: %d \n",numbytes);
    buf[numbytes]=0;
    printf("%s\n",buf);

    //pthread_t sendreq;
    //pthread_create(&sendreq,NULL,(void*)send_contact_request,acc);

    //sleep(1);


    //sleep(1);




    send_contact_request(acc);
    register_withserver(ic);




    //imcb_log(ic,"got: %s ",&buf);
    imcb_log(ic, "UDP received");
}

static void androidsms_logout(struct im_connection *ic) 
{
    printf("\nXX logout XX\n");
    struct sms_data *sd=ic->proto_data;
    imcb_log(ic, "logging out");
    b_event_remove(sd->acceptevent);
    printf("SEGFAULT\n");
    printf("%X\n",sd);
    close(sd->socket_desc);
    close(sd->udp_socket);
    printf("NAH\n");
    g_free(sd);
    imc_logout(ic, TRUE);
    //close(sd->socket_desc);

}

void init_plugin(void)
{
    struct prpl *ret = g_new0(struct prpl, 1);

    ret->name = "android-sms";
    ret->login = androidsms_login;
    ret->init = androidsms_init;
    ret->logout = androidsms_logout;
    ret->buddy_msg = androidsms_buddy_msg;
    //ret->get_info = skype_get_info;
    //ret->set_my_name = skype_set_my_name;
    //ret->away_states = skype_away_states;
    //ret->set_away = skype_set_away;
    ret->add_buddy = androidsms_add_buddy;
    //ret->remove_buddy = skype_remove_buddy;
    ret->chat_msg = androidsms_chat_msg;
    //ret->chat_leave = skype_chat_leave;
    //ret->chat_invite = skype_chat_invite;
    //ret->chat_with = skype_chat_with;
    ret->handle_cmp = g_strcasecmp;
    //ret->chat_topic = skype_chat_topic;
#if BITLBEE_VERSION_CODE > BITLBEE_VER(3, 0, 1)
    //ret->buddy_action_list = skype_buddy_action_list;
    //ret->buddy_action = skype_buddy_action;
#endif
    register_protocol(ret);
}
