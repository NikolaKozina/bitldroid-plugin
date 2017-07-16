//change memcpy to strcpy
#define _XOPEN_SOURCE
#define _BSD_SOURCE
#include <stdlib.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
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
static void send_contact_request(account_t *acc);
static gboolean udp_receive(gpointer data, gint fd, b_input_condition cond);
static gboolean connection_accept(gpointer data, gint fd, b_input_condition cond);
static gboolean connection_receive(gpointer data, gint fd, b_input_condition cond);

void handle_sentnotification(struct im_connection *ic, char* messageline, int len);

const char MSG_REGISTER = 'R';
const char MSG_SENT = 'S';
const char MSG_DELIVERED = 'D';
const char MSG_CONTACTS = 'X';

static void androidsms_login(account_t *acc)
{
    struct im_connection *ic = imcb_new(acc);
    struct sms_data *sd = g_new0(struct sms_data, 1);

    ic->proto_data = sd;

    imcb_log(ic, "Connecting");

    printf("SMSLISTENER\n");
    //imcb_log(ic,"SMSListener starting..");

    int client_sock , c , read_size;
    struct sockaddr_in server , client;

    sd->socket_desc = socket(AF_INET , SOCK_STREAM , 0);

    struct timeval timeout;
    timeout.tv_sec=2;
    timeout.tv_usec=0;
    if (setsockopt(sd->socket_desc,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT, (char*)&timeout,sizeof(timeout))<0)
        error("bind socket option set failed\n");

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
    imcb_log(ic, "Binding listen socket on port %d", set_getint(&acc->set, "port"));
    if( bind(sd->socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        imcb_log(ic,"Binding failed", strerror(errno));
        close(sd->socket_desc);
        return;
    }
    //puts("bind done");

    //Listen
    listen(sd->socket_desc , 3);
    //imcb_log(ic,"Listening...");

    //Accept and incoming connection
    sd->acceptevent = b_input_add(sd->socket_desc,B_EV_IO_READ,(b_event_handler)connection_accept,ic);
    //imcb_connected(ic);
    //imcb_log(ic,"Connected");
    ic->flags &= ~(OPT_PONGS);
    setup_udp(sd,acc);
    send_contact_request(acc);

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
        g_free(errmessage);
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

//Handle getting contact list from phone
void handle_contact(struct im_connection *ic, char* contactline)
{
    struct sms_data *sd=ic->proto_data;
    char *sub;
    char *number;
    char *namea;
    int len;
    printf("handle contact\n");
    imcb_connected(ic);

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

void handle_message(struct im_connection *ic, char* message, int msglen)
{
    char* delimiter;
    char* phonenumber;
    char* text;
    int phonelen, textlen;

    delimiter=memchr(message,0x1E,msglen);
    phonelen=delimiter-message+1; //add a byte for the string terminator
    phonenumber=g_malloc(phonelen);
    textlen=msglen-phonelen+1; //add a byte for the string terminator
    text=g_malloc(textlen);

    memcpy(phonenumber,message,delimiter-message);
    memcpy(text,delimiter+1,textlen);

    phonenumber[phonelen-1]='\0';
    text[textlen-1]='\0';

    format_phonenumber(phonenumber);
    
    imcb_buddy_msg(ic, phonenumber, text, 0, 0);


    //Run user defined script (script setting)
    char* script;//="/home/impulse/src/scripts/inc/android_sms_message";
    script=set_getstr(&(ic->acc->set), "script");//QQ
    char* fullcommand;

    bee_user_t *user;
    user=  bee_user_by_handle(ic->bee, ic, phonenumber);
    char* nick;
    nick=user->nick;

    printf("cmd stuff\n");
    int cmdlen;
    int retval;
    if (nick==0)
    {
        cmdlen=sizeof(char) * (strlen(phonenumber)+strlen(script)+strlen(text) +7 );
        fullcommand=g_malloc( cmdlen );
        retval=g_snprintf(fullcommand,cmdlen,"%s \"%s\" \"%s\"",script,phonenumber,text);
    } else {
        cmdlen=sizeof(char) * (strlen(nick)+strlen(script)+strlen(text) +7 );
        fullcommand=g_malloc( cmdlen );
        retval=g_snprintf(fullcommand,cmdlen,"%s \"%s\" \"%s\"",script,nick,text);
    }
    printf("%d/%d\n",retval,cmdlen);
    system(fullcommand);
    printf("cmd stuff done\n");
    g_free(fullcommand);
    g_free(phonenumber);
    g_free(text);
}

void handle_sentnotification(struct im_connection *ic, char* messageline, int len)
{
    char *sub;
    char *number;
    char *namea;
    char *subline;

    int start,end;
    int id;
    char *strid;
    char *stridoff;
    int result;
    char *strresult;
    char *strresultoff;
    char *message;

    printf("HANDLE SENT NOTIFICATION\n");

    messageline[len]='\0';
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

    stridoff=strid+id;
    stridoff[result-3]='\0';
    printf("stridoff %s\n",stridoff);
    printf("id %d\n",id);

    strresultoff=strresult + result;
    strresultoff[index(strresultoff,'X')-strresultoff]='\0';

    number=g_malloc(sizeof(char)*(end-start+3));
    memcpy(number,messageline+start+1,end-start-1);
    number[end-start]='\0';
    printf("NUM: %s\n",number);
    format_phonenumber(number);
    printf("NUm: %s\n",number);

    printf("result %d\n",result);
    printf("strresultoff %s\n",strresultoff);
    id=atoi(stridoff);
    g_free(strid);
    result=atoi(strresultoff);
    g_free(strresult);
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
            //imcb_log(ic, "OK");
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
        printf("MSMSMS:  %s\n",errmessage);
        imcb_buddy_msg(ic,number,errmessage,0,0);
        g_free(errmessage);
    }
    g_free(number);





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

    imcb_log(acc->ic, "Sending contact request to %s:%s", set_getstr(&acc->set, "server"), set_getstr(&acc->set, "port"));

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

        retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));

        while(retval)
        {
            printf("sending contact request  (function) to |%s:%s\n", set_getstr(&acc->set, "server"), set_getstr(&acc->set, "port"));

            retval=connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
            printf("connected %i\n",retval);
            if (retval) {
                printf("  errno %i\n",errno);
                imcb_log(acc->ic, "Error requesting contacts: %s",strerror(errno));
                perror("requesting contacts");
            }
            printf("  portno %d\n",portno);
            retval=0;
        }
        char* contactquery= "sendcontacts\n";
        write(sockfd,contactquery,strlen(contactquery));
        printf("wrote\n");
        close(sockfd);
    } else 
        imcb_log(acc->ic, "Couldn't resolve hostname: %s", set_getstr(&acc->set, "server"));
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
    g_free(message);

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
    printf("accept connection\n");

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
    const READ_BUFFERSIZE=2000;
    char client_message[READ_BUFFERSIZE];
    int read_size;

    static int inHead=0; //Position in header
    static int inText=0; //Position in text
    static unsigned char* header=NULL; //Header start (1) + message type (1) + text length (4) + Header end (1)
    static char* fulltext;
    static int32_t textlen;
    char *headStart;
    char *textStart;

    //imcb_add_buddy(ic, "12345678", NULL);
    //return;

    printf("connection receive\n");
    read_size = recv(fd, client_message , READ_BUFFERSIZE, 0); //Reserve space for changing last char in buffer to \0
    if (read_size>0)
    {
        printf("\n========\nread_size: %d\n\nmessage:\n",  read_size);
        printf("%s\n", client_message);
        int i;

        for(i=0;i<read_size;i++)
            printf("%4d - %x - %c\n",i,(unsigned char)client_message[i], client_message[i]);


        if (inHead+inText==0){ //Are we not currently in the middle of a header or text body?
            if ((headStart=memchr(client_message, 0x01, read_size))>0){//Is there a header start in the stream?
                header=g_malloc(6); //Don't include EOT char (replace with \0)
                if ((textStart=memchr(client_message, 0x02, read_size))>0){//Is there a text start (header end) in the stream?
                    inHead=0;
                    printf("0x02 at: %d",textStart-client_message);
                    printf("if textStart-headStart %d\n",textStart-headStart);
                    if (textStart-headStart==6){ //Is this a good header?
                        memcpy(header, client_message, textStart-headStart);
                        int j;
                        for (j=0;j<6;j++)
                            printf("H:  %x\n", header[j]);
                        textlen=(header[2]<<24) + (header[3]<<16) + ((unsigned char)header[4]<<8) + (unsigned char)header[5] ;
                        printf("textlen: %d\n",textlen);
                        fulltext=g_malloc(textlen+1);

                        printf("textlen: %d %x\n",textlen,(unsigned char)header[5]);
                        if (read_size>textStart-client_message){//Is there more data after the header?
                            int readlen;
                            readlen=read_size-(textStart-client_message);
                            printf("closeout? %d >= %d \n",inText+readlen,textlen);
                            if (inText+ readlen >= textlen){//Is there enough data to close out the last message we were reading?
                                memcpy(fulltext, textStart+1, textlen-inText+1);
                                inText=0;
                                //fulltext[textlen+1]='\0';
                                connection_receive_parse(ic,fulltext,textlen,header[1]);
                                g_free(header);
                                g_free(fulltext);
                                printf("got full message:\n%s\n",fulltext);
                            } else {
                                memcpy(fulltext, textStart, readlen);
                                inText+=readlen;
                            }
                        }
                    } else { //Wrong header length
                        printf("Bad header! size: %d\n",textStart-headStart);
                        inText=0;
                    }
                } else {
                    int readlen;
                    readlen=read_size-(headStart-client_message);
                    printf("reading header fragment: %d\n",readlen);
                    memcpy(header+inHead, headStart, readlen);
                    inHead+=readlen;
                }
            }
        } else if (inHead){
            if ((textStart=memchr(client_message, 0x02, read_size))>0){//Is there a text start (header end) in the stream?
                printf("textStart-client_message: %d\n",textStart-client_message);
                printf("inHead: %d\n",inHead);
                if ((textStart-client_message)+inHead==6){ //Is this a good header?
                    memcpy(header+inHead, client_message, textStart-client_message);
                    textlen=((unsigned char)header[2]<<24) + ((unsigned char)header[3]<<16) + ((unsigned char)header[4]<<8) + header[5] ;
                    printf("h[2]<<24: %d\n",header[2]<<24);
                    printf("h[3]<<16: %d\n",header[3]<<16);
                    printf("h[4]<<8:  %d\n",(unsigned int)header[4]<<8);
                    printf("h[5]:     %d\n\n",(unsigned char)header[5]);

                    printf("h[2]<<24: %d\n",header[2]<<24);
                    printf("h[3]<<16: %d\n",header[3]<<16);
                    printf("h[4]<<8:  %d\n",(unsigned int)header[4]<<8);
                    printf("h[5]:     %x\n",(unsigned int)header[5]);
                    printf("TEXTLEN: %d\n",textlen);
                    fulltext=g_malloc(textlen+2);
                    if (read_size>textStart-client_message){//Is there more data after the header?
                        int readlen;
                        readlen=read_size-(textStart-client_message);
                        if (inText+ readlen >= textlen){//Is there enough data to close out the last message we were reading?
                            memcpy(fulltext, textStart+1, readlen);
                            inText=0;
                            connection_receive_parse(ic,fulltext,textlen,header[1]);
                            g_free(header);
                            g_free(fulltext);
                        } else {
                            memcpy(fulltext, textStart, readlen);
                            inText+=readlen;
                        }
                    }
                } else { //Wrong header length
                    printf("Bad header! size: %d\n",textStart-headStart);
                    inText=0;
                }
                inHead=0;
            } else {
                int readlen;
                readlen=read_size;
                memcpy(header+inHead, client_message, readlen);
                inHead+=readlen;
            }
        } else if (inText){
            if (read_size>textStart-client_message){//Is there more data after the header?
                int readlen;
                readlen=read_size-(textStart-client_message);
                if (inText+ readlen >= textlen){//Is there enough data to close out the last message we were reading?
                    memcpy(fulltext, textStart, textlen-inText);
                    inText=0;
                } else {
                    memcpy(fulltext, textStart, readlen);
                    inText+=readlen;
                }
            }

        }
    }


    

}

void connection_receive_parse(struct im_connection *ic, char* message, int len, char type)
{
    char *offset;
    char *line;

    //imcb_add_buddy(ic, number, NULL);

    switch(type){
        case 'C':
            printf("CONTACT MESSAGE!!!!!!!!!!!!!!!!!!!!!!!\n");
            offset=message;
            while(line=strchr(offset,'X'))
            {
                offset=strchr(line,';')+1;
                if (offset){
                    *offset='\0';
                    offset++;
                    handle_contact(ic,line);
                }
            }
            break;
        case 'M':
            printf("handle_message\n");
            handle_message(ic,message,len);
            break;
        case 'S':
            handle_sentnotification(ic,message,len);
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
    serveraddress=g_malloc(IPBUFLEN); //QQ is this malloc free'd?
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

    ret->name = "bitldroid";
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
