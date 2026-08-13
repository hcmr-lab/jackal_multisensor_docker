//
// Implementation by Luis Garrote
//


#include "walker_driver/Sockets.h"


Sockets::Sockets()
{
    socketid=-1;
    memset(&addr,0,sizeof(addr));
    FrameSize=256;
}

Sockets::Sockets(int fd)
{
    socketid=fd;
    memset(&addr,0,sizeof(addr));
    FrameSize=256;
}
bool Sockets::create()
{


    if(socketid !=-1)
    {
        shutdown(socketid,SHUT_RDWR);

    }
    

    socketid = socket(AF_INET, SOCK_STREAM, 0);

    if(socketid<0)
    {

        perror("Socket is not well created ");
        return false;
    }

    return true;
    
}

bool Sockets::setReuseMode(int on)
{
    if(socketid !=-1)
    {
        if ( setsockopt (socketid, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 ){

            perror("On setReuseMode");
            return false;
        }
        return true;
    }
    else
    {
        perror("Socket is not OK");
        return false;
    }
}

bool Sockets::bindTo(int port)
{
    if(socketid !=-1)
    {

        addr.sin_addr.s_addr=INADDR_ANY;
        addr.sin_family=AF_INET;
        addr.sin_port=htons(port);

        if(bind (socketid,( struct sockaddr * ) &addr,sizeof (addr))<0)
        {
            perror("At Bind :");

            return false;

        }

        return true;
    }
    else
    {
        perror("Socket is not OK");
        return false;
    }
}

bool Sockets::listenTo(int peers)
{

    if(socketid !=-1)
    {
        if(listen (socketid, peers)<0)
        {

            perror("Listen : ");
            return false;

        }
        

        return true;
    }
    else
    {
        perror("Socket is not OK");
        return false;
    }
}

bool Sockets::isOk()
{
    //peerStatus()==false;
    if((socketid==-1)){
        return false;
    }else{
        return true;
    }
}


bool Sockets::incomming(){

    fd_set rfds;
    struct timeval tv;
    int retval;

    /* Watch stdin (fd 0) to see when it has input. */
    FD_ZERO(&rfds);
    FD_SET(socketid, &rfds);

    /* Wait up to five seconds. */
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    retval = select(1, &rfds, NULL, NULL, &tv);
    /* Don't rely on the value of tv now! */

    if (retval == -1){
        perror("select()");
        return false;
    }
    return  FD_ISSET(0, &rfds);
}

int Sockets::accept()
{

    if(socketid !=-1)
    {

        int addr_size = sizeof ( addr );
        int returnable;

#ifdef __APPLE__

        setBlockingMode(false);
        if ( (returnable = ::accept ( socketid, ( sockaddr * ) &addr, ( socklen_t * ) &addr_size))<=0 ){

            if(errno==EAGAIN || errno==EWOULDBLOCK){

            }else{


            }
            perror(" accept4 ");
            return -1;

        }

#else
        if ( (returnable = accept4 ( socketid, ( sockaddr * ) &addr, ( socklen_t * ) &addr_size,SOCK_NONBLOCK))<=0 ){

            if(errno==EAGAIN || errno==EWOULDBLOCK){

            }else{


            }
            perror(" accept4 ");
            return -1;

        }


#endif
        return returnable;

    }
    else
    {
        perror(" Socket is not ok ");
        return -1;
    }
}


bool Sockets::setBlockingMode(bool sts)
{
    if(socketid !=-1)
    {

        int opts;

        opts = fcntl (socketid,F_GETFL);

        if ( opts < 0 )
        {
            perror(" fcntl ");
            return false;
        }

        if ( sts )
            opts = ( opts | O_NONBLOCK );
        else
            opts = ( opts & ~O_NONBLOCK );

        opts=fcntl ( socketid,F_SETFL,opts );
        if(opts<0){
            perror(" fcntl ");
            return false;
        }
        return true;
    }else{
        perror(" Socket is not Ok ");
        return false;
    }
}

int Sockets::sendMessage( std::string str )
{
    if(Sockets::isOk())
    {
        int sts = send ( socketid, str.c_str(), str.size(), MSG_NOSIGNAL );
        if ( sts == -1 )
        {
            perror(" send ");
            return -1;
        }
        else
        {
            return sts;
        }
    }
    else
    {
        perror(" Socket is not Ok ");
        return -1;
    }
}

int Sockets::sendMessage( char* str )
{
    if(Sockets::isOk())
    {
        int sts = send ( socketid, str, strlen(str), MSG_NOSIGNAL );
        if ( sts == -1 )
        {
            perror(" send ");

            return -1;
        }
        else
        {
            
            return 1;
        }
    }
    else
    {

        perror(" Socket is not ok ");
        return -1;
    }
}

int Sockets::receiveMessage ( std::string& s )
{
    if(Sockets::isOk())
    {
        char *buf;
        buf=(char *)malloc(FrameSize * sizeof (char));
        s = "";

        memset (buf, 0, FrameSize);
        int sts=FrameSize;
        int counter=0;
        while(sts==FrameSize)
        {
            sts = recv ( socketid, buf, FrameSize, 0 );
            s = s+buf;
            memset (buf, 0, FrameSize);
            counter=counter+sts;
            //std::cout<<s<<std::endl;
        }

        if(sts==-1 || sts==0)
        {

            if(errno==EAGAIN || errno==EWOULDBLOCK){
                errno=0;
                return 0;
            }
            perror(" recv ");
            free(buf);
            return -1;

        }
        else
        {
            free(buf);
            return counter;
        }
    }else{
        perror(" Socket is not ok ");
        return -1;
    }

}

bool Sockets::connectTo( std::string host, int port )
{
    if (socketid ==-1 ) return false;

    addr.sin_family = AF_INET;
    addr.sin_port = htons ( port );

    memset(&(addr.sin_zero), 0, 8);

    addr.sin_addr.s_addr = inet_addr( host.c_str());

    int sts = inet_pton ( AF_INET, host.c_str(), &addr.sin_addr );

    if ( errno == EAFNOSUPPORT ) {
        perror(" inet_pton ");
        return false;
    }

    sts = connect ( socketid, ( sockaddr * ) &addr, sizeof ( addr ) );

    
    if ( sts == 0 )
    {
        return true;
    }
    else{
        perror("Erro no Connect");
        
        return false;
    }
}

Sockets::~Sockets()
{  
    shutdown(socketid,SHUT_RDWR);

}

bool Sockets::close(){
    if(Sockets::isOk())
    {
        
        shutdown(socketid,SHUT_RDWR);
        return true;
    }
    
    return false;
}

bool Sockets::setReceiveTimeOut_us(long usec)
{

    if(socketid !=-1){
        struct timeval tv;
        tv.tv_sec = (usec/1000000);  //timeout socket...
        tv.tv_usec=(usec%1000000);
        if(setsockopt(socketid, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv,sizeof(struct timeval))==-1)
            return false;
        return true;
    }
    return false;
}
bool Sockets::setSendTimeOut_us(long usec)
{

    if(socketid !=-1){
        struct timeval tv;
        tv.tv_sec = (usec/1000000);  //timeout socket...
        tv.tv_usec=(usec%1000000);
        if(setsockopt(socketid, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv,sizeof(struct timeval))==-1)
            return false;
        return true;
    }
    return false;
}

bool Sockets::waitAvailableToRead(int timeout){

    if(Sockets::isOk()){
        struct pollfd fds[1];

        fds[0].fd = socketid;
        fds[0].events = POLLIN;
        fds[0].revents = 0;
        int stats=poll(fds, 1, timeout);
        if ( stats == 0 )
        {
            return false;
        }
        else if(stats ==-1){
            perror("poll");
            return false;
        }else{
            if((fds[0].revents & POLLIN)>0){
                return true;
            }
            return false;
        }
    }
    perror("Erro no Socket");
    return false;
}

bool Sockets::waitAvailableToSend(int timeout){
    if(Sockets::isOk()){
        struct pollfd fds[1];

        fds[0].fd = socketid;
        fds[0].events = POLLOUT;
        fds[0].revents = 0;
        int stats=poll(fds, 1, timeout);
        if ( stats == 0 )
        {
            return false;
        }
        else if(stats ==-1){
            perror("Erro no poll");
            return false;
        }else{
            if((fds[0].revents & POLLOUT)>0)
                return true;
            return false;
        }

    }
    perror("Erro no Socket");
    return false;
}


void Sockets::printLastError(std::ostream& stream,std::string at){


    stream<<"Error "<<at<<"  : "<<strerror(errno)<<" \n";
    //stream<<"Last Status : "<<getStatusMsg()<<" \n";

}

bool Sockets::peerStatus(){

    if(socketid==-1)
        return false;
    char *buf;
    buf=(char *)malloc(1 * sizeof (char));

    memset (buf, 0, 1);
    int sts=1;
    
    sts = recv ( socketid, buf, 0, 0 );

    
    if(sts==-1)
    {
        perror("Erro no recv");
        free(buf);
        return false;

    }
    else
    {
        
        free(buf);
        return true;
    }

}
void Sockets::keepAliveMode(){
    int so_keepalive=1;
    int out;
    out = setsockopt(socketid,SOL_SOCKET,SO_KEEPALIVE,&so_keepalive, sizeof(so_keepalive));

}

