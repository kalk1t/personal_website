
void url_decode(char* str){
	char* src=str;
	char* dst=str;
	
	while(*src){
		if(*src=='+'){
			*dst++=' ';
			src++;
		}
		else if(*src=='%' && 
			isxdigit((unsigned char)src[1]) &&
		       	isxdigit((unsigned char)src[2])){
		
			char hex[3]={src[1],src[2],'\0'};
			*dst++=(char)strtol(hex,NULL,16);
			src+=3;
		}else{
			*dst++=*src++;
		}
	}
	*dst='\0';
}

char* html_escape(const char* src){
	size_t len=0;
	for(const char* p=src;*p;p++){
		switch(*p){
			case '&': len+=5;break; //&amp;
			case '<':
			case '>': len+=4;break; //&alt;&gt;
			case '"': len+=6;break; //&quot;
			default: len+=1;break;
		}
	}

	char* out=malloc(len+1);
	char* dst=out;
	for(const char* p=src;*p;p++){
		switch(*p){
			case '&': memcpy(dst,"&amp;",5);dst+=5;break;
			case '<': memcpy(dst,"&lt;",4);dst+=4;break;
			case '>': memcpy(dst,"&gt;",4);dst+=4;break;
			case '"': memcpy(dst,"&quot;",6);dst+=6;break;
			default: *dst++=*p;break;
		}
	}

*dst='\0';
return out;
}

void serve_client(int client_sock){
	time_t now=time(NULL);
        struct tm* t=localtime(&now);
	
	char buffer[BUF_SIZE];
	size_t bytes_read=read(client_sock,buffer,BUF_SIZE-1);
	if(bytes_read<=0){
		perror("read");
		close(client_sock);
		return ;
	}
	buffer[bytes_read]='\0';
	
	char path[256];
	char method[8];
	sscanf(buffer,"%s %s",method,path);

#if DEBUG
	printf("Received request: method=%s,path=%s\n",method,path);
#endif
	if(strcmp(path,"/")==0){
		strcpy(path,"/main.html");
	}
	
	char full_path[512]="../www/main.html";
	snprintf(full_path,sizeof(full_path),"../www%s",path);



#if DEBUG
	printf("Full path: %s\n",full_path);
#endif
	struct stat st;
	if(stat(full_path,&st)!=0){
		perror("stat");
		close(client_sock);
		return;
	}
	long len=st.st_size;
	
	FILE* file=fopen(full_path,"rb");
	if(!file){
		FILE* err_page=fopen("../www/404.html","rb");
		if(err_page){
			fseek(err_page,0,SEEK_END);
			long err_len=ftell(err_page);
			fseek(err_page,0,SEEK_SET);
			char* err_buf=malloc(err_len);
			size_t err_loaded=fread(err_buf,1,err_len,err_page);
			if(err_loaded!=err_len){
				perror("fread (404.html)");
				free(err_buf);
				fclose(err_page);
				close(client_sock);
				return;
			}
			fclose(err_page);

			char header[BUF_SIZE];
			snprintf(header,sizeof(header),
					"HTTP/1.1 404 Not Found\r\n"
					"Content-Type text/html\r\n"
					"Content-Length: %ld\r\n\r\n",
					err_len);
			ssize_t hBytes=write(client_sock,header,strlen(header));
			if(hBytes<0)perror("write header (404)");
			ssize_t bBytes=write(client_sock,err_buf,err_len);
			if(bBytes<0)perror("write body (404)");
			free(err_buf);
		}else{
			//fallback if 404.html is missing
			char* msg="HTTP/1.1 404 Not Found\r\nContent-Length:13\r\n\r\n404 Not Found";
			ssize_t fallback_bytes=write(client_sock,msg,strlen(msg));
			if(fallback_bytes<0)perror("write fallback 404)");
		}
		close(client_sock);
		return;
	}

	char* content=malloc(len+1);
	memset(content,0,len+1);
	size_t loaded=fread(content,1,len,file);
#if DEBUG
	printf("fread loaded = %zu, expected = %ld\n",loaded,len);
#endif
	fclose(file);
	if(loaded!=len){
		perror("fread");
		free(content);
		close(client_sock);
		return;
	}
#if DEBUG
	printf("fread:Success(%zu bytes read)\n",loaded);
#endif
	const char* content_type="text/html";
	if(strstr(path,".css")) content_type="text/css";
	else if(strstr(path,".pdf")) content_type="application/pdf";

	char header[BUF_SIZE];
	snprintf(header,sizeof(header),
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: %s\r\n"
			"Content-Length: %ld\r\n\r\n",
			content_type,len);

	
	ssize_t header_bytes=write(client_sock,header,strlen(header));
#if DEBUG
	printf("Header sent bytes: %zd\n",header_bytes);
#endif
	if(header_bytes<0) perror("write header");

	ssize_t body_bytes=write(client_sock,content,len);
#if DEBUG
	printf("content sent bytes: %zd\n",body_bytes);
#endif
	if(body_bytes<0) perror("write body");
	
	FILE* log=fopen("logs/access.log","a");
	if(log){
		fprintf(log,"[%04d-%02d-%02d %02d:%02d:%02d] \"%s %s\"\n",
				t->tm_year+1900,t->tm_mon+1,t->tm_mday,
				t->tm_hour,t->tm_min,t->tm_sec,method,path);
		fclose(log);
	}

#if DEBUG
	printf("Received request: method=%s, path=%s\n",method,path);
#endif

	free(content);
	close(client_sock);
}
