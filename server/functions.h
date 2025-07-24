
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

void serve_client(int client_sock){
	time_t now=time(NULL);
        struct tm* t=localtime(&now);
	
	char buffer[BUF_SIZE];
	size_t bytes_read=read(client_sock,buffer,BUF_SIZE-1);
	if(bytes_read<=0){
		perror("read");
		close(client_sock);
		return;
	}
	buffer[bytes_read]='\0';
	
	char path[256];
	char method[8];
	sscanf(buffer,"%s %s",method,path);
	

	//Submit a note
	int is_post=strcmp(method,"POST")==0;
	int is_submit_route=strcmp(path,"/submit")==0;
	if(is_post && is_submit_route){
		//step 1: Find content length
		char* c1=strstr(buffer,"Content-Length:");
		int content_length=0;
		if(c1){
			sscanf(c1,"Content_Length:%d",&content_length);
		}

		//step 2: Find start of POST body
		char* body=strstr(buffer,"\r\n\r\n");
		if(!body){
			perror("Malformet POST request");
			close(client_sock);
			return;
		}
		body+=4; //skip the \r\n\r\n
		
		//step 3 : If body is not fully in buffer,read more(simpe version assumes small POST)
		if((buffer+bytes_read)-body<content_length){
			perror("POST body too short or incomplete");
			close(client_sock);
			return;
		}

		//step 4: Parse 'note=' from URL-encoded body
		char name[128]={0};
		char note[2048]={0};
		

		char* name_field=strtok(body,"&");
		char* note_field=strtok(NULL,"&");
		if(name_field && strncmp(name_field,"name=",5)==0){
			strncpy(name,name_field+5,sizeof(name)-1);
			url_decode(name);
		}
		if(note_field && strncmp(note_field,"note=",5)==0){
			strncpy(note,note_field+5,sizeof(note)-1);
			url_decode(note);
		}


		//step 5: Save to file
		FILE* notes=fopen("notes.txt","a");
		if(notes){
			fprintf(notes,"From:%s [%04d-%02d-%02d %02d:%02d:%02d] \n \t-%s \n----\n",name,t->tm_year+1900,t->tm_mon+1,t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,note);
			fclose(notes);
		}
		//step 6: Return success response
		const char* response=
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 60\r\n"
			"Connection: Close\r\n\r\n"
			"<html><body>"
			"<h3>Note saved!</h3>"
			"<a href=\"/submit.html\">Back</a>"
			"</body></html>";
		ssize_t post=write(client_sock,response,strlen(response));
		if(post<0){
			perror("response");
		}
		close(client_sock);
		return;
	}//submit a note

#if DEBUG
	printf("Received request: methos=%s,path=%s\n",method,path);
#endif
	if(strcmp(path,"/")==0){
		strcpy(path,"/index.html");
	}
// Clear all notes
if (strcmp(method, "POST") == 0 && strcmp(path, "/clear") == 0) {
    int deleted = remove("notes.txt"); // delete notes.txt
    if (deleted == 0) {
        const char* html =
            "<html><head><title>Notes Cleared</title><link rel=\"stylesheet\" href=\"/style.css\"></head>"
            "<body><h1> All notes cleared!</h1>"
            "<a href=\"/\">Back to Home</a> | <a href=\"/notes\">View Notes</a>"
            "</body></html>";

        char header[BUF_SIZE];
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %zu\r\n"
            "Connection: close\r\n\r\n", strlen(html));

       ssize_t clear= write(client_sock, header, strlen(header));
       if(clear<0){
	       perror("clear header");
       }

       ssize_t clear_notes=write(client_sock, html, strlen(html));
       if(clear_notes<0)
       {
	       perror("html response while clearing notes");
       }
   	 } else {
        perror("remove(notes.txt) failed");

        const char* response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 30\r\n"
            "Connection: close\r\n\r\n"
            "Failed to delete notes.txt file.";
        ssize_t html_response=write(client_sock, response, strlen(response));
	if(html_response<0){
		perror("html response while clearing notes");
	}
    }

    close(client_sock);
    return;
}
	
	//read notes
	if (strcmp(method, "GET") == 0 && strcmp(path, "/notes") == 0) {
 	   FILE* f = fopen("notes.txt", "r");
 	   if (!f) {
        	const char* response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Content-Length: 55\r\n"
                               "Connection: close\r\n\r\n"
                               "<html><body><p>No notes yet.</p><a href=\"/\">Back</a></body></html>";
        	ssize_t notes_response=write(client_sock, response, strlen(response));
        	if(notes_response<0){
			perror("notes response");
		}
		close(client_sock);
       	 	return;
	   }
	fseek(f, 0, SEEK_END);
	long notes_len = ftell(f);
	rewind(f);
	
	char* note_content = malloc(notes_len + 1);
	ssize_t notes_read=fread(note_content, 1, notes_len, f);
	if(notes_read<0){
		perror("note content read");
	}
	note_content[notes_len] = '\0';
	fclose(f);

    	const char* html_start = "<html><body><h1>Saved Notes</h1><pre>\n";
const char* html_end =	"</pre><br>"
 	   		"<form action=\"/clear\" method=\"POST\" onsubmit=\"return confirm('Delete ALL notes?');\" style=\"margin-top:20px;\">"
 	   		"<button type=\"submit\" class=\"danger-button\"> Clear All Notes</button>"
			"</form>"
	   		"<br><a href=\"/\">Back to Home</a></body></html>";

    	int html_len = strlen(html_start) + notes_len + strlen(html_end);
    	char* html_body = malloc(html_len + 1);
    	snprintf(html_body, html_len + 1, "%s%s%s", html_start, note_content, html_end);
	char header[BUF_SIZE];
	snprintf(header, sizeof(header),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
    		"Content-Length: %zu\r\n"
    		"Connection: close\r\n\r\n", strlen(html_body));
    	ssize_t header_write=write(client_sock, header, strlen(header));
    	if(header_write<0){
		perror("note content write");
	}
	ssize_t html_write=write(client_sock,html_body,strlen(html_body));
	if(html_write<0){	
		perror("html_body write");
	}
	free(note_content);
    	free(html_body);
    	close(client_sock);
    	return;
	}//notes

	char full_path[512]="../www/index.html";
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
	
	FILE* log=fopen("access.log","a");
	if(log){
		fprintf(log,"[%04d-%02d-%02d %02d:%02d:%02d] \"%s %s\"n",
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
