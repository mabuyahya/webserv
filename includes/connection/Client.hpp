enum ClientState {
    READING_REQUEST,
    PROCESSING_CGI,// New state for CGI processing
    WRITING_RESPONSE,
    COMPLETE
};

class Client {
private:
    int                                 _socketFd;
    struct sockaddr_in                  _address;
    const ServerConfig* _serverConfig; // The config of the port they connected to
    time_t                              _lastActivityTime;
    
    ClientState                         _state;
    HttpRequest                         _request;
    HttpResponse                        _response;

public:
    Client(int fd, const ServerConfig* config);
    
    bool    hasTimedOut() const;
    void    updateActivity();
    

    void    setClientState(ClientState newState);
    void    setRequest(const HttpRequest& request);
    void    setResponse(const HttpResponse& response);    



    int     getSocketFd() const ;
    const ServerConfig* getServerConfig() const ;
    ClientState getState() const ;
    HttpRequest& getRequest() ;
    HttpResponse& getResponse() ;
};